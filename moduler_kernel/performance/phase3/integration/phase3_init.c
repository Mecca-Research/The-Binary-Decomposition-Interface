#include "phase3_init.h"
#include "../nvme/nvme_device.h"
#include "../net/net_device.h"
#include "../gpu/gpu_device.h"
#include <stdio.h>

int phase3_init(void) {
    printf("Initializing Phase 3: I/O & Accelerator Fast Paths\n");
    
    // Initialize NVMe subsystem
    if (nvme_init() < 0) {
        fprintf(stderr, "Failed to initialize NVMe subsystem\n");
        return -1;
    }
    
    // Initialize network subsystem
    if (net_init() < 0) {
        fprintf(stderr, "Failed to initialize network subsystem\n");
        nvme_shutdown();
        return -1;
    }
    
    // Initialize GPU subsystem
    if (gpu_init() < 0) {
        fprintf(stderr, "Failed to initialize GPU subsystem\n");
        net_shutdown();
        nvme_shutdown();
        return -1;
    }
    
    printf("Phase 3 initialized successfully\n");
    return 0;
}

void phase3_shutdown(void) {
    printf("Shutting down Phase 3\n");
    gpu_shutdown();
    net_shutdown();
    nvme_shutdown();
}
