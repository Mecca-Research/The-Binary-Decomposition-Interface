
#include "graph_executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <pthread.h>

// Forward declarations
static bool graph_executor_execute_sequential(GraphExecutor* executor, Graph* graph,
                                            GraphExecutionContext* context);

// Helper macros
#define NANO_TO_SEC(ns) ((double)(ns) / 1000000000.0)
#define SEC_TO_NANO(sec) ((uint64_t)((sec) * 1000000000.0))

// Time measurement helper
static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

// Scheduler implementation
GraphScheduler* graph_scheduler_create(Graph* graph) {
    if (!graph) return NULL;
    
    GraphScheduler* scheduler = (GraphScheduler*)calloc(1, sizeof(GraphScheduler));
    if (!scheduler) return NULL;
    
    scheduler->graph = graph;
    scheduler->ready_queue_size = graph->node_count * 2;
    scheduler->ready_queue = (uint32_t*)malloc(scheduler->ready_queue_size * sizeof(uint32_t));
    scheduler->remaining_deps = (uint32_t*)malloc(graph->node_count * sizeof(uint32_t));
    scheduler->execution_order = (uint32_t*)malloc(graph->node_count * sizeof(uint32_t));
    
    if (!scheduler->ready_queue || !scheduler->remaining_deps || !scheduler->execution_order) {
        graph_scheduler_destroy(scheduler);
        return NULL;
    }
    
    return scheduler;
}

void graph_scheduler_destroy(GraphScheduler* scheduler) {
    if (!scheduler) return;
    
    free(scheduler->ready_queue);
    free(scheduler->remaining_deps);
    free(scheduler->execution_order);
    free(scheduler);
}

bool graph_scheduler_schedule(GraphScheduler* scheduler) {
    if (!scheduler || !scheduler->graph) return false;
    
    uint64_t start_time = get_time_ns();
    
    Graph* graph = scheduler->graph;
    
    // Reset scheduler state
    scheduler->ready_queue_head = 0;
    scheduler->ready_queue_tail = 0;
    scheduler->execution_order_count = 0;
    
    // Initialize dependency counts
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node = graph->nodes[i];
        scheduler->remaining_deps[i] = node->dependency_count;
        
        // Nodes with no dependencies are ready
        if (node->dependency_count == 0) {
            scheduler->ready_queue[scheduler->ready_queue_tail++] = i;
        }
    }
    
    // Process ready queue
    while (scheduler->ready_queue_head < scheduler->ready_queue_tail) {
        uint32_t node_idx = scheduler->ready_queue[scheduler->ready_queue_head++];
        GraphNode* node = graph->nodes[node_idx];
        
        // Add to execution order
        scheduler->execution_order[scheduler->execution_order_count++] = node->id;
        
        // Update dependencies of successor nodes
        for (uint32_t i = 0; i < node->output_count; i++) {
            GraphEdge* edge = node->outputs[i];
            if (!edge) continue;
            
            // Find target node index
            uint32_t target_idx = UINT32_MAX;
            for (uint32_t j = 0; j < graph->node_count; j++) {
                if (graph->nodes[j] == edge->target) {
                    target_idx = j;
                    break;
                }
            }
            
            if (target_idx != UINT32_MAX) {
                scheduler->remaining_deps[target_idx]--;
                if (scheduler->remaining_deps[target_idx] == 0) {
                    scheduler->ready_queue[scheduler->ready_queue_tail++] = target_idx;
                }
            }
        }
    }
    
    scheduler->scheduling_time_ns += get_time_ns() - start_time;
    scheduler->total_scheduled++;
    
    // Check if all nodes were scheduled (no cycles)
    return scheduler->execution_order_count == graph->node_count;
}

uint32_t graph_scheduler_get_next_ready(GraphScheduler* scheduler) {
    if (!scheduler || scheduler->ready_queue_head >= scheduler->ready_queue_tail) {
        return 0; // No ready nodes
    }
    
    uint32_t node_idx = scheduler->ready_queue[scheduler->ready_queue_head++];
    return scheduler->graph->nodes[node_idx]->id;
}

void graph_scheduler_mark_completed(GraphScheduler* scheduler, uint32_t node_id) {
    if (!scheduler) return;
    
    // Find the node and update dependencies of its successors
    GraphNode* node = graph_get_node(scheduler->graph, node_id);
    if (!node) return;
    
    for (uint32_t i = 0; i < node->output_count; i++) {
        GraphEdge* edge = node->outputs[i];
        if (!edge) continue;
        
        // Find target node index
        uint32_t target_idx = UINT32_MAX;
        for (uint32_t j = 0; j < scheduler->graph->node_count; j++) {
            if (scheduler->graph->nodes[j] == edge->target) {
                target_idx = j;
                break;
            }
        }
        
        if (target_idx != UINT32_MAX) {
            scheduler->remaining_deps[target_idx]--;
            if (scheduler->remaining_deps[target_idx] == 0) {
                // Add to ready queue if there's space
                if (scheduler->ready_queue_tail < scheduler->ready_queue_size) {
                    scheduler->ready_queue[scheduler->ready_queue_tail++] = target_idx;
                }
            }
        }
    }
}

bool graph_scheduler_is_complete(const GraphScheduler* scheduler) {
    if (!scheduler) return false;
    
    // Check if all nodes have been executed (no remaining dependencies)
    for (uint32_t i = 0; i < scheduler->graph->node_count; i++) {
        if (scheduler->remaining_deps[i] > 0) {
            return false;
        }
    }
    
    return true;
}

void graph_scheduler_reset(GraphScheduler* scheduler) {
    if (!scheduler) return;
    
    scheduler->ready_queue_head = 0;
    scheduler->ready_queue_tail = 0;
    scheduler->execution_order_count = 0;
}

// Execution context implementation
GraphExecutionContext* graph_execution_context_create(Graph* graph) {
    if (!graph) return NULL;
    
    GraphExecutionContext* context = (GraphExecutionContext*)calloc(1, sizeof(GraphExecutionContext));
    if (!context) return NULL;
    
    context->graph = graph;  // Store reference to the graph
    context->input_count = graph->input_count;
    context->output_count = graph->output_count;
    
    if (context->input_count > 0) {
        context->input_values = (GraphValue*)calloc(context->input_count, sizeof(GraphValue));
    }
    
    if (context->output_count > 0) {
        context->output_values = (GraphValue*)calloc(context->output_count, sizeof(GraphValue));
    }
    
    // Allocate execution state tracking
    context->node_executed = (bool*)calloc(graph->node_count, sizeof(bool));
    context->node_outputs = (GraphValue**)calloc(graph->node_count, sizeof(GraphValue*));
    
    // Allocate output arrays for each node
    for (uint32_t i = 0; i < graph->node_count; i++) {
        GraphNode* node = graph->nodes[i];
        if (node->max_outputs > 0) {
            context->node_outputs[i] = (GraphValue*)calloc(node->max_outputs, sizeof(GraphValue));
        }
    }
    
    // Initialize memory pool
    context->memory_limit = 1024 * 1024; // 1MB default
    context->memory_pool = malloc(context->memory_limit);
    context->memory_used = 0;
    
    if (!context->node_executed || !context->node_outputs || !context->memory_pool) {
        graph_execution_context_destroy(context);
        return NULL;
    }
    
    return context;
}

void graph_execution_context_destroy(GraphExecutionContext* context) {
    if (!context) return;
    
    free(context->input_values);
    free(context->output_values);
    free(context->node_executed);
    
    if (context->node_outputs) {
        // Note: We don't know the exact count, so we can't free individual arrays
        // This is a limitation of the current design
        free(context->node_outputs);
    }
    
    free(context->memory_pool);
    free(context->error_message);
    free(context);
}

// Graph executor implementation
GraphExecutor* graph_executor_create(void) {
    GraphExecutorConfig config = graph_executor_default_config();
    return graph_executor_create_with_config(&config);
}

GraphExecutor* graph_executor_create_with_config(const GraphExecutorConfig* config) {
    GraphExecutor* executor = (GraphExecutor*)calloc(1, sizeof(GraphExecutor));
    if (!executor) return NULL;
    
    if (config) {
        executor->config = *config;
    } else {
        executor->config = graph_executor_default_config();
    }
    
    // Create JIT VM if enabled
    if (executor->config.enable_jit) {
        executor->jit_vm = jit_vm_create(1024 * 1024); // 1MB heap
        if (!executor->jit_vm) {
            // Continue without JIT
            executor->config.enable_jit = false;
        }
    }
    
    return executor;
}

void graph_executor_destroy(GraphExecutor* executor) {
    if (!executor) return;
    
    graph_scheduler_destroy(executor->scheduler);
    
    if (executor->jit_vm) {
        jit_vm_destroy(executor->jit_vm);
    }
    
    free(executor);
}

GraphExecutionResult graph_executor_execute(GraphExecutor* executor, Graph* graph,
                                          GraphValue* inputs, uint32_t input_count) {
    GraphExecutionResult result = {0};
    
    if (!executor || !graph) {
        result.error_message = strdup("Invalid executor or graph");
        return result;
    }
    
    uint64_t start_time = get_time_ns();
    
    // Create execution context
    GraphExecutionContext* context = graph_execution_context_create(graph);
    if (!context) {
        result.error_message = strdup("Failed to create execution context");
        return result;
    }
    
    // Set input values
    if (inputs && input_count > 0) {
        uint32_t copy_count = input_count < context->input_count ? input_count : context->input_count;
        memcpy(context->input_values, inputs, copy_count * sizeof(GraphValue));
    }
    
    // Execute the graph
    bool success = false;
    
    switch (executor->config.mode) {
        case GRAPH_EXEC_SEQUENTIAL:
            success = graph_executor_execute_sequential(executor, graph, context);
            break;
        case GRAPH_EXEC_PARALLEL:
            success = graph_executor_execute_parallel(executor, graph, context);
            break;
        case GRAPH_EXEC_JIT_COMPILED:
            if (executor->jit_vm) {
                success = graph_executor_execute_compiled(executor, graph,
                                                        inputs, input_count,
                                                        context->output_values, &context->output_count);
            } else {
                success = graph_executor_execute_sequential(executor, graph, context);
            }
            break;
        default:
            success = graph_executor_execute_sequential(executor, graph, context);
            break;
    }
    
    result.success = success;
    result.execution_time_ns = get_time_ns() - start_time;
    result.nodes_executed = context->nodes_executed;
    
    if (success) {
        // Copy output values
        result.output_count = context->output_count;
        if (result.output_count > 0) {
            result.output_values = (GraphValue*)malloc(result.output_count * sizeof(GraphValue));
            if (result.output_values) {
                memcpy(result.output_values, context->output_values, 
                      result.output_count * sizeof(GraphValue));
            }
        }
        
        executor->successful_executions++;
    } else {
        result.error_message = context->error_message ? strdup(context->error_message) : 
                              strdup("Graph execution failed");
        executor->failed_executions++;
    }
    
    // Update statistics
    executor->total_executions++;
    executor->total_execution_time_ns += result.execution_time_ns;
    executor->nodes_executed += result.nodes_executed;
    
    graph_execution_context_destroy(context);
    return result;
}

// Sequential execution implementation
static bool graph_executor_execute_sequential(GraphExecutor* executor, Graph* graph,
                                            GraphExecutionContext* context) {
    if (!executor || !graph || !context) return false;
    
    // Create scheduler if needed
    if (!executor->scheduler) {
        executor->scheduler = graph_scheduler_create(graph);
        if (!executor->scheduler) return false;
    }
    
    // Schedule the graph
    if (!graph_scheduler_schedule(executor->scheduler)) {
        graph_execution_context_set_error(context, "Failed to schedule graph (cycle detected)", 0);
        return false;
    }
    
    // Execute nodes in scheduled order
    for (uint32_t i = 0; i < executor->scheduler->execution_order_count; i++) {
        uint32_t node_id = executor->scheduler->execution_order[i];
        GraphNode* node = graph_get_node(graph, node_id);
        
        if (!node) {
            graph_execution_context_set_error(context, "Invalid node in execution order", node_id);
            return false;
        }
        
        // Prepare input values for this node
        GraphValue* node_inputs = NULL;
        if (node->input_count > 0) {
            node_inputs = (GraphValue*)malloc(node->input_count * sizeof(GraphValue));
            if (!node_inputs) {
                graph_execution_context_set_error(context, "Memory allocation failed", node_id);
                return false;
            }
            
            // Collect inputs from predecessor nodes or graph inputs
            for (uint32_t j = 0; j < node->input_count; j++) {
                GraphEdge* edge = node->inputs[j];
                if (edge && edge->source) {
                    // Find source node index
                    uint32_t source_idx = UINT32_MAX;
                    for (uint32_t k = 0; k < graph->node_count; k++) {
                        if (graph->nodes[k] == edge->source) {
                            source_idx = k;
                            break;
                        }
                    }
                    
                    if (source_idx != UINT32_MAX && context->node_outputs[source_idx]) {
                        node_inputs[j] = context->node_outputs[source_idx][edge->source_output];
                    } else if (edge->source->type == GRAPH_NODE_INPUT) {
                        // Input node - get value from context inputs
                        for (uint32_t k = 0; k < graph->input_count; k++) {
                            if (graph->inputs[k] == edge->source) {
                                if (k < context->input_count) {
                                    node_inputs[j] = context->input_values[k];
                                }
                                break;
                            }
                        }
                    }
                } else {
                    // No input edge - use zero value
                    node_inputs[j] = graph_value_zero(GRAPH_TYPE_F64);
                }
            }
        }
        
        // Find node index for output storage
        uint32_t node_idx = UINT32_MAX;
        for (uint32_t j = 0; j < graph->node_count; j++) {
            if (graph->nodes[j] == node) {
                node_idx = j;
                break;
            }
        }
        
        if (node_idx == UINT32_MAX) {
            free(node_inputs);
            graph_execution_context_set_error(context, "Node index not found", node_id);
            return false;
        }
        
        // Execute the node
        bool node_success = graph_execute_node(node, node_inputs, 
                                             context->node_outputs[node_idx], context);
        
        free(node_inputs);
        
        if (!node_success) {
            graph_execution_context_set_error(context, "Node execution failed", node_id);
            return false;
        }
        
        context->node_executed[node_idx] = true;
        context->nodes_executed++;
        
        // If this is an output node, copy to context outputs
        if (node->type == GRAPH_NODE_OUTPUT) {
            for (uint32_t j = 0; j < graph->output_count; j++) {
                if (graph->outputs[j] == node && j < context->output_count) {
                    context->output_values[j] = context->node_outputs[node_idx][0];
                    break;
                }
            }
        }
    }
    
    return true;
}

// Node execution implementation
bool graph_execute_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs,
                       GraphExecutionContext* context) {
    if (!node || !outputs) return false;
    
    uint64_t start_time = get_time_ns();
    bool success = false;
    
    switch (node->type) {
        case GRAPH_NODE_CONSTANT:
            success = graph_execute_constant_node(node, outputs);
            break;
            
        case GRAPH_NODE_INPUT:
            // Input nodes get their values from the execution context
            if (context && context->input_values) {
                // Find the input index for this node by searching in the graph's inputs array
                uint32_t input_index = UINT32_MAX;
                Graph* graph = context->graph;
                if (graph) {
                    for (uint32_t i = 0; i < graph->input_count; i++) {
                        if (graph->inputs[i] == node) {
                            input_index = i;
                            break;
                        }
                    }
                }
                
                // Use the actual input value if found and within bounds
                if (input_index != UINT32_MAX && input_index < context->input_count) {
                    outputs[0] = context->input_values[input_index];
                    success = true;
                } else {
                    // Error case: input node not found or index out of bounds
                    outputs[0] = graph_value_zero(GRAPH_TYPE_F64);
                    success = false;
                }
            } else {
                // No context or input values provided
                outputs[0] = graph_value_zero(GRAPH_TYPE_F64);
                success = false;
            }
            break;
            
        case GRAPH_NODE_OUTPUT:
            // Output nodes just pass through their input
            if (inputs) {
                outputs[0] = inputs[0];
            } else {
                outputs[0] = graph_value_zero(GRAPH_TYPE_F64);
            }
            success = true;
            break;
            
        case GRAPH_NODE_ADD:
        case GRAPH_NODE_SUB:
        case GRAPH_NODE_MUL:
        case GRAPH_NODE_DIV:
        case GRAPH_NODE_MOD:
            success = graph_execute_arithmetic_node(node, inputs, outputs);
            break;
            
        case GRAPH_NODE_AND:
        case GRAPH_NODE_OR:
        case GRAPH_NODE_XOR:
        case GRAPH_NODE_NOT:
            success = graph_execute_logical_node(node, inputs, outputs);
            break;
            
        case GRAPH_NODE_EQ:
        case GRAPH_NODE_NE:
        case GRAPH_NODE_LT:
        case GRAPH_NODE_LE:
        case GRAPH_NODE_GT:
        case GRAPH_NODE_GE:
            success = graph_execute_comparison_node(node, inputs, outputs);
            break;
            
        case GRAPH_NODE_BRANCH:
        case GRAPH_NODE_PHI:
        case GRAPH_NODE_SELECT:
            success = graph_execute_control_node(node, inputs, outputs, context);
            break;
            
        case GRAPH_NODE_LOAD:
        case GRAPH_NODE_STORE:
        case GRAPH_NODE_ALLOC:
            success = graph_execute_memory_node(node, inputs, outputs, context);
            break;
            
        case GRAPH_NODE_CAST:
            if (inputs && node->custom_data) {
                struct {
                    GraphDataType from_type;
                    GraphDataType to_type;
                } *cast_data = (void*)node->custom_data;
                outputs[0] = graph_value_cast(&inputs[0], cast_data->to_type);
                success = true;
            }
            break;
            
        case GRAPH_NODE_CUSTOM:
            success = graph_execute_custom_node(node, inputs, outputs, context);
            break;
            
        default:
            success = false;
            break;
    }
    
    uint64_t execution_time = get_time_ns() - start_time;
    node->execution_time_ns += execution_time;
    node->execution_count++;
    
    return success;
}

bool graph_execute_constant_node(GraphNode* node, GraphValue* outputs) {
    if (!node || !outputs) return false;
    
    outputs[0] = node->constant_value;
    return true;
}

bool graph_execute_arithmetic_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs) {
    if (!node || !inputs || !outputs) return false;
    
    switch (node->type) {
        case GRAPH_NODE_ADD:
            outputs[0] = graph_value_add(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_SUB:
            outputs[0] = graph_value_sub(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_MUL:
            outputs[0] = graph_value_mul(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_DIV:
            outputs[0] = graph_value_div(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_MOD:
            outputs[0] = graph_value_mod(&inputs[0], &inputs[1]);
            break;
        default:
            return false;
    }
    
    return true;
}

bool graph_execute_logical_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs) {
    if (!node || !outputs) return false;
    
    switch (node->type) {
        case GRAPH_NODE_AND:
            if (!inputs) return false;
            outputs[0] = graph_value_and(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_OR:
            if (!inputs) return false;
            outputs[0] = graph_value_or(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_XOR:
            if (!inputs) return false;
            outputs[0] = graph_value_xor(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_NOT:
            if (!inputs) return false;
            outputs[0] = graph_value_not(&inputs[0]);
            break;
        default:
            return false;
    }
    
    return true;
}

bool graph_execute_comparison_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs) {
    if (!node || !inputs || !outputs) return false;
    
    switch (node->type) {
        case GRAPH_NODE_EQ:
            outputs[0] = graph_value_eq(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_NE:
            outputs[0] = graph_value_ne(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_LT:
            outputs[0] = graph_value_lt(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_LE:
            outputs[0] = graph_value_le(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_GT:
            outputs[0] = graph_value_gt(&inputs[0], &inputs[1]);
            break;
        case GRAPH_NODE_GE:
            outputs[0] = graph_value_ge(&inputs[0], &inputs[1]);
            break;
        default:
            return false;
    }
    
    return true;
}

bool graph_execute_control_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs,
                               GraphExecutionContext* context) {
    if (!node || !outputs) return false;
    
    switch (node->type) {
        case GRAPH_NODE_SELECT:
            if (!inputs) return false;
            // Select: condition ? true_value : false_value
            if (inputs[0].data.boolean) {
                outputs[0] = inputs[1];
            } else {
                outputs[0] = inputs[2];
            }
            break;
            
        case GRAPH_NODE_PHI:
            // PHI node - for now, just return the first input
            if (inputs) {
                outputs[0] = inputs[0];
            } else {
                outputs[0] = graph_value_zero(GRAPH_TYPE_F64);
            }
            break;
            
        default:
            return false;
    }
    
    return true;
}

bool graph_execute_memory_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs,
                              GraphExecutionContext* context) {
    if (!node || !outputs || !context) return false;
    
    switch (node->type) {
        case GRAPH_NODE_ALLOC:
            if (!inputs) return false;
            // Allocate memory from context pool
            size_t size = (size_t)inputs[0].data.i64;
            void* ptr = graph_executor_alloc(context, size);
            outputs[0].type = GRAPH_TYPE_PTR;
            outputs[0].data.ptr = ptr;
            break;
            
        case GRAPH_NODE_LOAD:
            if (!inputs) return false;
            // Load from memory (simplified - just return zero for now)
            outputs[0] = graph_value_zero(GRAPH_TYPE_F64);
            break;
            
        case GRAPH_NODE_STORE:
            if (!inputs) return false;
            // Store to memory (simplified - no-op for now)
            outputs[0] = graph_value_zero(GRAPH_TYPE_VOID);
            break;
            
        default:
            return false;
    }
    
    return true;
}

bool graph_execute_custom_node(GraphNode* node, GraphValue* inputs, GraphValue* outputs,
                              GraphExecutionContext* context) {
    if (!node || !outputs) return false;
    
    // Custom node execution - for now, just return zero
    outputs[0] = graph_value_zero(GRAPH_TYPE_F64);
    return true;
}

// Configuration
GraphExecutorConfig graph_executor_default_config(void) {
    GraphExecutorConfig config = {
        .mode = GRAPH_EXEC_SEQUENTIAL,
        .max_threads = 1,
        .enable_jit = false,
        .enable_optimization = true,
        .enable_profiling = false,
        .jit_threshold = 100,
        .memory_limit = 1024 * 1024 // 1MB
    };
    return config;
}

void graph_executor_set_config(GraphExecutor* executor, const GraphExecutorConfig* config) {
    if (executor && config) {
        executor->config = *config;
    }
}

void graph_executor_get_config(const GraphExecutor* executor, GraphExecutorConfig* config) {
    if (executor && config) {
        *config = executor->config;
    }
}

// Statistics
void graph_executor_get_stats(const GraphExecutor* executor, GraphExecutorStats* stats) {
    if (!executor || !stats) return;
    
    memset(stats, 0, sizeof(GraphExecutorStats));
    
    stats->total_executions = executor->total_executions;
    stats->successful_executions = executor->successful_executions;
    stats->failed_executions = executor->failed_executions;
    stats->total_execution_time_ns = executor->total_execution_time_ns;
    stats->jit_compilation_time_ns = executor->jit_compilation_time_ns;
    stats->nodes_executed = executor->nodes_executed;
    stats->edges_traversed = executor->edges_traversed;
    stats->memory_allocated = executor->memory_allocated;
    stats->memory_freed = executor->memory_freed;
    stats->active_threads = executor->active_threads;
    
    if (stats->total_executions > 0) {
        stats->average_execution_time_ns = stats->total_execution_time_ns / stats->total_executions;
    }
    
    if (stats->total_execution_time_ns > 0) {
        stats->throughput_nodes_per_second = 
            (double)stats->nodes_executed / NANO_TO_SEC(stats->total_execution_time_ns);
    }
}

void graph_executor_reset_stats(GraphExecutor* executor) {
    if (!executor) return;
    
    executor->total_executions = 0;
    executor->successful_executions = 0;
    executor->failed_executions = 0;
    executor->total_execution_time_ns = 0;
    executor->jit_compilation_time_ns = 0;
    executor->nodes_executed = 0;
    executor->edges_traversed = 0;
    executor->memory_allocated = 0;
    executor->memory_freed = 0;
}

void graph_executor_print_stats(const GraphExecutor* executor) {
    if (!executor) return;
    
    GraphExecutorStats stats;
    graph_executor_get_stats(executor, &stats);
    
    printf("Graph Executor Statistics:\n");
    printf("  Total Executions: %lu\n", stats.total_executions);
    printf("  Successful: %lu\n", stats.successful_executions);
    printf("  Failed: %lu\n", stats.failed_executions);
    printf("  Total Execution Time: %.3f ms\n", NANO_TO_SEC(stats.total_execution_time_ns) * 1000);
    printf("  Average Execution Time: %.3f ms\n", NANO_TO_SEC(stats.average_execution_time_ns) * 1000);
    printf("  Nodes Executed: %lu\n", stats.nodes_executed);
    printf("  Throughput: %.2f nodes/sec\n", stats.throughput_nodes_per_second);
    printf("  Memory Allocated: %lu bytes\n", stats.memory_allocated);
    printf("  Active Threads: %u\n", stats.active_threads);
}

// Memory management
void* graph_executor_alloc(GraphExecutionContext* context, size_t size) {
    if (!context || !context->memory_pool) return malloc(size);
    
    // Align to 8-byte boundary
    size = (size + 7) & ~7;
    
    if (context->memory_used + size > context->memory_limit) {
        return NULL; // Out of memory
    }
    
    void* ptr = (char*)context->memory_pool + context->memory_used;
    context->memory_used += size;
    return ptr;
}

void graph_executor_free(GraphExecutionContext* context, void* ptr) {
    // Memory pool doesn't support individual free operations
    // Memory is freed when context is destroyed
    (void)context;
    (void)ptr;
}

bool graph_executor_check_memory_limit(const GraphExecutionContext* context, size_t additional) {
    if (!context) return false;
    return context->memory_used + additional <= context->memory_limit;
}

// Error handling
void graph_execution_context_set_error(GraphExecutionContext* context, 
                                      const char* message, uint32_t node_id) {
    if (!context) return;
    
    free(context->error_message);
    context->error_message = message ? strdup(message) : NULL;
    context->error_node_id = node_id;
}

const char* graph_execution_error_to_string(const GraphExecutionResult* result) {
    if (!result) return "Invalid result";
    
    if (result->success) return "Success";
    
    return result->error_message ? result->error_message : "Unknown error";
}

// Utility functions for graph values
bool graph_value_is_valid(const GraphValue* value) {
    if (!value) return false;
    
    switch (value->type) {
        case GRAPH_TYPE_F32:
            return !isnan(value->data.f32) && !isinf(value->data.f32);
        case GRAPH_TYPE_F64:
            return !isnan(value->data.f64) && !isinf(value->data.f64);
        default:
            return true;
    }
}

GraphValue graph_value_zero(GraphDataType type) {
    GraphValue value = {0};
    value.type = type;
    return value;
}

GraphValue graph_value_add(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    // For simplicity, assume both are F64
    result.type = GRAPH_TYPE_F64;
    result.data.f64 = a->data.f64 + b->data.f64;
    
    return result;
}

GraphValue graph_value_sub(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_F64;
    result.data.f64 = a->data.f64 - b->data.f64;
    
    return result;
}

GraphValue graph_value_mul(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_F64;
    result.data.f64 = a->data.f64 * b->data.f64;
    
    return result;
}

GraphValue graph_value_div(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b || b->data.f64 == 0.0) return result;
    
    result.type = GRAPH_TYPE_F64;
    result.data.f64 = a->data.f64 / b->data.f64;
    
    return result;
}

GraphValue graph_value_mod(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b || b->data.f64 == 0.0) return result;
    
    result.type = GRAPH_TYPE_F64;
    result.data.f64 = fmod(a->data.f64, b->data.f64);
    
    return result;
}

GraphValue graph_value_and(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_BOOL;
    result.data.boolean = a->data.boolean && b->data.boolean;
    
    return result;
}

GraphValue graph_value_or(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_BOOL;
    result.data.boolean = a->data.boolean || b->data.boolean;
    
    return result;
}

GraphValue graph_value_xor(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_BOOL;
    result.data.boolean = a->data.boolean != b->data.boolean;
    
    return result;
}

GraphValue graph_value_not(const GraphValue* a) {
    GraphValue result = {0};
    
    if (!a) return result;
    
    result.type = GRAPH_TYPE_BOOL;
    result.data.boolean = !a->data.boolean;
    
    return result;
}

GraphValue graph_value_eq(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_BOOL;
    result.data.boolean = (a->data.f64 == b->data.f64);
    
    return result;
}

GraphValue graph_value_ne(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_BOOL;
    result.data.boolean = (a->data.f64 != b->data.f64);
    
    return result;
}

GraphValue graph_value_lt(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_BOOL;
    result.data.boolean = (a->data.f64 < b->data.f64);
    
    return result;
}

GraphValue graph_value_le(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_BOOL;
    result.data.boolean = (a->data.f64 <= b->data.f64);
    
    return result;
}

GraphValue graph_value_gt(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_BOOL;
    result.data.boolean = (a->data.f64 > b->data.f64);
    
    return result;
}

GraphValue graph_value_ge(const GraphValue* a, const GraphValue* b) {
    GraphValue result = {0};
    
    if (!a || !b) return result;
    
    result.type = GRAPH_TYPE_BOOL;
    result.data.boolean = (a->data.f64 >= b->data.f64);
    
    return result;
}

GraphValue graph_value_cast(const GraphValue* value, GraphDataType target_type) {
    GraphValue result = {0};
    
    if (!value) return result;
    
    result.type = target_type;
    
    // Simple casting - extend as needed
    switch (target_type) {
        case GRAPH_TYPE_F64:
            switch (value->type) {
                case GRAPH_TYPE_F32:
                    result.data.f64 = (double)value->data.f32;
                    break;
                case GRAPH_TYPE_I32:
                    result.data.f64 = (double)value->data.i32;
                    break;
                case GRAPH_TYPE_I64:
                    result.data.f64 = (double)value->data.i64;
                    break;
                default:
                    result.data.f64 = value->data.f64;
                    break;
            }
            break;
            
        case GRAPH_TYPE_I32:
            result.data.i32 = (int32_t)value->data.f64;
            break;
            
        case GRAPH_TYPE_BOOL:
            result.data.boolean = (value->data.f64 != 0.0);
            break;
            
        default:
            result = *value;
            result.type = target_type;
            break;
    }
    
    return result;
}

// Parallel execution stub
bool graph_executor_execute_parallel(GraphExecutor* executor, Graph* graph,
                                    GraphExecutionContext* context) {
    // For now, fall back to sequential execution
    return graph_executor_execute_sequential(executor, graph, context);
}

// JIT compilation stubs
bool graph_executor_compile_graph(GraphExecutor* executor, Graph* graph) {
    // TODO: Implement JIT compilation
    return false;
}

bool graph_executor_execute_compiled(GraphExecutor* executor, Graph* graph,
                                   GraphValue* inputs, uint32_t input_count,
                                   GraphValue* outputs, uint32_t* output_count) {
    // TODO: Implement compiled execution
    return false;
}

