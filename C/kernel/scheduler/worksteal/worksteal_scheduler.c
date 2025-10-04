
// ===================================================================
// Phase 5.3: Work Stealing Scheduler Implementation
// ===================================================================
#include "worksteal_scheduler.h"
#include <stdlib.h>
#include <string.h>

// --- Lock-Free Queue Implementation ---

LockFreeQueue* queue_create(void) {
    LockFreeQueue* queue = malloc(sizeof(LockFreeQueue));
    if (!queue) return NULL;
    
    atomic_init(&queue->head, 0);
    atomic_init(&queue->tail, 0);
    memset(queue->items, 0, sizeof(queue->items));
    
    return queue;
}

void queue_free(LockFreeQueue* queue) {
    free(queue);
}

bool queue_push(LockFreeQueue* queue, NodeId node_id) {
    if (!queue) return false;
    
    size_t tail = atomic_load(&queue->tail);
    size_t next_tail = (tail + 1) % QUEUE_SIZE;
    size_t head = atomic_load(&queue->head);
    
    if (next_tail == head) {
        return false;  // Queue full
    }
    
    queue->items[tail] = node_id;
    atomic_store(&queue->tail, next_tail);
    
    return true;
}

bool queue_pop(LockFreeQueue* queue, NodeId* out_node_id) {
    if (!queue || !out_node_id) return false;
    
    size_t tail = atomic_load(&queue->tail);
    
    if (tail == 0) {
        tail = QUEUE_SIZE;
    }
    tail--;
    
    size_t head = atomic_load(&queue->head);
    
    if (tail < head) {
        return false;  // Queue empty
    }
    
    *out_node_id = queue->items[tail];
    atomic_store(&queue->tail, tail);
    
    return true;
}

bool queue_steal(LockFreeQueue* queue, NodeId* out_node_id) {
    if (!queue || !out_node_id) return false;
    
    size_t head = atomic_load(&queue->head);
    size_t tail = atomic_load(&queue->tail);
    
    if (head >= tail) {
        return false;  // Queue empty
    }
    
    *out_node_id = queue->items[head];
    atomic_store(&queue->head, head + 1);
    
    return true;
}

// --- Worker Thread ---

typedef struct {
    WorkStealingScheduler* sched;
    size_t worker_id;
} WorkerThreadArg;

bool steal_work(WorkStealingScheduler* sched, size_t worker_id, NodeId* out_node_id) {
    if (!sched || !out_node_id) return false;
    
    // Try to steal from other workers
    for (size_t i = 0; i < sched->num_workers; i++) {
        if (i == worker_id) continue;
        
        if (queue_steal(sched->remote_queues[i], out_node_id)) {
            return true;
        }
    }
    
    return false;
}

int worker_thread(void* arg) {
    WorkerThreadArg* warg = (WorkerThreadArg*)arg;
    WorkStealingScheduler* sched = warg->sched;
    size_t worker_id = warg->worker_id;
    
    LockFreeQueue* local_queue = sched->remote_queues[worker_id];
    
    while (atomic_load(&sched->running)) {
        NodeId node_id;
        
        // Try to pop from local queue
        if (queue_pop(local_queue, &node_id)) {
            // Execute node
            if (node_id < sched->graph->node_count) {
                const GraphNode* node = &sched->graph->nodes[node_id];
                DeviceVTable* device = sched->devices[worker_id % sched->device_count];
                
                void* kernel = NULL;
                if (device->lower(node, &kernel) == 0) {
                    device->enqueue(kernel, NULL, 0);
                    free(kernel);
                }
            }
        }
        // Try to steal work
        else if (steal_work(sched, worker_id, &node_id)) {
            // Execute stolen node
            if (node_id < sched->graph->node_count) {
                const GraphNode* node = &sched->graph->nodes[node_id];
                DeviceVTable* device = sched->devices[worker_id % sched->device_count];
                
                void* kernel = NULL;
                if (device->lower(node, &kernel) == 0) {
                    device->enqueue(kernel, NULL, 0);
                    free(kernel);
                }
            }
        }
        else {
            // No work available, yield
            thrd_yield();
        }
    }
    
    free(warg);
    return 0;
}

// --- Work Stealing Scheduler ---

WorkStealingScheduler* worksteal_scheduler_create(BdiGraph* graph, DeviceVTable** devices, size_t device_count, size_t num_workers) {
    if (!graph || !devices || num_workers == 0) return NULL;
    
    WorkStealingScheduler* sched = malloc(sizeof(WorkStealingScheduler));
    if (!sched) return NULL;
    
    sched->graph = graph;
    sched->devices = devices;
    sched->device_count = device_count;
    sched->num_workers = num_workers;
    atomic_init(&sched->running, false);
    
    // Create queues
    sched->local_queue = queue_create();
    sched->remote_queues = malloc(sizeof(LockFreeQueue*) * num_workers);
    
    for (size_t i = 0; i < num_workers; i++) {
        sched->remote_queues[i] = queue_create();
    }
    
    // Create worker threads
    sched->worker_threads = malloc(sizeof(thrd_t) * num_workers);
    
    return sched;
}

void worksteal_scheduler_free(WorkStealingScheduler* sched) {
    if (!sched) return;
    
    queue_free(sched->local_queue);
    
    for (size_t i = 0; i < sched->num_workers; i++) {
        queue_free(sched->remote_queues[i]);
    }
    
    free(sched->remote_queues);
    free(sched->worker_threads);
    free(sched);
}

int worksteal_scheduler_run(WorkStealingScheduler* sched) {
    if (!sched) return -1;
    
    atomic_store(&sched->running, true);
    
    // Populate initial work
    for (size_t i = 0; i < sched->graph->node_count; i++) {
        size_t worker_id = i % sched->num_workers;
        queue_push(sched->remote_queues[worker_id], sched->graph->nodes[i].id);
    }
    
    // Start worker threads
    for (size_t i = 0; i < sched->num_workers; i++) {
        WorkerThreadArg* arg = malloc(sizeof(WorkerThreadArg));
        arg->sched = sched;
        arg->worker_id = i;
        
        thrd_create(&sched->worker_threads[i], worker_thread, arg);
    }
    
    // Signal workers to stop
    atomic_store(&sched->running, false);

    // Wait for workers to finish
    for (size_t i = 0; i < sched->num_workers; i++) {
        thrd_join(sched->worker_threads[i], NULL);
    }
    
    return 0;
}

void worksteal_scheduler_stop(WorkStealingScheduler* sched) {
    if (!sched) return;
    atomic_store(&sched->running, false);
}
