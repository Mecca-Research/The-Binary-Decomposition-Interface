#ifndef PHASE3_GPU_DEVICE_H
#define PHASE3_GPU_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Device structure (to be implemented)
typedef struct gpu_device gpu_device_t;

// Initialize subsystem
int gpu_init(void);

// Shutdown subsystem
void gpu_shutdown(void);

// Probe devices
int gpu_probe_devices(gpu_device_t** devices, size_t max_devices);

#ifdef __cplusplus
}
#endif

#endif // PHASE3_GPU_DEVICE_H
