
/**
 * @file graph_call.h
 * @brief Syscall-free graph call interface
 * 
 * Replaces traditional syscalls with port-based graph calls using SPSC rings.
 * Eliminates mode switch overhead (~100-300ns) with memory fence only (~20ns).
 */

#ifndef PHASE1_GRAPH_CALL_H
#define PHASE1_GRAPH_CALL_H

#include "../rings/spsc_ring.h"
#include "descriptor.h"

#ifdef __cplusplus
extern "C" {
#endif

// Graph call types
typedef enum {
    GRAPH_CALL_IPC_SEND = 0,
    GRAPH_CALL_IPC_RECV,
    GRAPH_CALL_MEMORY_ALLOC,
    GRAPH_CALL_MEMORY_FREE,
    GRAPH_CALL_FIBER_YIELD,
    GRAPH_CALL_FIBER_SPAWN,
    GRAPH_CALL_MAX
} graph_call_type_t;

// Graph call status
typedef enum {
    GRAPH_CALL_SUCCESS = 0,
    GRAPH_CALL_ERROR_INVALID_TYPE = -1,
    GRAPH_CALL_ERROR_INVALID_PARAM = -2,
    GRAPH_CALL_ERROR_RING_FULL = -3,
    GRAPH_CALL_ERROR_TIMEOUT = -4
} graph_call_status_t;

/**
 * @brief Graph call request structure
 */
typedef struct {
    graph_call_type_t type;
    uint64_t call_id;
    
    // Parameters (union for different call types)
    union {
        struct {
            memory_descriptor_t descriptor;
            uint32_t target_core;
        } ipc_send;
        
        struct {
            uint32_t required_perms;
        } ipc_recv;
        
        struct {
            size_t size;
            bool dma_capable;
        } memory_alloc;
        
        struct {
            void* ptr;
            size_t size;
        } memory_free;
        
        struct {
            void* yield_value;
        } fiber_yield;
        
        struct {
            void (*entry)(void*);
            void* arg;
            uint32_t priority;
        } fiber_spawn;
    } params;
    
    // Response
    union {
        void* ptr;
        uint64_t value;
        int status;
    } result;
} graph_call_request_t;

/**
 * @brief Graph call port (per-core)
 */
typedef struct {
    uint32_t core_id;
    spsc_ring_t* request_ring;
    spsc_ring_t* response_ring;
} graph_call_port_t;

/**
 * @brief Create graph call port
 * 
 * @param core_id Core ID
 * @param ring_capacity Ring buffer capacity
 * @return Pointer to port, or NULL on failure
 */
graph_call_port_t* graph_call_port_create(uint32_t core_id, size_t ring_capacity);

/**
 * @brief Destroy graph call port
 * 
 * @param port Port to destroy
 */
void graph_call_port_destroy(graph_call_port_t* port);

/**
 * @brief Submit graph call (user-space)
 * 
 * Enqueues request in SPSC ring without syscall.
 * 
 * @param port Port
 * @param request Request to submit
 * @return Status code
 */
graph_call_status_t graph_call_submit(graph_call_port_t* port, graph_call_request_t* request);

/**
 * @brief Wait for graph call response (user-space)
 * 
 * Polls response ring for completion.
 * 
 * @param port Port
 * @param request Request to wait for (updated with response)
 * @param timeout_ns Timeout in nanoseconds (0 = no timeout)
 * @return Status code
 */
graph_call_status_t graph_call_wait(graph_call_port_t* port,
                                     graph_call_request_t* request,
                                     uint64_t timeout_ns);

/**
 * @brief Process graph calls (kernel-space)
 * 
 * Dequeues and processes requests from ring.
 * 
 * @param port Port
 * @return Number of calls processed
 */
size_t graph_call_process(graph_call_port_t* port);

/**
 * @brief Get graph call type string
 * 
 * @param type Call type
 * @return String representation
 */
const char* graph_call_type_to_string(graph_call_type_t type);

#ifdef __cplusplus
}
#endif

#endif // PHASE1_GRAPH_CALL_H
