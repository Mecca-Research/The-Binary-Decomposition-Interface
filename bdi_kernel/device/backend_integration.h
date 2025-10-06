
/**
 * @file backend_integration.h
 * @brief Backend Scheduler Integration
 * 
 * Connects device I/O operations to backend schedulers,
 * routes device work to appropriate backends (CPU/GPU/FPGA/BPU),
 * and implements device affinity and NUMA-aware placement.
 */

#ifndef BDI_BACKEND_INTEGRATION_H
#define BDI_BACKEND_INTEGRATION_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "device_manager.h"

/* Backend types */
typedef enum {
    BACKEND_TYPE_CPU = 0,
    BACKEND_TYPE_GPU,
    BACKEND_TYPE_FPGA,
    BACKEND_TYPE_BPU,
    BACKEND_TYPE_MAX
} backend_type_t;

/* I/O operation types */
typedef enum {
    IO_OP_READ = 0,
    IO_OP_WRITE,
    IO_OP_IOCTL,
    IO_OP_DMA,
    IO_OP_MAX
} io_op_type_t;

/* I/O priority levels */
#define IO_PRIORITY_IDLE        0
#define IO_PRIORITY_LOW         1
#define IO_PRIORITY_NORMAL      2
#define IO_PRIORITY_HIGH        3
#define IO_PRIORITY_REALTIME    4

/**
 * @brief I/O request descriptor
 */
struct io_request {
    struct device *device;
    io_op_type_t op_type;
    void *buffer;
    size_t size;
    uint64_t offset;
    uint32_t priority;
    uint32_t flags;
    
    /* Backend routing */
    backend_type_t backend;
    uint32_t cpu_affinity;
    uint32_t numa_node;
    
    /* Completion callback */
    void (*completion)(struct io_request *req, int status);
    void *completion_data;
    
    /* Statistics */
    uint64_t submit_time;
    uint64_t completion_time;
};

/**
 * @brief Backend descriptor
 */
struct backend_descriptor {
    backend_type_t type;
    const char *name;
    
    /* Backend capabilities */
    bool supports_dma;
    bool supports_async;
    uint32_t max_queue_depth;
    
    /* Backend operations */
    int (*submit_io)(struct io_request *req);
    int (*cancel_io)(struct io_request *req);
    int (*poll_completion)(void);
    
    /* Statistics */
    _Atomic uint64_t total_requests;
    _Atomic uint64_t completed_requests;
    _Atomic uint64_t failed_requests;
    _Atomic uint64_t pending_requests;
};

/**
 * @brief Backend integration subsystem
 */
struct backend_integration {
    /* Registered backends */
    struct backend_descriptor backends[BACKEND_TYPE_MAX];
    
    /* Default backend for each device type */
    backend_type_t default_backend[DEVICE_TYPE_MAX];
    
    /* Statistics */
    _Atomic uint64_t total_io_requests;
    _Atomic uint64_t routed_to_cpu;
    _Atomic uint64_t routed_to_gpu;
    _Atomic uint64_t routed_to_fpga;
    _Atomic uint64_t routed_to_bpu;
    
    /* Subsystem state */
    _Atomic bool initialized;
};

/* Global backend integration instance */
extern struct backend_integration g_backend_integration;

/**
 * @brief Initialize backend integration
 * 
 * @return 0 on success, negative error code on failure
 */
int backend_integration_init(void);

/**
 * @brief Shutdown backend integration
 */
void backend_integration_shutdown(void);

/**
 * @brief Register a backend
 * 
 * @param backend Backend descriptor
 * @return 0 on success, negative error code on failure
 */
int backend_register(struct backend_descriptor *backend);

/**
 * @brief Unregister a backend
 * 
 * @param type Backend type
 */
void backend_unregister(backend_type_t type);

/**
 * @brief Set default backend for device type
 * 
 * @param device_type Device type
 * @param backend_type Backend type
 * @return 0 on success, negative error code on failure
 */
int backend_set_default(device_type_t device_type, backend_type_t backend_type);

/**
 * @brief Submit I/O request
 * 
 * @param req I/O request
 * @return 0 on success, negative error code on failure
 */
int backend_submit_io(struct io_request *req);

/**
 * @brief Route I/O request to appropriate backend
 * 
 * @param req I/O request
 * @return Backend type
 */
backend_type_t backend_route_io(struct io_request *req);

/**
 * @brief Set device backend affinity
 * 
 * @param dev Device
 * @param backend Backend type
 * @return 0 on success, negative error code on failure
 */
int backend_set_device_affinity(struct device *dev, backend_type_t backend);

/**
 * @brief Get backend statistics
 * 
 * @param type Backend type
 * @param total Output for total requests
 * @param completed Output for completed requests
 * @param failed Output for failed requests
 * @param pending Output for pending requests
 * @return 0 on success, negative error code on failure
 */
int backend_get_stats(backend_type_t type, uint64_t *total, uint64_t *completed,
                     uint64_t *failed, uint64_t *pending);

/**
 * @brief Poll for I/O completions
 * 
 * @return Number of completions processed
 */
uint32_t backend_poll_completions(void);

#endif /* BDI_BACKEND_INTEGRATION_H */
