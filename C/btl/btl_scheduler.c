
// BTL Instruction Scheduler Implementation
#include "btl_scheduler.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAPACITY 256

struct BTL_Scheduler {
    BTL_DependencyGraph graph;
    uint32_t *schedule;
    size_t schedule_count;
    uint32_t critical_path_length;
};

BTL_Scheduler* btl_scheduler_create(void) {
    BTL_Scheduler *scheduler = malloc(sizeof(BTL_Scheduler));
    if (!scheduler) return NULL;
    
    scheduler->graph.nodes = calloc(INITIAL_CAPACITY, sizeof(BTL_InstructionNode*));
    if (!scheduler->graph.nodes) {
        free(scheduler);
        return NULL;
    }
    
    scheduler->graph.num_nodes = 0;
    scheduler->graph.nodes_capacity = INITIAL_CAPACITY;
    
    scheduler->schedule = calloc(INITIAL_CAPACITY, sizeof(uint32_t));
    if (!scheduler->schedule) {
        free(scheduler->graph.nodes);
        free(scheduler);
        return NULL;
    }
    
    scheduler->schedule_count = 0;
    scheduler->critical_path_length = 0;
    
    return scheduler;
}

void btl_scheduler_destroy(BTL_Scheduler *scheduler) {
    if (!scheduler) return;
    
    // Free all nodes
    for (size_t i = 0; i < scheduler->graph.num_nodes; i++) {
        BTL_InstructionNode *node = scheduler->graph.nodes[i];
        if (node) {
            free(node->predecessors);
            free(node->successors);
            free(node);
        }
    }
    
    free(scheduler->graph.nodes);
    free(scheduler->schedule);
    free(scheduler);
}

uint32_t btl_scheduler_add_instruction(BTL_Scheduler *scheduler, uint32_t opcode,
                                        uint32_t latency) {
    if (!scheduler) return 0;
    
    // Expand capacity if needed
    if (scheduler->graph.num_nodes >= scheduler->graph.nodes_capacity) {
        size_t new_capacity = scheduler->graph.nodes_capacity * 2;
        BTL_InstructionNode **new_nodes = realloc(scheduler->graph.nodes,
                                                   new_capacity * sizeof(BTL_InstructionNode*));
        if (!new_nodes) return 0;
        scheduler->graph.nodes = new_nodes;
        scheduler->graph.nodes_capacity = new_capacity;
    }
    
    // Create new node
    BTL_InstructionNode *node = calloc(1, sizeof(BTL_InstructionNode));
    if (!node) return 0;
    
    node->id = scheduler->graph.num_nodes;
    node->opcode = opcode;
    node->latency = latency;
    
    node->predecessors_capacity = 8;
    node->predecessors = calloc(8, sizeof(BTL_InstructionNode*));
    node->num_predecessors = 0;
    
    node->successors_capacity = 8;
    node->successors = calloc(8, sizeof(BTL_InstructionNode*));
    node->num_successors = 0;
    
    node->earliest_start = 0;
    node->latest_start = UINT32_MAX;
    node->scheduled = false;
    node->schedule_time = 0;
    
    scheduler->graph.nodes[scheduler->graph.num_nodes++] = node;
    
    return node->id;
}

void btl_scheduler_add_dependency(BTL_Scheduler *scheduler, uint32_t from_id,
                                   uint32_t to_id) {
    if (!scheduler || from_id >= scheduler->graph.num_nodes || 
        to_id >= scheduler->graph.num_nodes) return;
    
    BTL_InstructionNode *from = scheduler->graph.nodes[from_id];
    BTL_InstructionNode *to = scheduler->graph.nodes[to_id];
    
    // Add to successors of 'from'
    if (from->num_successors >= from->successors_capacity) {
        size_t new_capacity = from->successors_capacity * 2;
        BTL_InstructionNode **new_successors = realloc(from->successors,
                                                        new_capacity * sizeof(BTL_InstructionNode*));
        if (!new_successors) return;
        from->successors = new_successors;
        from->successors_capacity = new_capacity;
    }
    from->successors[from->num_successors++] = to;
    
    // Add to predecessors of 'to'
    if (to->num_predecessors >= to->predecessors_capacity) {
        size_t new_capacity = to->predecessors_capacity * 2;
        BTL_InstructionNode **new_predecessors = realloc(to->predecessors,
                                                          new_capacity * sizeof(BTL_InstructionNode*));
        if (!new_predecessors) return;
        to->predecessors = new_predecessors;
        to->predecessors_capacity = new_capacity;
    }
    to->predecessors[to->num_predecessors++] = from;
}

// Compute earliest start times (forward pass)
static void compute_earliest_start(BTL_Scheduler *scheduler) {
    for (size_t i = 0; i < scheduler->graph.num_nodes; i++) {
        BTL_InstructionNode *node = scheduler->graph.nodes[i];
        node->earliest_start = 0;
        
        // Find maximum of predecessor finish times
        for (size_t j = 0; j < node->num_predecessors; j++) {
            BTL_InstructionNode *pred = node->predecessors[j];
            uint32_t pred_finish = pred->earliest_start + pred->latency;
            if (pred_finish > node->earliest_start) {
                node->earliest_start = pred_finish;
            }
        }
    }
}

// Compute latest start times (backward pass)
static void compute_latest_start(BTL_Scheduler *scheduler, uint32_t makespan) {
    // Initialize all to makespan
    for (size_t i = 0; i < scheduler->graph.num_nodes; i++) {
        scheduler->graph.nodes[i]->latest_start = makespan;
    }
    
    // Backward pass
    for (size_t i = scheduler->graph.num_nodes; i > 0; i--) {
        BTL_InstructionNode *node = scheduler->graph.nodes[i - 1];
        
        // Find minimum of successor start times minus latency
        for (size_t j = 0; j < node->num_successors; j++) {
            BTL_InstructionNode *succ = node->successors[j];
            uint32_t required_start = succ->latest_start - node->latency;
            if (required_start < node->latest_start) {
                node->latest_start = required_start;
            }
        }
    }
}

bool btl_scheduler_build_graph(BTL_Scheduler *scheduler) {
    if (!scheduler) return false;
    
    compute_earliest_start(scheduler);
    
    // Find makespan (maximum earliest finish time)
    uint32_t makespan = 0;
    for (size_t i = 0; i < scheduler->graph.num_nodes; i++) {
        BTL_InstructionNode *node = scheduler->graph.nodes[i];
        uint32_t finish = node->earliest_start + node->latency;
        if (finish > makespan) makespan = finish;
    }
    
    compute_latest_start(scheduler, makespan);
    scheduler->critical_path_length = makespan;
    
    return true;
}

// List scheduling algorithm
bool btl_scheduler_schedule(BTL_Scheduler *scheduler) {
    if (!scheduler) return false;
    
    // Build graph first
    if (!btl_scheduler_build_graph(scheduler)) return false;
    
    // Ready list: instructions with no unscheduled predecessors
    BTL_InstructionNode **ready = calloc(scheduler->graph.num_nodes, 
                                         sizeof(BTL_InstructionNode*));
    if (!ready) return false;
    
    size_t ready_count = 0;
    uint32_t current_time = 0;
    size_t scheduled_count = 0;
    
    // Initialize ready list with instructions that have no predecessors
    for (size_t i = 0; i < scheduler->graph.num_nodes; i++) {
        BTL_InstructionNode *node = scheduler->graph.nodes[i];
        if (node->num_predecessors == 0) {
            ready[ready_count++] = node;
        }
    }
    
    // Schedule instructions
    while (scheduled_count < scheduler->graph.num_nodes) {
        if (ready_count == 0) {
            // Advance time to next instruction completion
            current_time++;
            
            // Check for newly ready instructions
            for (size_t i = 0; i < scheduler->graph.num_nodes; i++) {
                BTL_InstructionNode *node = scheduler->graph.nodes[i];
                if (node->scheduled) continue;
                
                bool all_preds_done = true;
                for (size_t j = 0; j < node->num_predecessors; j++) {
                    BTL_InstructionNode *pred = node->predecessors[j];
                    if (!pred->scheduled || 
                        pred->schedule_time + pred->latency > current_time) {
                        all_preds_done = false;
                        break;
                    }
                }
                
                if (all_preds_done) {
                    ready[ready_count++] = node;
                }
            }
            
            continue;
        }
        
        // Select instruction with earliest start time (priority)
        size_t best_idx = 0;
        for (size_t i = 1; i < ready_count; i++) {
            if (ready[i]->earliest_start < ready[best_idx]->earliest_start) {
                best_idx = i;
            }
        }
        
        BTL_InstructionNode *node = ready[best_idx];
        
        // Schedule the instruction
        node->scheduled = true;
        node->schedule_time = current_time;
        scheduler->schedule[scheduler->schedule_count++] = node->id;
        scheduled_count++;
        
        // Remove from ready list
        ready[best_idx] = ready[--ready_count];
    }
    
    free(ready);
    return true;
}

const uint32_t* btl_scheduler_get_schedule(BTL_Scheduler *scheduler, size_t *out_count) {
    if (!scheduler || !out_count) return NULL;
    *out_count = scheduler->schedule_count;
    return scheduler->schedule;
}

uint32_t btl_scheduler_get_critical_path(BTL_Scheduler *scheduler) {
    return scheduler ? scheduler->critical_path_length : 0;
}

size_t btl_scheduler_get_instruction_count(BTL_Scheduler *scheduler) {
    return scheduler ? scheduler->graph.num_nodes : 0;
}

uint32_t btl_scheduler_get_total_latency(BTL_Scheduler *scheduler) {
    if (!scheduler) return 0;
    
    uint32_t total = 0;
    for (size_t i = 0; i < scheduler->graph.num_nodes; i++) {
        total += scheduler->graph.nodes[i]->latency;
    }
    return total;
}
