
// ===================================================================
// DESC: Enhanced scheduler implementation with C23 features,
//       multi-level scheduling, and atomic operations
// Phase 9: Scheduler Integration & Fairness
// ===================================================================
#include "scheduler.h"
#include "fairness.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Forward declarations */
void aeon_hash_meta(const GraphNode* node, uint8_t out_hash[32]);
static bool policy_gate_check(Scheduler* sched, GraphNode* node);

/* ===================================================================
 * Priority to Weight Conversion Tables
 * =================================================================== */

/* Nice values -20 to +19 mapped to weights */
static const uint32_t sched_prio_to_weight_table[40] = {
    /* -20 */ 88761, 71755, 56483, 46273, 36291,
    /* -15 */ 29154, 23254, 18705, 14949, 11916,
    /* -10 */ 9548, 7620, 6100, 4904, 3906,
    /*  -5 */ 3121, 2501, 1991, 1586, 1277,
    /*   0 */ 1024, 820, 655, 526, 423,
    /*   5 */ 335, 272, 215, 172, 137,
    /*  10 */ 110, 87, 70, 56, 45,
    /*  15 */ 36, 29, 23, 18, 15
};

/* Inverse weight multipliers for precise calculations */
static const uint32_t sched_prio_to_wmult_table[40] = {
    /* -20 */ 48388, 59856, 76040, 92818, 118348,
    /* -15 */ 147320, 184698, 229616, 287308, 360437,
    /* -10 */ 449829, 563644, 704093, 875809, 1099582,
    /*  -5 */ 1376151, 1717300, 2157191, 2708050, 3363326,
    /*   0 */ 4194304, 5237765, 6557202, 8165337, 10153587,
    /*   5 */ 12820798, 15790321, 19976592, 24970740, 31350126,
    /*  10 */ 39045157, 49367440, 61356676, 76695844, 95443717,
    /*  15 */ 119304647, 148102320, 186737708, 238609294, 286331153
};

/* ===================================================================
 * Utility Functions
 * =================================================================== */

/**
 * @brief Default time function using clock_gettime
 */
uint64_t aeon_scheduler_default_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

/**
 * @brief Convert priority to weight
 */
uint32_t aeon_scheduler_prio_to_weight(int32_t prio) {
    /* Clamp priority to valid range */
    if (prio < SCHED_PRIO_MIN) prio = SCHED_PRIO_MIN;
    if (prio > SCHED_PRIO_MAX) prio = SCHED_PRIO_MAX;
    
    /* Convert to array index (0-39) */
    int32_t idx = prio - SCHED_PRIO_MIN;
    return sched_prio_to_weight_table[idx];
}

/**
 * @brief Convert priority to inverse weight multiplier
 */
uint32_t aeon_scheduler_prio_to_wmult(int32_t prio) {
    /* Clamp priority to valid range */
    if (prio < SCHED_PRIO_MIN) prio = SCHED_PRIO_MIN;
    if (prio > SCHED_PRIO_MAX) prio = SCHED_PRIO_MAX;
    
    /* Convert to array index (0-39) */
    int32_t idx = prio - SCHED_PRIO_MIN;
    return sched_prio_to_wmult_table[idx];
}

/* ===================================================================
 * Scheduler State Management (Atomic)
 * =================================================================== */

/**
 * @brief Set scheduler state atomically
 */
SchedState aeon_scheduler_set_state(Scheduler* sched, SchedState new_state) {
    if (!sched) return SCHED_STATE_STOPPED;
    return atomic_exchange(&sched->state, new_state);
}

/**
 * @brief Get scheduler state atomically
 */
SchedState aeon_scheduler_get_state(Scheduler* sched) {
    if (!sched) return SCHED_STATE_STOPPED;
    return atomic_load(&sched->state);
}

/* ===================================================================
 * Scheduler Creation and Initialization
 * =================================================================== */

/**
 * @brief Create a new scheduler instance
 */
Scheduler* aeon_scheduler_create(BdiGraph* g, DeviceVTable** devices, size_t dev_count) {
    if (!g || !devices || dev_count == 0) {
        return nullptr;
    }
    
    Scheduler* sched = (Scheduler*)calloc(1, sizeof(Scheduler));
    if (!sched) {
        return nullptr;
    }
    
    /* Initialize core components */
    sched->graph = g;
    sched->devices = devices;
    sched->device_count = dev_count;
    
    /* Initialize security policy (default: insecure mode) */
    sched->policy = (SecurityPolicy){
        .secure_mode = false,
        .required_proof_class = 0
    };
    
    /* Initialize atomic state */
    atomic_init(&sched->state, SCHED_STATE_STOPPED);
    atomic_init(&sched->tick_count, 0);
    atomic_init(&sched->ready_count, 0);
    
    /* Allocate ready set */
    sched->ready_capacity = SCHED_MAX_READY_NODES;
    sched->ready_set = (NodeId*)calloc(sched->ready_capacity, sizeof(NodeId));
    if (!sched->ready_set) {
        free(sched);
        return nullptr;
    }
    
    /* Initialize time function */
    sched->get_time_ns = aeon_scheduler_default_time_ns;
    sched->last_tick_time = sched->get_time_ns();
    
    /* Initialize configuration */
    sched->sched_latency = SCHED_LATENCY_NS;
    sched->min_granularity = SCHED_MIN_GRANULARITY;
    sched->balance_interval_ms = 100; /* 100ms default */
    
    /* Initialize statistics */
    atomic_init(&sched->stats.total_scheduled, 0);
    atomic_init(&sched->stats.total_preemptions, 0);
    atomic_init(&sched->stats.total_migrations, 0);
    atomic_init(&sched->stats.total_context_switches, 0);
    atomic_init(&sched->stats.cfs_scheduled, 0);
    atomic_init(&sched->stats.rt_scheduled, 0);
    atomic_init(&sched->stats.dl_scheduled, 0);
    sched->stats.last_balance_time = sched->last_tick_time;
    
    sched->initialized = false;
    
    return sched;
}

/**
 * @brief Initialize scheduler with fairness algorithms
 */
int aeon_scheduler_init(Scheduler* sched) {
    if (!sched) {
        return -1;
    }
    
    if (sched->initialized) {
        return 0; /* Already initialized */
    }
    
    /* Initialize CFS scheduler */
    sched->cfs_scheduler = fair_scheduler_create_cfs(sched);
    if (!sched->cfs_scheduler) {
        fprintf(stderr, "Failed to initialize CFS scheduler\n");
        return -1;
    }
    
    /* Initialize RT scheduler */
    sched->rt_scheduler = fair_scheduler_create_rt(sched);
    if (!sched->rt_scheduler) {
        fprintf(stderr, "Failed to initialize RT scheduler\n");
        fair_scheduler_destroy_cfs(sched->cfs_scheduler);
        return -1;
    }
    
    /* Initialize Deadline scheduler */
    sched->dl_scheduler = fair_scheduler_create_deadline(sched);
    if (!sched->dl_scheduler) {
        fprintf(stderr, "Failed to initialize Deadline scheduler\n");
        fair_scheduler_destroy_cfs(sched->cfs_scheduler);
        fair_scheduler_destroy_rt(sched->rt_scheduler);
        return -1;
    }
    
    /* Initialize Device scheduler */
    sched->device_scheduler = device_scheduler_create(sched);
    if (!sched->device_scheduler) {
        fprintf(stderr, "Failed to initialize Device scheduler\n");
        fair_scheduler_destroy_cfs(sched->cfs_scheduler);
        fair_scheduler_destroy_rt(sched->rt_scheduler);
        fair_scheduler_destroy_deadline(sched->dl_scheduler);
        return -1;
    }
    
    sched->initialized = true;
    atomic_store(&sched->state, SCHED_STATE_RUNNING);
    
    printf("Scheduler initialized with CFS, RT, Deadline, and Device scheduling\n");
    return 0;
}

/**
 * @brief Shutdown scheduler
 */
int aeon_scheduler_shutdown(Scheduler* sched) {
    if (!sched || !sched->initialized) {
        return -1;
    }
    
    /* Set state to stopped */
    atomic_store(&sched->state, SCHED_STATE_STOPPED);
    
    /* Cleanup sub-schedulers */
    if (sched->device_scheduler) {
        device_scheduler_destroy(sched->device_scheduler);
        sched->device_scheduler = nullptr;
    }
    
    if (sched->dl_scheduler) {
        fair_scheduler_destroy_deadline(sched->dl_scheduler);
        sched->dl_scheduler = nullptr;
    }
    
    if (sched->rt_scheduler) {
        fair_scheduler_destroy_rt(sched->rt_scheduler);
        sched->rt_scheduler = nullptr;
    }
    
    if (sched->cfs_scheduler) {
        fair_scheduler_destroy_cfs(sched->cfs_scheduler);
        sched->cfs_scheduler = nullptr;
    }
    
    sched->initialized = false;
    
    printf("Scheduler shutdown complete\n");
    return 0;
}

/**
 * @brief Free scheduler resources
 */
void aeon_scheduler_free(Scheduler* sched) {
    if (!sched) return;
    
    /* Shutdown if still running */
    if (sched->initialized) {
        aeon_scheduler_shutdown(sched);
    }
    
    /* Free ready set */
    free(sched->ready_set);
    
    /* Free scheduler structure */
    free(sched);
}

/* ===================================================================
 * Security Policy Gate (from original implementation)
 * =================================================================== */

/**
 * @brief Policy gate check for secure execution
 */
static bool policy_gate_check(Scheduler* sched, GraphNode* node) {
    /* If not in secure mode, all checks pass */
    if (!sched->policy.secure_mode) {
        return true;
    }
    
    /* In secure mode, node must have attached metadata */
    if (node->meta_off == 0 && sched->graph->meta_size > 0) {
        /* Special case handling */
    } else if (node->meta_off > sched->graph->meta_size) {
        printf("POLICY_GATE: REJECT Node %llu - No metadata in secure mode.\n", 
               (unsigned long long)node->id);
        return false;
    }
    
    NodeMeta* meta = (NodeMeta*)(sched->graph->meta_arena + node->meta_off);
    
    /* Check 1: Minimum required proof class */
    if ((meta->proof_class & sched->policy.required_proof_class) != 
        sched->policy.required_proof_class) {
        printf("POLICY_GATE: REJECT Node %llu - Does not meet required proof class.\n",
               (unsigned long long)node->id);
        return false;
    }
    
    /* Check 2: Metadata hash validation (Merkle-hashing) */
    uint8_t computed_hash[32];
    aeon_hash_meta(node, computed_hash);
    if (memcmp(meta->hash, computed_hash, 32) != 0) {
        printf("POLICY_GATE: REJECT Node %llu - Metadata hash mismatch! (Tampering detected)\n",
               (unsigned long long)node->id);
        return false;
    }
    
    printf("POLICY_GATE: PASS Node %llu.\n", (unsigned long long)node->id);
    return true;
}

/* ===================================================================
 * Scheduler Operations
 * =================================================================== */

/**
 * @brief Set security policy
 */
void aeon_scheduler_set_policy(Scheduler* sched, SecurityPolicy policy) {
    if (!sched) return;
    sched->policy = policy;
}

/**
 * @brief Add a node to the scheduler
 */
int aeon_scheduler_add_node(Scheduler* sched, NodeId node_id,
                            SchedPolicy policy, int32_t priority) {
    if (!sched || !sched->initialized) {
        return -1;
    }
    
    /* Route to appropriate sub-scheduler based on policy */
    switch (policy) {
        case SCHED_NORMAL:
        case SCHED_BATCH:
        case SCHED_IDLE:
            return fair_scheduler_add_cfs_node(sched->cfs_scheduler, node_id, priority);
            
        case SCHED_FIFO:
        case SCHED_RR:
            return fair_scheduler_add_rt_node(sched->rt_scheduler, node_id, 
                                             policy, priority);
            
        case SCHED_DEADLINE:
            return fair_scheduler_add_dl_node(sched->dl_scheduler, node_id);
            
        default:
            fprintf(stderr, "Unknown scheduling policy: %d\n", policy);
            return -1;
    }
}

/**
 * @brief Remove a node from the scheduler
 */
int aeon_scheduler_remove_node(Scheduler* sched, NodeId node_id) {
    if (!sched || !sched->initialized) {
        return -1;
    }
    
    /* Try removing from all sub-schedulers */
    int result = 0;
    
    result |= fair_scheduler_remove_cfs_node(sched->cfs_scheduler, node_id);
    result |= fair_scheduler_remove_rt_node(sched->rt_scheduler, node_id);
    result |= fair_scheduler_remove_dl_node(sched->dl_scheduler, node_id);
    
    return result;
}

/**
 * @brief Main scheduling loop (multi-level scheduler)
 */
int aeon_scheduler_schedule(Scheduler* sched) {
    if (!sched || !sched->initialized) {
        return -1;
    }
    
    SchedState state = atomic_load(&sched->state);
    if (state != SCHED_STATE_RUNNING) {
        return 0; /* Not running */
    }
    
    /* Scheduler class hierarchy: Deadline > RT > CFS */
    
    /* 1. Try Deadline scheduler first (highest priority) */
    NodeId dl_node = fair_scheduler_pick_next_dl(sched->dl_scheduler);
    if (dl_node != 0) {
        atomic_fetch_add(&sched->stats.dl_scheduled, 1);
        atomic_fetch_add(&sched->stats.total_scheduled, 1);
        return device_scheduler_dispatch(sched->device_scheduler, dl_node);
    }
    
    /* 2. Try RT scheduler */
    NodeId rt_node = fair_scheduler_pick_next_rt(sched->rt_scheduler);
    if (rt_node != 0) {
        atomic_fetch_add(&sched->stats.rt_scheduled, 1);
        atomic_fetch_add(&sched->stats.total_scheduled, 1);
        return device_scheduler_dispatch(sched->device_scheduler, rt_node);
    }
    
    /* 3. Try CFS scheduler */
    NodeId cfs_node = fair_scheduler_pick_next_cfs(sched->cfs_scheduler);
    if (cfs_node != 0) {
        atomic_fetch_add(&sched->stats.cfs_scheduled, 1);
        atomic_fetch_add(&sched->stats.total_scheduled, 1);
        return device_scheduler_dispatch(sched->device_scheduler, cfs_node);
    }
    
    /* No tasks to schedule */
    return 0;
}

/**
 * @brief Scheduler tick handler
 */
void aeon_scheduler_tick(Scheduler* sched) {
    if (!sched || !sched->initialized) {
        return;
    }
    
    uint64_t current_time = sched->get_time_ns();
    atomic_fetch_add(&sched->tick_count, 1);
    
    /* Update time accounting for all sub-schedulers */
    fair_scheduler_tick_cfs(sched->cfs_scheduler, current_time);
    fair_scheduler_tick_rt(sched->rt_scheduler, current_time);
    fair_scheduler_tick_dl(sched->dl_scheduler, current_time);
    
    /* Check if load balancing is needed */
    uint64_t time_since_balance = current_time - sched->stats.last_balance_time;
    if (time_since_balance >= (sched->balance_interval_ms * 1000000ULL)) {
        aeon_scheduler_balance_load(sched);
        sched->stats.last_balance_time = current_time;
    }
    
    sched->last_tick_time = current_time;
}

/**
 * @brief Perform load balancing
 */
int aeon_scheduler_balance_load(Scheduler* sched) {
    if (!sched || !sched->initialized) {
        return -1;
    }
    
    /* Perform load balancing across devices */
    int result = device_scheduler_balance_load(sched->device_scheduler);
    
    if (result > 0) {
        atomic_fetch_add(&sched->stats.total_migrations, result);
    }
    
    return result;
}

/**
 * @brief Preempt current task on a device
 */
int aeon_scheduler_preempt(Scheduler* sched, DeviceId device_id) {
    if (!sched || !sched->initialized) {
        return -1;
    }
    
    atomic_fetch_add(&sched->stats.total_preemptions, 1);
    atomic_fetch_add(&sched->stats.total_context_switches, 1);
    
    return device_scheduler_preempt(sched->device_scheduler, device_id);
}

/**
 * @brief Get scheduler statistics
 */
int aeon_scheduler_get_stats(Scheduler* sched, SchedStatistics* stats) {
    if (!sched || !stats) {
        return -1;
    }
    
    /* Copy atomic statistics */
    stats->total_scheduled = atomic_load(&sched->stats.total_scheduled);
    stats->total_preemptions = atomic_load(&sched->stats.total_preemptions);
    stats->total_migrations = atomic_load(&sched->stats.total_migrations);
    stats->total_context_switches = atomic_load(&sched->stats.total_context_switches);
    stats->cfs_scheduled = atomic_load(&sched->stats.cfs_scheduled);
    stats->rt_scheduled = atomic_load(&sched->stats.rt_scheduled);
    stats->dl_scheduled = atomic_load(&sched->stats.dl_scheduled);
    stats->last_balance_time = sched->stats.last_balance_time;
    
    return 0;
}

/**
 * @brief Print scheduler statistics
 */
void aeon_scheduler_print_stats(Scheduler* sched) {
    if (!sched) return;
    
    SchedStatistics stats;
    aeon_scheduler_get_stats(sched, &stats);
    
    printf("\n=== Scheduler Statistics ===\n");
    printf("Total Scheduled:      %llu\n", (unsigned long long)stats.total_scheduled);
    printf("  CFS Scheduled:      %llu\n", (unsigned long long)stats.cfs_scheduled);
    printf("  RT Scheduled:       %llu\n", (unsigned long long)stats.rt_scheduled);
    printf("  DL Scheduled:       %llu\n", (unsigned long long)stats.dl_scheduled);
    printf("Total Preemptions:    %llu\n", (unsigned long long)stats.total_preemptions);
    printf("Total Migrations:     %llu\n", (unsigned long long)stats.total_migrations);
    printf("Total Context Switches: %llu\n", (unsigned long long)stats.total_context_switches);
    printf("Tick Count:           %llu\n", (unsigned long long)atomic_load(&sched->tick_count));
    printf("===========================\n\n");
}

/**
 * @brief Run scheduler for a single wave (original API - maintained for compatibility)
 */
int aeon_scheduler_run_wave(Scheduler* sched) {
    if (!sched) return -1;
    BdiGraph* g = sched->graph;
    
    /* Find ready nodes (simple version: all nodes with no inputs) */
    for (size_t i = 0; i < g->node_count; i++) {
        if (g->nodes[i].input_count == 0) {
            GraphNode* node = &g->nodes[i];
            
            /* Apply the Policy Gate */
            if (policy_gate_check(sched, node)) {
                /* Dispatch the node if it passes */
                DeviceId hint = node->device_hint > 0 ? node->device_hint : DEVICE_ID_CPU;
                DeviceVTable* device = sched->devices[hint];
                if (device) {
                    printf("SCHEDULER: Dispatching Node %llu to Device '%s'.\n",
                           (unsigned long long)node->id, device->name);
                    void* kernel;
                    device->lower(node, &kernel);
                    device->enqueue(kernel, nullptr, 0);
                }
            }
        }
    }
    
    /* Sync devices */
    for (size_t i = 1; i <= sched->device_count; i++) {
        if (sched->devices[i]) sched->devices[i]->sync();
    }
    
    return 0;
}
