#ifndef PHASE3_NET_DEVICE_H
#define PHASE3_NET_DEVICE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Device structure (to be implemented)
typedef struct net_device net_device_t;

// Initialize subsystem
int net_init(void);

// Shutdown subsystem
void net_shutdown(void);

// Probe devices
int net_probe_devices(net_device_t** devices, size_t max_devices);

#ifdef __cplusplus
}
#endif

#endif // PHASE3_NET_DEVICE_H
