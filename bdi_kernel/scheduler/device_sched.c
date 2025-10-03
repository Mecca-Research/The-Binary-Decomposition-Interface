
// ===================================================================
// DESC: Device scheduler implementation - Multi-device scheduling
// Phase 9: Scheduler Integration & Fairness
// ===================================================================
#include "fairness.h"
#include "scheduler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* ===================================================================
 * Device Types and Structures
 * =================================================================== */

/**
 * @brief Device types
 */
typedef enum {
    DEVICE_TYPE_CPU = 0,
    DEVICE_TYPE_GPU = 1,
    DEVICE_TYPE_FPGA = 2,
    DEVICE_TYPE_BPU = 3,    /* Binary Processing Unit */
    DEVICE_TYPE_COUNT = 4
} DeviceType;

/**
 * @brief Per-device run queue
 */
typedef struct {
    DeviceId device_id;
    DeviceType device_type;
    
    /* Task queue */
    NodeId task_queue[1024];
    uint32_t queue_head;
    uint32_t queue_tail;
    uint32_t queue_count;
    
    /* Current running task */
    NodeId current_task;
    uint64_t current_start_time;
    
    /* Load tracking */
    uint32_t load_weight;
    uint32_t nr_running;
    
    /* Device capabilities */
    uint32_t capability_mask;
    bool is_online;
    
    /* Statistics */
    uint64_t total_runtime;
    uint64_t idle_time;
    uint32_t tasks_executed;
} device_runqueue_t;

/**
 * @brief Device Scheduler structure
 */
typedef struct {
    Scheduler* base_scheduler;
    
    /* Per-device run queues */
    device_runqueue_t device_queues[SCHED_MAX_DEVICES];
    uint32_t num_devices;
    
    /* Device affinity map (node_id -> device_mask) */
    uint32_t affinity_map[SCHED_MAX_READY_NODES];
    
    /* Load balancing */
    uint64_t last_balance_time;
    uint32_t balance_threshold;
    
    /* Statistics */
    uint64_t total_dispatched;
    uint64_t total_migrations;
} device_scheduler_t;

/* ===================================================================
 * Device Scheduler Implementation
 * =================================================================== */

/**
 * @brief Create device scheduler
 */
void* device_scheduler_create(Scheduler* base) {
    if (!base) return nullptr;
    
    device_scheduler_t* ds = (device_scheduler_t*)calloc(1, sizeof(device_scheduler_t));
    if (!ds) return nullptr;
    
    ds->base_scheduler = base;
    ds->num_devices = base->device_count;
    ds->balance_threshold = 10; /* Load difference threshold */
    ds->total_dispatched = 0;
    ds->total_migrations = 0;
    
    /* Initialize device queues */
    for (uint32_t i = 0; i < ds->num_devices && i < SCHED_MAX_DEVICES; i++) {
        device_runqueue_t* rq = &ds->device_queues[i];
        
        rq->device_id = i + 1; /* Device IDs start at 1 */
        rq->device_type = DEVICE_TYPE_CPU; /* Default to CPU */
        rq->queue_head = 0;
        rq->queue_tail = 0;
        rq->queue_count = 0;
        rq->current_task = 0;
        rq->current_start_time = 0;
        rq->load_weight = 0;
        rq->nr_running = 0;
        rq->capability_mask = 0xFFFFFFFF; /* All capabilities */
        rq->is_online = true;
        rq->total_runtime = 0;
        rq->idle_time = 0;
        rq->tasks_executed = 0;
    }
    
    /* Initialize affinity map (all nodes can run on all devices) */
    memset(ds->affinity_map, 0xFF, sizeof(ds->affinity_map));
    
    ds->last_balance_time = base->get_time_ns();
    
    return ds;
}

/**
 * @brief Destroy device scheduler
 */
void device_scheduler_destroy(void* ds_ptr) {
    if (!ds_ptr) return;
    free(ds_ptr);
}

/**
 * @brief Find least loaded device
 */
static DeviceId device_find_least_loaded(device_scheduler_t* ds, uint32_t affinity_mask) {
    DeviceId best_device = 0;
    uint32_t min_load = UINT32_MAX;
    
    for (uint32_t i = 0; i < ds->num_devices; i++) {
        device_runqueue_t* rq = &ds->device_queues[i];
        
        /* Check if device is online and allowed by affinity */
        if (!rq->is_online) continue;
        if (!(affinity_mask & (1U << i))) continue;
        
        /* Find device with minimum load */
        if (rq->load_weight < min_load) {
            min_load = rq->load_weight;
            best_device = rq->device_id;
        }
    }
    
    return best_device;
}

/**
 * @brief Enqueue task to device
 */
static int device_enqueue_task(device_runqueue_t* rq, NodeId node_id) {
    if (rq->queue_count >= 1024) {
        return -1; /* Queue full */
    }
    
    rq->task_queue[rq->queue_tail] = node_id;
    rq->queue_tail = (rq->queue_tail + 1) % 1024;
    rq->queue_count++;
    rq->load_weight++;
    rq->nr_running++;
    
    return 0;
}

/**
 * @brief Dequeue task from device
 */
static NodeId device_dequeue_task(device_runqueue_t* rq) {
    if (rq->queue_count == 0) {
        return 0;
    }
    
    NodeId node_id = rq->task_queue[rq->queue_head];
    rq->queue_head = (rq->queue_head + 1) % 1024;
    rq->queue_count--;
    
    if (rq->load_weight > 0) {
        rq->load_weight--;
    }
    if (rq->nr_running > 0) {
        rq->nr_running--;
    }
    
    return node_id;
}

/**
 * @brief Dispatch task to device
 */
int device_scheduler_dispatch(void* ds_ptr, NodeId node_id) {
    if (!ds_ptr || node_id == 0) return -1;
    
    device_scheduler_t* ds = (device_scheduler_t*)ds_ptr;
    
    /* Get affinity mask for node */
    uint32_t affinity_mask = 0xFFFFFFFF; /* Default: all devices */
    if (node_id < SCHED_MAX_READY_NODES) {
        affinity_mask = ds->affinity_map[node_id];
    }
    
    /* Find best device */
    DeviceId device_id = device_find_least_loaded(ds, affinity_mask);
    if (device_id == 0) {
        fprintf(stderr, "No suitable device found for node %llu\n", 
                (unsigned long long)node_id);
        return -1;
    }
    
    /* Get device queue */
    device_runqueue_t* rq = nullptr;
    for (uint32_t i = 0; i < ds->num_devices; i++) {
        if (ds->device_queues[i].device_id == device_id) {
            rq = &ds->device_queues[i];
            break;
        }
    }
    
    if (!rq) return -1;
    
    /* Enqueue task */
    int result = device_enqueue_task(rq, node_id);
    if (result != 0) {
        return result;
    }
    
    ds->total_dispatched++;
    
    /* Execute task immediately if device is idle */
    if (rq->current_task == 0) {
        NodeId task = device_dequeue_task(rq);
        if (task != 0) {
            rq->current_task = task;
            rq->current_start_time = ds->base_scheduler->get_time_ns();
            rq->tasks_executed++;
            
            printf("DEVICE_SCHED: Dispatching node %llu to device %u\n",
                   (unsigned long long)task, device_id);
        }
    }
    
    return 0;
}

/**
 * @brief Preempt current task on device
 */
int device_scheduler_preempt(void* ds_ptr, DeviceId device_id) {
    if (!ds_ptr || device_id == 0) return -1;
    
    device_scheduler_t* ds = (device_scheduler_t*)ds_ptr;
    
    /* Find device queue */
    device_runqueue_t* rq = nullptr;
    for (uint32_t i = 0; i < ds->num_devices; i++) {
        if (ds->device_queues[i].device_id == device_id) {
            rq = &ds->device_queues[i];
            break;
        }
    }
    
    if (!rq || rq->current_task == 0) {
        return -1; /* No task running */
    }
    
    /* Re-enqueue current task */
    NodeId preempted_task = rq->current_task;
    rq->current_task = 0;
    
    /* Update runtime */
    uint64_t current_time = ds->base_scheduler->get_time_ns();
    uint64_t runtime = current_time - rq->current_start_time;
    rq->total_runtime += runtime;
    
    /* Re-enqueue preempted task */
    device_enqueue_task(rq, preempted_task);
    
    /* Pick next task */
    NodeId next_task = device_dequeue_task(rq);
    if (next_task != 0) {
        rq->current_task = next_task;
        rq->current_start_time = current_time;
        rq->tasks_executed++;
    }
    
    return 0;
}

/**
 * @brief Balance load across devices
 */
int device_scheduler_balance_load(void* ds_ptr) {
    if (!ds_ptr) return -1;
    
    device_scheduler_t* ds = (device_scheduler_t*)ds_ptr;
    
    /* Find most and least loaded devices */
    uint32_t max_load = 0;
    uint32_t min_load = UINT32_MAX;
    device_runqueue_t* max_rq = nullptr;
    device_runqueue_t* min_rq = nullptr;
    
    for (uint32_t i = 0; i < ds->num_devices; i++) {
        device_runqueue_t* rq = &ds->device_queues[i];
        if (!rq->is_online) continue;
        
        if (rq->load_weight > max_load) {
            max_load = rq->load_weight;
            max_rq = rq;
        }
        if (rq->load_weight < min_load) {
            min_load = rq->load_weight;
            min_rq = rq;
        }
    }
    
    /* Check if balancing is needed */
    if (!max_rq || !min_rq || max_rq == min_rq) {
        return 0;
    }
    
    uint32_t load_diff = max_load - min_load;
    if (load_diff < ds->balance_threshold) {
        return 0; /* Load is balanced */
    }
    
    /* Migrate tasks from max to min */
    int migrations = 0;
    uint32_t tasks_to_migrate = load_diff / 2;
    
    for (uint32_t i = 0; i < tasks_to_migrate && max_rq->queue_count > 0; i++) {
        NodeId task = device_dequeue_task(max_rq);
        if (task != 0) {
            device_enqueue_task(min_rq, task);
            migrations++;
            ds->total_migrations++;
        }
    }
    
    if (migrations > 0) {
        printf("DEVICE_SCHED: Migrated %d tasks from device %u to device %u\n",
               migrations, max_rq->device_id, min_rq->device_id);
    }
    
    return migrations;
}

/**
 * @brief Set device affinity for a node
 */
int device_scheduler_set_affinity(void* ds_ptr, NodeId node_id, uint32_t device_mask) {
    if (!ds_ptr || node_id == 0 || node_id >= SCHED_MAX_READY_NODES) {
        return -1;
    }
    
    device_scheduler_t* ds = (device_scheduler_t*)ds_ptr;
    ds->affinity_map[node_id] = device_mask;
    
    return 0;
}

/**
 * @brief Get device statistics
 */
void device_scheduler_print_stats(void* ds_ptr) {
    if (!ds_ptr) return;
    
    device_scheduler_t* ds = (device_scheduler_t*)ds_ptr;
    
    printf("\n=== Device Scheduler Statistics ===\n");
    printf("Total Dispatched:     %llu\n", (unsigned long long)ds->total_dispatched);
    printf("Total Migrations:     %llu\n", (unsigned long long)ds->total_migrations);
    printf("\nPer-Device Statistics:\n");
    
    for (uint32_t i = 0; i < ds->num_devices; i++) {
        device_runqueue_t* rq = &ds->device_queues[i];
        printf("  Device %u:\n", rq->device_id);
        printf("    Online:           %s\n", rq->is_online ? "Yes" : "No");
        printf("    Queue Count:      %u\n", rq->queue_count);
        printf("    Load Weight:      %u\n", rq->load_weight);
        printf("    Running Tasks:    %u\n", rq->nr_running);
        printf("    Tasks Executed:   %u\n", rq->tasks_executed);
        printf("    Total Runtime:    %llu ns\n", (unsigned long long)rq->total_runtime);
    }
    printf("===================================\n\n");
}
