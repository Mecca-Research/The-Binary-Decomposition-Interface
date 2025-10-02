
/**
 * @file nvme_device.c
 * @brief NVMe device discovery and initialization implementation
 */

#include "nvme_device.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

// Helper macros for register access
#define nvme_read32(dev, offset) \
    (*((volatile uint32_t*)((char*)(dev)->bar0 + (offset))))

#define nvme_read64(dev, offset) \
    (*((volatile uint64_t*)((char*)(dev)->bar0 + (offset))))

#define nvme_write32(dev, offset, value) \
    (*((volatile uint32_t*)((char*)(dev)->bar0 + (offset))) = (value))

#define nvme_write64(dev, offset, value) \
    (*((volatile uint64_t*)((char*)(dev)->bar0 + (offset))) = (value))

// Global state
static bool nvme_initialized = false;

int nvme_init(void) {
    if (nvme_initialized) {
        return 0;
    }
    
    // Initialize NVMe subsystem
    // In a real implementation, this would set up any global state
    
    nvme_initialized = true;
    return 0;
}

void nvme_shutdown(void) {
    if (!nvme_initialized) {
        return;
    }
    
    // Cleanup NVMe subsystem
    nvme_initialized = false;
}

int nvme_probe_devices(nvme_device_t** devices, size_t max_devices) {
    if (!nvme_initialized) {
        return -EINVAL;
    }
    
    // Scan /sys/bus/pci/devices for NVMe devices
    DIR* dir = opendir("/sys/bus/pci/devices");
    if (!dir) {
        return -errno;
    }
    
    size_t count = 0;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != NULL && count < max_devices) {
        if (entry->d_name[0] == '.') {
            continue;
        }
        
        // Check if this is an NVMe device (class 0x010802)
        char class_path[512];
        snprintf(class_path, sizeof(class_path), 
                 "/sys/bus/pci/devices/%s/class", entry->d_name);
        
        FILE* class_file = fopen(class_path, "r");
        if (!class_file) {
            continue;
        }
        
        uint32_t class_code;
        if (fscanf(class_file, "0x%x", &class_code) != 1) {
            fclose(class_file);
            continue;
        }
        fclose(class_file);
        
        // NVMe class code is 0x010802
        if (class_code != 0x010802) {
            continue;
        }
        
        // Parse PCI address
        uint16_t domain;
        uint8_t bus, device, function;
        if (sscanf(entry->d_name, "%hx:%hhx:%hhx.%hhx", 
                   &domain, &bus, &device, &function) != 4) {
            continue;
        }
        
        // Attach to device
        nvme_device_t* dev = nvme_attach_device(domain, bus, device, function);
        if (dev) {
            devices[count++] = dev;
        }
    }
    
    closedir(dir);
    return count;
}

nvme_device_t* nvme_attach_device(uint16_t domain, uint8_t bus,
                                   uint8_t device, uint8_t function) {
    // Allocate device structure
    nvme_device_t* dev = calloc(1, sizeof(nvme_device_t));
    if (!dev) {
        return NULL;
    }
    
    dev->domain = domain;
    dev->bus = bus;
    dev->device = device;
    dev->function = function;
    
    // Open PCI device resource file for BAR0
    char resource_path[512];
    snprintf(resource_path, sizeof(resource_path),
             "/sys/bus/pci/devices/%04x:%02x:%02x.%x/resource0",
             domain, bus, device, function);
    
    int fd = open(resource_path, O_RDWR | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "Failed to open NVMe device resource: %s\n", 
                strerror(errno));
        free(dev);
        return NULL;
    }
    
    // Get BAR0 size
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        free(dev);
        return NULL;
    }
    dev->bar0_size = st.st_size;
    
    // Map BAR0
    dev->bar0 = mmap(NULL, dev->bar0_size, PROT_READ | PROT_WRITE,
                     MAP_SHARED, fd, 0);
    close(fd);
    
    if (dev->bar0 == MAP_FAILED) {
        fprintf(stderr, "Failed to map NVMe BAR0: %s\n", strerror(errno));
        free(dev);
        return NULL;
    }
    
    // Read controller capabilities
    uint64_t cap = nvme_read64(dev, NVME_REG_CAP);
    dev->caps.max_queue_entries = ((cap >> 0) & 0xFFFF) + 1;
    dev->caps.timeout_ms = (((cap >> 24) & 0xFF) + 1) * 500;
    dev->caps.doorbell_stride = (1 << (((cap >> 32) & 0xF) + 2));
    dev->caps.supports_nvm_command_set = (cap >> 37) & 1;
    
    // Determine NUMA node
    char numa_path[512];
    snprintf(numa_path, sizeof(numa_path),
             "/sys/bus/pci/devices/%04x:%02x:%02x.%x/numa_node",
             domain, bus, device, function);
    
    FILE* numa_file = fopen(numa_path, "r");
    if (numa_file) {
        if (fscanf(numa_file, "%d", &dev->numa_node) != 1) {
            dev->numa_node = -1;
        }
        fclose(numa_file);
    } else {
        dev->numa_node = -1;
    }
    
    return dev;
}

void nvme_detach_device(nvme_device_t* dev) {
    if (!dev) {
        return;
    }
    
    // Disable controller
    nvme_disable_controller(dev);
    
    // Unmap BAR0
    if (dev->bar0 && dev->bar0 != MAP_FAILED) {
        munmap((void*)dev->bar0, dev->bar0_size);
    }
    
    // Free queue memory
    free(dev->admin_sq);
    free(dev->admin_cq);
    free(dev->io_sq);
    free(dev->io_cq);
    free(dev->io_sq_head);
    free(dev->io_sq_tail);
    free(dev->io_cq_head);
    free(dev->io_cq_phase);
    free(dev->namespaces);
    
    free(dev);
}

int nvme_get_capabilities(nvme_device_t* dev, nvme_controller_caps_t* caps) {
    if (!dev || !caps) {
        return -EINVAL;
    }
    
    *caps = dev->caps;
    return 0;
}

int nvme_reset_controller(nvme_device_t* dev) {
    if (!dev) {
        return -EINVAL;
    }
    
    // Disable controller
    uint32_t cc = nvme_read32(dev, NVME_REG_CC);
    cc &= ~NVME_CC_ENABLE;
    nvme_write32(dev, NVME_REG_CC, cc);
    
    // Wait for controller to be ready (CSTS.RDY = 0)
    uint64_t timeout = dev->caps.timeout_ms * 1000; // Convert to microseconds
    uint64_t elapsed = 0;
    
    while (elapsed < timeout) {
        uint32_t csts = nvme_read32(dev, NVME_REG_CSTS);
        if (!(csts & NVME_CSTS_RDY)) {
            break;
        }
        usleep(1000); // Sleep 1ms
        elapsed += 1000;
    }
    
    uint32_t csts = nvme_read32(dev, NVME_REG_CSTS);
    if (csts & NVME_CSTS_RDY) {
        return -ETIMEDOUT;
    }
    
    return 0;
}

int nvme_enable_controller(nvme_device_t* dev) {
    if (!dev) {
        return -EINVAL;
    }
    
    // Set controller configuration
    uint32_t cc = NVME_CC_ENABLE | NVME_CC_IOSQES | NVME_CC_IOCQES;
    nvme_write32(dev, NVME_REG_CC, cc);
    
    // Wait for controller to be ready (CSTS.RDY = 1)
    uint64_t timeout = dev->caps.timeout_ms * 1000;
    uint64_t elapsed = 0;
    
    while (elapsed < timeout) {
        uint32_t csts = nvme_read32(dev, NVME_REG_CSTS);
        if (csts & NVME_CSTS_RDY) {
            return 0;
        }
        if (csts & NVME_CSTS_CFS) {
            return -EIO; // Controller fatal status
        }
        usleep(1000);
        elapsed += 1000;
    }
    
    return -ETIMEDOUT;
}

int nvme_disable_controller(nvme_device_t* dev) {
    if (!dev) {
        return -EINVAL;
    }
    
    return nvme_reset_controller(dev);
}

void nvme_get_stats(nvme_device_t* dev, uint64_t* reads, uint64_t* writes,
                    uint64_t* completions, uint64_t* errors) {
    if (!dev) {
        return;
    }
    
    if (reads) *reads = dev->total_reads;
    if (writes) *writes = dev->total_writes;
    if (completions) *completions = dev->total_completions;
    if (errors) *errors = dev->total_errors;
}
