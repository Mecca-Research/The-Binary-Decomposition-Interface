
// BTL Instruction Scheduler - Dependency graph and scheduling
/**
 * @file btl_scheduler.h
 * @brief Task Scheduling System
 * @details This file provides the btl scheduler functionality for the BDI system.
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BTL_SCHEDULER_H
#define BTL_SCHEDULER_H

#include "../c23_compat.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Instruction node in dependency graph
typedef struct BTL_InstructionNode {
    uint32_t id;
    uint32_t opcode;
    uint32_t latency;
    
    // Dependencies
    struct BTL_InstructionNode **predecessors;
    size_t num_predecessors;
    size_t predecessors_capacity;
    
    struct BTL_InstructionNode **successors;
    size_t num_successors;
    size_t successors_capacity;
    
    // Scheduling info
    uint32_t earliest_start;
    uint32_t latest_start;
    bool scheduled;
    uint32_t schedule_time;
} BTL_InstructionNode;

// Dependency graph
typedef struct {
    BTL_InstructionNode **nodes;
    size_t num_nodes;
    size_t nodes_capacity;
} BTL_DependencyGraph;

// Scheduler context
typedef struct BTL_Scheduler BTL_Scheduler;

// Create and destroy scheduler
BTL_Scheduler* btl_scheduler_create(void);
void btl_scheduler_destroy(BTL_Scheduler *scheduler);

// Add instruction to scheduler
uint32_t btl_scheduler_add_instruction(BTL_Scheduler *scheduler, uint32_t opcode, 
                                        uint32_t latency);

// Add dependency between instructions
void btl_scheduler_add_dependency(BTL_Scheduler *scheduler, uint32_t from_id, 
                                   uint32_t to_id);

// Build dependency graph
bool btl_scheduler_build_graph(BTL_Scheduler *scheduler);

// Schedule instructions (list scheduling algorithm)
bool btl_scheduler_schedule(BTL_Scheduler *scheduler);

// Get scheduled instruction order
const uint32_t* btl_scheduler_get_schedule(BTL_Scheduler *scheduler, size_t *out_count);

// Get critical path length
uint32_t btl_scheduler_get_critical_path(BTL_Scheduler *scheduler);

// Statistics
size_t btl_scheduler_get_instruction_count(BTL_Scheduler *scheduler);
uint32_t btl_scheduler_get_total_latency(BTL_Scheduler *scheduler);

#endif // BTL_SCHEDULER_H
