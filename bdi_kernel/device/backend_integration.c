
/**
 * @file backend_integration.c
 * @brief Backend Scheduler Integration Implementation
 */

#include "backend_integration.h"
#include <stdio.h>
#include <string.h>

/* Global backend integration instance */
struct backend_integration g_backend_integration = {0};

/**
 * @brief Initialize backend integration
 */
int backend_integration_init(void) {
    if (atomic_load_explicit(&g_backend_integration.initialized, memory_order_acquire)) {
        return 0; /* Already initialized */
    }
    
    /* Initialize backend descriptors */
    for (uint32_t i = 0; i < BACKEND_TYPE_MAX; i++) {
        struct backend_descriptor *backend = &g_backend_integration.backends[i];
        backend->type = (backend_type_t)i;
        backend->name = nullptr;
        backend->supports_dma = false;
        backend->supports_async = false;
        backend->max_queue_depth = 0;
        backend->submit_io = nullptr;
        backend->cancel_io = nullptr;
        backend->poll_completion = nullptr;
        atomic_store_explicit(&backend->total_requests, 0, memory_order_relaxed);
        atomic_store_explicit(&backend->completed_requests, 0, memory_order_relaxed);
        atomic_store_explicit(&backend->failed_requests, 0, memory_order_relaxed);
        atomic_store_explicit(&backend->pending_requests, 0, memory_order_relaxed);
    }
    
    /* Set default backends for device types */
    for (uint32_t i = 0; i < DEVICE_TYPE_MAX; i++) {
        g_backend_integration.default_backend[i] = BACKEND_TYPE_CPU;
    }
    
    /* Initialize statistics */
    atomic_store_explicit(&g_backend_integration.total_io_requests, 0, memory_order_relaxed);
    atomic_store_explicit(&g_backend_integration.routed_to_cpu, 0, memory_order_relaxed);
    atomic_store_explicit(&g_backend_integration.routed_to_gpu, 0, memory_order_relaxed);
    atomic_store_explicit(&g_backend_integration.routed_to_fpga, 0, memory_order_relaxed);
    atomic_store_explicit(&g_backend_integration.routed_to_bpu, 0, memory_order_relaxed);
    
    atomic_store_explicit(&g_backend_integration.initialized, true, memory_order_release);
    
    printf("[BackendIntegration] Initialized successfully\n");
    return 0;
}

/**
 * @brief Shutdown backend integration
 */
void backend_integration_shutdown(void) {
    if (!atomic_load_explicit(&g_backend_integration.initialized, memory_order_acquire)) {
        return;
    }
    
    atomic_store_explicit(&g_backend_integration.initialized, false, memory_order_release);
    printf("[BackendIntegration] Shutdown complete\n");
}

/**
 * @brief Register a backend
 */
int backend_register(struct backend_descriptor *backend) {
    if (backend == nullptr || backend->type >= BACKEND_TYPE_MAX) {
        return -1;
    }
    
    struct backend_descriptor *target = &g_backend_integration.backends[backend->type];
    
    target->name = backend->name;
    target->supports_dma = backend->supports_dma;
    target->supports_async = backend->supports_async;
    target->max_queue_depth = backend->max_queue_depth;
    target->submit_io = backend->submit_io;
    target->cancel_io = backend->cancel_io;
    target->poll_completion = backend->poll_completion;
    
    printf("[BackendIntegration] Registered backend: %s (type: %d)\n", 
           backend->name, backend->type);
    
    return 0;
}

/**
 * @brief Unregister a backend
 */
void backend_unregister(backend_type_t type) {
    if (type >= BACKEND_TYPE_MAX) {
        return;
    }
    
    struct backend_descriptor *backend = &g_backend_integration.backends[type];
    
    printf("[BackendIntegration] Unregistered backend: %s\n", 
           backend->name ? backend->name : "unknown");
    
    backend->name = nullptr;
    backend->submit_io = nullptr;
    backend->cancel_io = nullptr;
    backend->poll_completion = nullptr;
}

/**
 * @brief Set default backend for device type
 */
int backend_set_default(device_type_t device_type, backend_type_t backend_type) {
    if (device_type >= DEVICE_TYPE_MAX || backend_type >= BACKEND_TYPE_MAX) {
        return -1;
    }
    
    g_backend_integration.default_backend[device_type] = backend_type;
    
    printf("[BackendIntegration] Set default backend for device type %d: %d\n",
           device_type, backend_type);
    
    return 0;
}

/**
 * @brief Route I/O request to appropriate backend
 */
backend_type_t backend_route_io(struct io_request *req) {
    if (req == nullptr || req->device == nullptr) {
        return BACKEND_TYPE_CPU;
    }
    
    /* Check if device has specific backend affinity */
    if (req->device->platform_data != nullptr) {
        backend_type_t *affinity = (backend_type_t *)req->device->platform_data;
        return *affinity;
    }
    
    /* Use default backend for device type */
    if (req->device->type < DEVICE_TYPE_MAX) {
        return g_backend_integration.default_backend[req->device->type];
    }
    
    return BACKEND_TYPE_CPU;
}

/**
 * @brief Submit I/O request
 */
int backend_submit_io(struct io_request *req) {
    if (req == nullptr) {
        return -1;
    }
    
    /* Route to appropriate backend */
    backend_type_t backend = backend_route_io(req);
    req->backend = backend;
    
    /* Update routing statistics */
    atomic_fetch_add_explicit(&g_backend_integration.total_io_requests, 1, 
                              memory_order_relaxed);
    
    switch (backend) {
        case BACKEND_TYPE_CPU:
            atomic_fetch_add_explicit(&g_backend_integration.routed_to_cpu, 1, 
                                     memory_order_relaxed);
            break;
        case BACKEND_TYPE_GPU:
            atomic_fetch_add_explicit(&g_backend_integration.routed_to_gpu, 1,
                                     memory_order_relaxed);
            break;
        case BACKEND_TYPE_FPGA:
            atomic_fetch_add_explicit(&g_backend_integration.routed_to_fpga, 1,
                                     memory_order_relaxed);
            break;
        case BACKEND_TYPE_BPU:
            atomic_fetch_add_explicit(&g_backend_integration.routed_to_bpu, 1,
                                     memory_order_relaxed);
            break;
        default:
            break;
    }
    
    /* Get backend descriptor */
    struct backend_descriptor *backend_desc = &g_backend_integration.backends[backend];
    
    if (backend_desc->submit_io == nullptr) {
        printf("[BackendIntegration] Backend %d has no submit_io function\n", backend);
        return -1;
    }
    
    /* Update backend statistics */
    atomic_fetch_add_explicit(&backend_desc->total_requests, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&backend_desc->pending_requests, 1, memory_order_relaxed);
    
    /* Submit to backend */
    int result = backend_desc->submit_io(req);
    
    if (result != 0) {
        atomic_fetch_add_explicit(&backend_desc->failed_requests, 1, memory_order_relaxed);
        atomic_fetch_sub_explicit(&backend_desc->pending_requests, 1, memory_order_relaxed);
    }
    
    return result;
}

/**
 * @brief Set device backend affinity
 */
int backend_set_device_affinity(struct device *dev, backend_type_t backend) {
    if (dev == nullptr || backend >= BACKEND_TYPE_MAX) {
        return -1;
    }
    
    /* Allocate affinity data if not present */
    if (dev->platform_data == nullptr) {
        backend_type_t *affinity = (backend_type_t *)malloc(sizeof(backend_type_t));
        if (affinity == nullptr) {
            return -1;
        }
        *affinity = backend;
        dev->platform_data = affinity;
    } else {
        *(backend_type_t *)dev->platform_data = backend;
    }
    
    printf("[BackendIntegration] Set device %s affinity to backend %d\n",
           dev->name, backend);
    
    return 0;
}

/**
 * @brief Get backend statistics
 */
int backend_get_stats(backend_type_t type, uint64_t *total, uint64_t *completed,
                     uint64_t *failed, uint64_t *pending) {
    if (type >= BACKEND_TYPE_MAX) {
        return -1;
    }
    
    struct backend_descriptor *backend = &g_backend_integration.backends[type];
    
    if (total != nullptr) {
        *total = atomic_load_explicit(&backend->total_requests, memory_order_relaxed);
    }
    if (completed != nullptr) {
        *completed = atomic_load_explicit(&backend->completed_requests, memory_order_relaxed);
    }
    if (failed != nullptr) {
        *failed = atomic_load_explicit(&backend->failed_requests, memory_order_relaxed);
    }
    if (pending != nullptr) {
        *pending = atomic_load_explicit(&backend->pending_requests, memory_order_relaxed);
    }
    
    return 0;
}

/**
 * @brief Poll for I/O completions
 */
uint32_t backend_poll_completions(void) {
    uint32_t total_completions = 0;
    
    for (uint32_t i = 0; i < BACKEND_TYPE_MAX; i++) {
        struct backend_descriptor *backend = &g_backend_integration.backends[i];
        
        if (backend->poll_completion != nullptr) {
            int completions = backend->poll_completion();
            if (completions > 0) {
                total_completions += completions;
                atomic_fetch_add_explicit(&backend->completed_requests, completions,
                                         memory_order_relaxed);
                atomic_fetch_sub_explicit(&backend->pending_requests, completions,
                                         memory_order_relaxed);
            }
        }
    }
    
    return total_completions;
}
