
// ===================================================================
// DESC: Fairness scheduler implementation - CFS, RT, and Deadline
// Phase 9: Scheduler Integration & Fairness
// ===================================================================
#include "fairness.h"
#include "scheduler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

/* ===================================================================
 * CFS (Completely Fair Scheduler) Implementation
 * =================================================================== */

/**
 * @brief CFS Scheduler structure
 */
typedef struct {
    Scheduler* base_scheduler;
    
    /* Red-black tree for CFS (simplified as sorted array) */
    node_sched_info_t* tasks[SCHED_MAX_READY_NODES];
    uint32_t task_count;
    
    /* Virtual runtime tracking */
    uint64_t min_vruntime;
    
    /* Load tracking */
    uint32_t total_weight;
    uint32_t nr_running;
    
    /* Statistics */
    uint64_t total_scheduled;
} cfs_scheduler_t;

/**
 * @brief Create CFS scheduler
 */
void* fair_scheduler_create_cfs(Scheduler* base) {
    if (!base) return nullptr;
    
    cfs_scheduler_t* cfs = (cfs_scheduler_t*)calloc(1, sizeof(cfs_scheduler_t));
    if (!cfs) return nullptr;
    
    cfs->base_scheduler = base;
    cfs->min_vruntime = 0;
    cfs->total_weight = 0;
    cfs->nr_running = 0;
    cfs->task_count = 0;
    cfs->total_scheduled = 0;
    
    return cfs;
}

/**
 * @brief Destroy CFS scheduler
 */
void fair_scheduler_destroy_cfs(void* cfs_ptr) {
    if (!cfs_ptr) return;
    
    cfs_scheduler_t* cfs = (cfs_scheduler_t*)cfs_ptr;
    
    /* Free all task info structures */
    for (uint32_t i = 0; i < cfs->task_count; i++) {
        free(cfs->tasks[i]);
    }
    
    free(cfs);
}

/**
 * @brief Calculate time slice for a task
 */
static uint64_t cfs_calc_time_slice(cfs_scheduler_t* cfs, node_sched_info_t* task) {
    if (cfs->nr_running == 0) {
        return cfs->base_scheduler->min_granularity;
    }
    
    /* Time slice = (sched_latency * task_weight) / total_weight */
    uint64_t slice = (cfs->base_scheduler->sched_latency * task->weight) / cfs->total_weight;
    
    /* Ensure minimum granularity */
    if (slice < cfs->base_scheduler->min_granularity) {
        slice = cfs->base_scheduler->min_granularity;
    }
    
    return slice;
}

/**
 * @brief Calculate virtual runtime delta
 */
static uint64_t cfs_calc_delta_fair(uint64_t delta, node_sched_info_t* task) {
    if (task->weight == 1024) {
        return delta; /* Default weight, no adjustment */
    }
    
    /* vruntime_delta = (delta * NICE_0_LOAD) / task_weight */
    return (delta * 1024) / task->weight;
}

/**
 * @brief Update virtual runtime for a task
 */
static void cfs_update_vruntime(cfs_scheduler_t* cfs, node_sched_info_t* task, uint64_t delta) {
    uint64_t vdelta = cfs_calc_delta_fair(delta, task);
    task->vruntime += vdelta;
    task->runtime += delta;
    task->total_runtime += delta;
}

/**
 * @brief Insert task into CFS queue (sorted by vruntime)
 */
static int cfs_enqueue_task_internal(cfs_scheduler_t* cfs, node_sched_info_t* task) {
    if (cfs->task_count >= SCHED_MAX_READY_NODES) {
        return -1;
    }
    
    /* Set initial vruntime if new task */
    if (task->vruntime == 0) {
        task->vruntime = cfs->min_vruntime;
    }
    
    /* Find insertion point (keep sorted by vruntime) */
    uint32_t insert_pos = cfs->task_count;
    for (uint32_t i = 0; i < cfs->task_count; i++) {
        if (task->vruntime < cfs->tasks[i]->vruntime) {
            insert_pos = i;
            break;
        }
    }
    
    /* Shift tasks to make room */
    for (uint32_t i = cfs->task_count; i > insert_pos; i--) {
        cfs->tasks[i] = cfs->tasks[i - 1];
    }
    
    /* Insert task */
    cfs->tasks[insert_pos] = task;
    cfs->task_count++;
    
    /* Update load */
    cfs->total_weight += task->weight;
    cfs->nr_running++;
    
    return 0;
}

/**
 * @brief Remove task from CFS queue
 */
static int cfs_dequeue_task_internal(cfs_scheduler_t* cfs, node_sched_info_t* task) {
    /* Find task */
    uint32_t task_pos = UINT32_MAX;
    for (uint32_t i = 0; i < cfs->task_count; i++) {
        if (cfs->tasks[i]->node_id == task->node_id) {
            task_pos = i;
            break;
        }
    }
    
    if (task_pos == UINT32_MAX) {
        return -1; /* Task not found */
    }
    
    /* Shift tasks */
    for (uint32_t i = task_pos; i < cfs->task_count - 1; i++) {
        cfs->tasks[i] = cfs->tasks[i + 1];
    }
    
    cfs->task_count--;
    
    /* Update load */
    cfs->total_weight -= task->weight;
    cfs->nr_running--;
    
    return 0;
}

/**
 * @brief Pick next task from CFS queue
 */
/**
 * @brief Requeue a task after vruntime update (maintains sorted order)
 */
static void cfs_requeue_task(cfs_scheduler_t* cfs, node_sched_info_t* task) {
    /* Find current position */
    uint32_t old_pos = UINT32_MAX;
    for (uint32_t i = 0; i < cfs->task_count; i++) {
        if (cfs->tasks[i] == task) {
            old_pos = i;
            break;
        }
    }
    
    if (old_pos == UINT32_MAX) {
        return; /* Task not in queue */
    }
    
    /* Find new position based on updated vruntime */
    uint32_t new_pos = old_pos;
    
    /* Check if we need to move forward (vruntime increased) */
    for (uint32_t i = old_pos + 1; i < cfs->task_count; i++) {
        if (task->vruntime > cfs->tasks[i]->vruntime) {
            new_pos = i;
        } else {
            break;
        }
    }
    
    /* Only reposition if position changed */
    if (new_pos != old_pos) {
        /* Remove from old position */
        node_sched_info_t* temp = cfs->tasks[old_pos];
        
        /* Shift tasks between old and new position */
        for (uint32_t i = old_pos; i < new_pos; i++) {
            cfs->tasks[i] = cfs->tasks[i + 1];
        }
        
        /* Insert at new position */
        cfs->tasks[new_pos] = temp;
    }
}

static node_sched_info_t* cfs_pick_next_task_internal(cfs_scheduler_t* cfs) {
    if (cfs->task_count == 0) {
        return nullptr;
    }
    
    /* Pick task with minimum vruntime (first in sorted array) */
    node_sched_info_t* task = cfs->tasks[0];
    
    /* Update min_vruntime */
    cfs->min_vruntime = task->vruntime;
    
    return task;
}

/**
 * @brief Add node to CFS scheduler
 */
int fair_scheduler_add_cfs_node(void* cfs_ptr, NodeId node_id, int32_t priority) {
    if (!cfs_ptr) return -1;
    
    cfs_scheduler_t* cfs = (cfs_scheduler_t*)cfs_ptr;
    
    /* Create task info */
    node_sched_info_t* task = (node_sched_info_t*)calloc(1, sizeof(node_sched_info_t));
    if (!task) return -1;
    
    task->node_id = node_id;
    task->latency_class = LATENCY_CLASS_NORMAL;
    task->policy = SCHED_POLICY_CFS;
    task->priority = priority;
    task->weight = aeon_scheduler_prio_to_weight(priority);
    task->nice_value = priority + 20; /* Convert to 0-39 range */
    task->vruntime = 0;
    task->runtime = 0;
    task->total_runtime = 0;
    task->wait_time = 0;
    task->execution_count = 0;
    task->is_running = false;
    task->is_preemptible = true;
    task->arrival_time = cfs->base_scheduler->get_time_ns();
    
    /* Enqueue task */
    int result = cfs_enqueue_task_internal(cfs, task);
    if (result != 0) {
        free(task);
        return result;
    }
    
    return 0;
}

/**
 * @brief Remove node from CFS scheduler
 */
int fair_scheduler_remove_cfs_node(void* cfs_ptr, NodeId node_id) {
    if (!cfs_ptr) return -1;
    
    cfs_scheduler_t* cfs = (cfs_scheduler_t*)cfs_ptr;
    
    /* Find and remove task */
    for (uint32_t i = 0; i < cfs->task_count; i++) {
        if (cfs->tasks[i]->node_id == node_id) {
            node_sched_info_t* task = cfs->tasks[i];
            cfs_dequeue_task_internal(cfs, task);
            free(task);
            return 0;
        }
    }
    
    return -1; /* Not found */
}

/**
 * @brief Pick next CFS task
 */
NodeId fair_scheduler_pick_next_cfs(void* cfs_ptr) {
    if (!cfs_ptr) return 0;
    
    cfs_scheduler_t* cfs = (cfs_scheduler_t*)cfs_ptr;
    
    node_sched_info_t* task = cfs_pick_next_task_internal(cfs);
    if (!task) return 0;
    
    /* Mark as running */
    task->is_running = true;
    task->execution_count++;
    cfs->total_scheduled++;
    
    return task->node_id;
}

/**
 * @brief CFS tick handler
 */
void fair_scheduler_tick_cfs(void* cfs_ptr, uint64_t current_time) {
    if (!cfs_ptr) return;
    
    cfs_scheduler_t* cfs = (cfs_scheduler_t*)cfs_ptr;
    
    /* Update vruntime for running tasks and requeue if time slice expired */
    for (uint32_t i = 0; i < cfs->task_count; i++) {
        node_sched_info_t* task = cfs->tasks[i];
        if (task->is_running) {
            /* Assume 1ms tick */
            cfs_update_vruntime(cfs, task, 1000000);
            
            /* Check if time slice expired (simplified: 10ms slice) */
            if (task->runtime >= 10000000) {
                /* Time slice expired - mark as not running and requeue */
                task->is_running = false;
                task->runtime = 0;
                
                /* Reinsert task in sorted order by vruntime */
                cfs_requeue_task(cfs, task);
            }
        }
    }
}

/* ===================================================================
 * Real-Time Scheduler Implementation
 * =================================================================== */

/**
 * @brief RT Scheduler structure
 */
typedef struct {
    Scheduler* base_scheduler;
    
    /* Priority queues (one per priority level 0-99) */
    node_sched_info_t* rt_queues[SCHED_MAX_RT_PRIO][256];
    uint32_t rt_counts[SCHED_MAX_RT_PRIO];
    
    /* Active priority bitmap */
    uint32_t active_bitmap[4]; /* 100 bits = 4 x 32-bit words */
    
    /* Round-robin time quantum */
    uint64_t rr_time_quantum;
    
    /* Statistics */
    uint64_t total_scheduled;
} rt_scheduler_t;

/**
 * @brief Create RT scheduler
 */
void* fair_scheduler_create_rt(Scheduler* base) {
    if (!base) return nullptr;
    
    rt_scheduler_t* rt = (rt_scheduler_t*)calloc(1, sizeof(rt_scheduler_t));
    if (!rt) return nullptr;
    
    rt->base_scheduler = base;
    rt->rr_time_quantum = 100000000; /* 100ms default */
    rt->total_scheduled = 0;
    
    /* Initialize queues */
    memset(rt->rt_counts, 0, sizeof(rt->rt_counts));
    memset(rt->active_bitmap, 0, sizeof(rt->active_bitmap));
    
    return rt;
}

/**
 * @brief Destroy RT scheduler
 */
void fair_scheduler_destroy_rt(void* rt_ptr) {
    if (!rt_ptr) return;
    
    rt_scheduler_t* rt = (rt_scheduler_t*)rt_ptr;
    
    /* Free all task info structures */
    for (uint32_t prio = 0; prio < SCHED_MAX_RT_PRIO; prio++) {
        for (uint32_t i = 0; i < rt->rt_counts[prio]; i++) {
            free(rt->rt_queues[prio][i]);
        }
    }
    
    free(rt);
}

/**
 * @brief Set priority bit in bitmap
 */
static void rt_set_priority_bit(rt_scheduler_t* rt, uint32_t prio) {
    uint32_t word = prio / 32;
    uint32_t bit = prio % 32;
    rt->active_bitmap[word] |= (1U << bit);
}

/**
 * @brief Clear priority bit in bitmap
 */
static void rt_clear_priority_bit(rt_scheduler_t* rt, uint32_t prio) {
    uint32_t word = prio / 32;
    uint32_t bit = prio % 32;
    rt->active_bitmap[word] &= ~(1U << bit);
}

/**
 * @brief Find highest priority with tasks
 */
static int rt_find_highest_priority(rt_scheduler_t* rt) {
    /* Scan bitmap from highest priority (0) to lowest (99) */
    for (int prio = 0; prio < SCHED_MAX_RT_PRIO; prio++) {
        uint32_t word = prio / 32;
        uint32_t bit = prio % 32;
        if (rt->active_bitmap[word] & (1U << bit)) {
            return prio;
        }
    }
    return -1;
}

/**
 * @brief Add node to RT scheduler
 */
int fair_scheduler_add_rt_node(void* rt_ptr, NodeId node_id, 
                               SchedPolicy policy, int32_t priority) {
    if (!rt_ptr) return -1;
    
    rt_scheduler_t* rt = (rt_scheduler_t*)rt_ptr;
    
    /* Clamp priority to RT range */
    if (priority < 0) priority = 0;
    if (priority >= SCHED_MAX_RT_PRIO) priority = SCHED_MAX_RT_PRIO - 1;
    
    /* Check queue capacity */
    if (rt->rt_counts[priority] >= 256) {
        return -1;
    }
    
    /* Create task info */
    node_sched_info_t* task = (node_sched_info_t*)calloc(1, sizeof(node_sched_info_t));
    if (!task) return -1;
    
    task->node_id = node_id;
    task->latency_class = LATENCY_CLASS_REALTIME;
    task->policy = policy;
    task->priority = priority;
    task->weight = 0; /* RT tasks don't use weight */
    task->is_running = false;
    task->is_preemptible = (policy == SCHED_RR);
    task->arrival_time = rt->base_scheduler->get_time_ns();
    
    /* Add to priority queue */
    rt->rt_queues[priority][rt->rt_counts[priority]] = task;
    rt->rt_counts[priority]++;
    
    /* Set priority bit */
    rt_set_priority_bit(rt, priority);
    
    return 0;
}

/**
 * @brief Remove node from RT scheduler
 */
int fair_scheduler_remove_rt_node(void* rt_ptr, NodeId node_id) {
    if (!rt_ptr) return -1;
    
    rt_scheduler_t* rt = (rt_scheduler_t*)rt_ptr;
    
    /* Search all priority queues */
    for (uint32_t prio = 0; prio < SCHED_MAX_RT_PRIO; prio++) {
        for (uint32_t i = 0; i < rt->rt_counts[prio]; i++) {
            if (rt->rt_queues[prio][i]->node_id == node_id) {
                node_sched_info_t* task = rt->rt_queues[prio][i];
                
                /* Shift remaining tasks */
                for (uint32_t j = i; j < rt->rt_counts[prio] - 1; j++) {
                    rt->rt_queues[prio][j] = rt->rt_queues[prio][j + 1];
                }
                
                rt->rt_counts[prio]--;
                
                /* Clear priority bit if queue is empty */
                if (rt->rt_counts[prio] == 0) {
                    rt_clear_priority_bit(rt, prio);
                }
                
                free(task);
                return 0;
            }
        }
    }
    
    return -1; /* Not found */
}

/**
 * @brief Pick next RT task
 */
NodeId fair_scheduler_pick_next_rt(void* rt_ptr) {
    if (!rt_ptr) return 0;
    
    rt_scheduler_t* rt = (rt_scheduler_t*)rt_ptr;
    
    /* Find highest priority with tasks */
    int prio = rt_find_highest_priority(rt);
    if (prio < 0) return 0;
    
    /* Get first task from queue (FIFO) */
    if (rt->rt_counts[prio] == 0) return 0;
    
    node_sched_info_t* task = rt->rt_queues[prio][0];
    task->is_running = true;
    task->execution_count++;
    rt->total_scheduled++;
    
    /* For SCHED_RR, move task to end of queue after execution */
    if (task->policy == SCHED_RR) {
        /* Rotate queue */
        for (uint32_t i = 0; i < rt->rt_counts[prio] - 1; i++) {
            rt->rt_queues[prio][i] = rt->rt_queues[prio][i + 1];
        }
        rt->rt_queues[prio][rt->rt_counts[prio] - 1] = task;
    }
    
    return task->node_id;
}

/**
 * @brief RT tick handler
 */
void fair_scheduler_tick_rt(void* rt_ptr, uint64_t current_time) {
    if (!rt_ptr) return;
    
    /* RT tasks run to completion or until preempted */
    /* Time accounting is handled by the main scheduler */
}

/* ===================================================================
 * Deadline Scheduler Implementation
 * =================================================================== */

/**
 * @brief Deadline Scheduler structure
 */
typedef struct {
    Scheduler* base_scheduler;
    
    /* Deadline queue (sorted by deadline) */
    node_sched_info_t* dl_queue[256];
    uint32_t dl_count;
    
    /* Statistics */
    uint64_t total_scheduled;
    uint64_t deadline_misses;
} dl_scheduler_t;

/**
 * @brief Create Deadline scheduler
 */
void* fair_scheduler_create_deadline(Scheduler* base) {
    if (!base) return nullptr;
    
    dl_scheduler_t* dl = (dl_scheduler_t*)calloc(1, sizeof(dl_scheduler_t));
    if (!dl) return nullptr;
    
    dl->base_scheduler = base;
    dl->dl_count = 0;
    dl->total_scheduled = 0;
    dl->deadline_misses = 0;
    
    return dl;
}

/**
 * @brief Destroy Deadline scheduler
 */
void fair_scheduler_destroy_deadline(void* dl_ptr) {
    if (!dl_ptr) return;
    
    dl_scheduler_t* dl = (dl_scheduler_t*)dl_ptr;
    
    /* Free all task info structures */
    for (uint32_t i = 0; i < dl->dl_count; i++) {
        free(dl->dl_queue[i]);
    }
    
    free(dl);
}

/**
 * @brief Add node to Deadline scheduler
 */
int fair_scheduler_add_dl_node(void* dl_ptr, NodeId node_id) {
    if (!dl_ptr) return -1;
    
    dl_scheduler_t* dl = (dl_scheduler_t*)dl_ptr;
    
    if (dl->dl_count >= 256) {
        return -1;
    }
    
    /* Create task info */
    node_sched_info_t* task = (node_sched_info_t*)calloc(1, sizeof(node_sched_info_t));
    if (!task) return -1;
    
    task->node_id = node_id;
    task->latency_class = LATENCY_CLASS_REALTIME;
    task->policy = SCHED_POLICY_DEADLINE;
    task->is_running = false;
    task->is_preemptible = true;
    task->arrival_time = dl->base_scheduler->get_time_ns();
    
    /* Set deadline (example: 10ms from now) */
    task->deadline = task->arrival_time + 10000000;
    task->period = 10000000; /* 10ms period */
    task->runtime = 0;
    
    /* Insert sorted by deadline (EDF - Earliest Deadline First) */
    uint32_t insert_pos = dl->dl_count;
    for (uint32_t i = 0; i < dl->dl_count; i++) {
        if (task->deadline < dl->dl_queue[i]->deadline) {
            insert_pos = i;
            break;
        }
    }
    
    /* Shift tasks */
    for (uint32_t i = dl->dl_count; i > insert_pos; i--) {
        dl->dl_queue[i] = dl->dl_queue[i - 1];
    }
    
    dl->dl_queue[insert_pos] = task;
    dl->dl_count++;
    
    return 0;
}

/**
 * @brief Remove node from Deadline scheduler
 */
int fair_scheduler_remove_dl_node(void* dl_ptr, NodeId node_id) {
    if (!dl_ptr) return -1;
    
    dl_scheduler_t* dl = (dl_scheduler_t*)dl_ptr;
    
    /* Find and remove task */
    for (uint32_t i = 0; i < dl->dl_count; i++) {
        if (dl->dl_queue[i]->node_id == node_id) {
            node_sched_info_t* task = dl->dl_queue[i];
            
            /* Shift tasks */
            for (uint32_t j = i; j < dl->dl_count - 1; j++) {
                dl->dl_queue[j] = dl->dl_queue[j + 1];
            }
            
            dl->dl_count--;
            free(task);
            return 0;
        }
    }
    
    return -1; /* Not found */
}

/**
 * @brief Pick next Deadline task
 */
NodeId fair_scheduler_pick_next_dl(void* dl_ptr) {
    if (!dl_ptr) return 0;
    
    dl_scheduler_t* dl = (dl_scheduler_t*)dl_ptr;
    
    if (dl->dl_count == 0) return 0;
    
    /* Pick task with earliest deadline (first in sorted queue) */
    node_sched_info_t* task = dl->dl_queue[0];
    
    /* Check for deadline miss */
    uint64_t current_time = dl->base_scheduler->get_time_ns();
    if (current_time > task->deadline) {
        dl->deadline_misses++;
        printf("WARNING: Deadline miss for node %llu\n", 
               (unsigned long long)task->node_id);
    }
    
    task->is_running = true;
    task->execution_count++;
    dl->total_scheduled++;
    
    return task->node_id;
}

/**
 * @brief Deadline tick handler
 */
void fair_scheduler_tick_dl(void* dl_ptr, uint64_t current_time) {
    if (!dl_ptr) return;
    
    dl_scheduler_t* dl = (dl_scheduler_t*)dl_ptr;
    
    /* Check for deadline misses */
    for (uint32_t i = 0; i < dl->dl_count; i++) {
        node_sched_info_t* task = dl->dl_queue[i];
        if (!task->is_running && current_time > task->deadline) {
            dl->deadline_misses++;
        }
    }
}
