
// ===================================================================
// Phase 5.3: Priority Scheduler Implementation
// ===================================================================
#include "priority_scheduler.h"
#include <stdlib.h>
#include <string.h>

// --- Priority Scheduler ---

PriorityScheduler* priority_scheduler_create(BdiGraph* graph, DeviceVTable** devices, size_t device_count) {
    if (!graph || !devices) return NULL;
    
    PriorityScheduler* sched = malloc(sizeof(PriorityScheduler));
    if (!sched) return NULL;
    
    sched->graph = graph;
    sched->devices = devices;
    sched->device_count = device_count;
    sched->node_count = graph->node_count;
    sched->current_cycle = 0;
    atomic_init(&sched->running, false);
    
    // Initialize scheduled nodes
    sched->scheduled_nodes = malloc(sizeof(ScheduledNode) * sched->node_count);
    if (!sched->scheduled_nodes) {
        free(sched);
        return NULL;
    }
    
    for (size_t i = 0; i < sched->node_count; i++) {
        sched->scheduled_nodes[i].node_id = graph->nodes[i].id;
        sched->scheduled_nodes[i].priority = 0;
        sched->scheduled_nodes[i].deadline = UINT64_MAX;
        atomic_init(&sched->scheduled_nodes[i].scheduled, false);
    }
    
    return sched;
}

void priority_scheduler_free(PriorityScheduler* sched) {
    if (!sched) return;
    
    free(sched->scheduled_nodes);
    free(sched);
}

int scheduler_set_priority(PriorityScheduler* sched, NodeId node_id, int32_t priority) {
    if (!sched || node_id >= sched->node_count) return -1;
    
    for (size_t i = 0; i < sched->node_count; i++) {
        if (sched->scheduled_nodes[i].node_id == node_id) {
            sched->scheduled_nodes[i].priority = priority;
            return 0;
        }
    }
    
    return -1;
}

int scheduler_set_deadline(PriorityScheduler* sched, NodeId node_id, uint64_t deadline) {
    if (!sched || node_id >= sched->node_count) return -1;
    
    for (size_t i = 0; i < sched->node_count; i++) {
        if (sched->scheduled_nodes[i].node_id == node_id) {
            sched->scheduled_nodes[i].deadline = deadline;
            return 0;
        }
    }
    
    return -1;
}

ScheduledNode* find_highest_priority_node(PriorityScheduler* sched) {
    if (!sched) return NULL;
    
    ScheduledNode* highest = NULL;
    int32_t max_priority = INT32_MIN;
    
    for (size_t i = 0; i < sched->node_count; i++) {
        ScheduledNode* node = &sched->scheduled_nodes[i];
        
        if (atomic_load(&node->scheduled)) continue;
        
        // Check dependencies
        const GraphNode* graph_node = &sched->graph->nodes[i];
        bool deps_ready = true;
        
        for (size_t j = 0; j < graph_node->input_count; j++) {
            NodeId input_id = graph_node->inputs[j];
            if (input_id < sched->node_count) {
                if (!atomic_load(&sched->scheduled_nodes[input_id].scheduled)) {
                    deps_ready = false;
                    break;
                }
            }
        }
        
        if (!deps_ready) continue;
        
        // Boost priority if deadline is near
        int32_t effective_priority = node->priority;
        if (node->deadline != UINT64_MAX) {
            uint64_t time_to_deadline = node->deadline - sched->current_cycle;
            if (time_to_deadline < 100) {
                effective_priority += 1000;
            }
        }
        
        if (effective_priority > max_priority) {
            max_priority = effective_priority;
            highest = node;
        }
    }
    
    return highest;
}

bool is_deadline_missed(const ScheduledNode* node, uint64_t current_cycle) {
    if (!node) return false;
    return node->deadline != UINT64_MAX && current_cycle > node->deadline;
}

int priority_scheduler_run(PriorityScheduler* sched) {
    if (!sched) return -1;
    
    atomic_store(&sched->running, true);
    
    while (atomic_load(&sched->running)) {
        ScheduledNode* node = find_highest_priority_node(sched);
        
        if (!node) {
            break;  // No more nodes to schedule
        }
        
        // Check for deadline miss
        if (is_deadline_missed(node, sched->current_cycle)) {
            // Log deadline miss (TODO: proper logging)
        }
        
        // Execute node
        if (node->node_id < sched->graph->node_count) {
            const GraphNode* graph_node = &sched->graph->nodes[node->node_id];
            DeviceVTable* device = sched->devices[0];  // Use first device
            
            void* kernel = NULL;
            if (device->lower(graph_node, &kernel) == 0) {
                device->enqueue(kernel, NULL, 0);
                device->sync();
                free(kernel);
            }
        }
        
        atomic_store(&node->scheduled, true);
        sched->current_cycle++;
    }
    
    atomic_store(&sched->running, false);
    return 0;
}
