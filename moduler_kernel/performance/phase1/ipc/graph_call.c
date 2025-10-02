
/**
 * @file graph_call.c
 * @brief Implementation of syscall-free graph calls
 */

#include "graph_call.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global call ID counter
static atomic_uint64_t g_next_call_id = 1;

graph_call_port_t* graph_call_port_create(uint32_t core_id, size_t ring_capacity) {
    graph_call_port_t* port = calloc(1, sizeof(graph_call_port_t));
    if (!port) {
        return NULL;
    }
    
    port->core_id = core_id;
    
    // Create request and response rings
    port->request_ring = spsc_ring_create(ring_capacity);
    if (!port->request_ring) {
        free(port);
        return NULL;
    }
    
    port->response_ring = spsc_ring_create(ring_capacity);
    if (!port->response_ring) {
        spsc_ring_destroy(port->request_ring);
        free(port);
        return NULL;
    }
    
    return port;
}

void graph_call_port_destroy(graph_call_port_t* port) {
    if (port) {
        if (port->request_ring) {
            spsc_ring_destroy(port->request_ring);
        }
        if (port->response_ring) {
            spsc_ring_destroy(port->response_ring);
        }
        free(port);
    }
}

graph_call_status_t graph_call_submit(graph_call_port_t* port, graph_call_request_t* request) {
    if (!port || !request) {
        return GRAPH_CALL_ERROR_INVALID_PARAM;
    }
    
    // Assign call ID
    request->call_id = atomic_fetch_add(&g_next_call_id, 1);
    
    // Enqueue request (no syscall!)
    ring_status_t status = spsc_ring_enqueue(port->request_ring, request);
    if (status != RING_SUCCESS) {
        return GRAPH_CALL_ERROR_RING_FULL;
    }
    
    // Memory fence to ensure visibility
    atomic_thread_fence(memory_order_release);
    
    return GRAPH_CALL_SUCCESS;
}

graph_call_status_t graph_call_wait(graph_call_port_t* port,
                                     graph_call_request_t* request,
                                     uint64_t timeout_ns) {
    if (!port || !request) {
        return GRAPH_CALL_ERROR_INVALID_PARAM;
    }
    
    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Poll response ring
    while (1) {
        graph_call_request_t* response = NULL;
        ring_status_t status = spsc_ring_dequeue(port->response_ring, (void**)&response);
        
        if (status == RING_SUCCESS && response && response->call_id == request->call_id) {
            // Found our response
            *request = *response;
            return GRAPH_CALL_SUCCESS;
        }
        
        // Check timeout
        if (timeout_ns > 0) {
            clock_gettime(CLOCK_MONOTONIC, &now);
            uint64_t elapsed_ns = (now.tv_sec - start.tv_sec) * 1000000000ULL +
                                  (now.tv_nsec - start.tv_nsec);
            if (elapsed_ns >= timeout_ns) {
                return GRAPH_CALL_ERROR_TIMEOUT;
            }
        }
        
        // Yield CPU
        __asm__ __volatile__("pause");
    }
}

size_t graph_call_process(graph_call_port_t* port) {
    if (!port) {
        return 0;
    }
    
    size_t processed = 0;
    
    // Process all pending requests
    while (1) {
        graph_call_request_t* request = NULL;
        ring_status_t status = spsc_ring_dequeue(port->request_ring, (void**)&request);
        
        if (status != RING_SUCCESS || !request) {
            break;
        }
        
        // Process request based on type
        switch (request->type) {
            case GRAPH_CALL_IPC_SEND:
                // Handle IPC send
                request->result.status = GRAPH_CALL_SUCCESS;
                break;
                
            case GRAPH_CALL_IPC_RECV:
                // Handle IPC receive
                request->result.status = GRAPH_CALL_SUCCESS;
                break;
                
            case GRAPH_CALL_MEMORY_ALLOC:
                // Handle memory allocation
                request->result.ptr = NULL;  // Placeholder
                request->result.status = GRAPH_CALL_SUCCESS;
                break;
                
            case GRAPH_CALL_MEMORY_FREE:
                // Handle memory free
                request->result.status = GRAPH_CALL_SUCCESS;
                break;
                
            case GRAPH_CALL_FIBER_YIELD:
                // Handle fiber yield
                request->result.status = GRAPH_CALL_SUCCESS;
                break;
                
            case GRAPH_CALL_FIBER_SPAWN:
                // Handle fiber spawn
                request->result.value = 0;  // Placeholder fiber ID
                request->result.status = GRAPH_CALL_SUCCESS;
                break;
                
            default:
                request->result.status = GRAPH_CALL_ERROR_INVALID_TYPE;
                break;
        }
        
        // Enqueue response
        spsc_ring_enqueue(port->response_ring, request);
        processed++;
    }
    
    return processed;
}

const char* graph_call_type_to_string(graph_call_type_t type) {
    switch (type) {
        case GRAPH_CALL_IPC_SEND: return "IPC_SEND";
        case GRAPH_CALL_IPC_RECV: return "IPC_RECV";
        case GRAPH_CALL_MEMORY_ALLOC: return "MEMORY_ALLOC";
        case GRAPH_CALL_MEMORY_FREE: return "MEMORY_FREE";
        case GRAPH_CALL_FIBER_YIELD: return "FIBER_YIELD";
        case GRAPH_CALL_FIBER_SPAWN: return "FIBER_SPAWN";
        default: return "UNKNOWN";
    }
}
