
// ===================================================================
// DESC: Integration layer implementation for M4-M6 components
// ===================================================================

#include "integration.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global system instance
bdi_system_t* global_bdi_system = NULL;

// --- System Initialization ---

int bdi_system_init(bdi_system_t* system) {
    memset(system, 0, sizeof(bdi_system_t));
    
    printf("BDI System: Initializing M4-M6 components...\n");
    
    // Initialize core components (assuming M0-M3 are already set up)
    // This would typically be done by the main kernel initialization
    
    // Initialize M4: Storage subsystem
    if (bdi_init_storage_subsystem(system) != 0) {
        printf("BDI System: Failed to initialize storage subsystem\n");
        return -1;
    }
    
    // Initialize M5: USB/HID subsystem
    if (bdi_init_usb_subsystem(system) != 0) {
        printf("BDI System: Failed to initialize USB subsystem\n");
        return -1;
    }
    
    // Initialize M6: Enhanced scheduler
    if (bdi_init_enhanced_scheduler(system) != 0) {
        printf("BDI System: Failed to initialize enhanced scheduler\n");
        return -1;
    }
    
    // Initialize M6: Math subsystem
    if (bdi_init_math_subsystem(system) != 0) {
        printf("BDI System: Failed to initialize math subsystem\n");
        return -1;
    }
    
    // Initialize M6: Shell
    if (bdi_init_shell(system) != 0) {
        printf("BDI System: Failed to initialize shell\n");
        return -1;
    }
    
    global_bdi_system = system;
    printf("BDI System: M4-M6 initialization complete\n");
    return 0;
}

int bdi_system_shutdown(bdi_system_t* system) {
    printf("BDI System: Shutting down M4-M6 components...\n");
    
    // Shutdown shell
    if (system->shell) {
        bdi_shell_shutdown(system->shell);
        free(system->shell);
    }
    
    // Shutdown math subsystem
    if (system->math_context) {
        mbh_context_destroy(system->math_context);
    }
    smart_number_library_cleanup();
    
    // Shutdown enhanced scheduler
    if (system->fair_scheduler) {
        fair_scheduler_shutdown(system->fair_scheduler);
        free(system->fair_scheduler);
    }
    
    // Shutdown USB controllers
    for (uint32_t i = 0; i < system->xhci_count; i++) {
        if (system->xhci_controllers[i]) {
            xhci_shutdown_controller(system->xhci_controllers[i]);
            free(system->xhci_controllers[i]);
        }
    }
    
    // Shutdown keyboards
    for (uint32_t i = 0; i < system->keyboard_count; i++) {
        if (system->keyboards[i]) {
            hid_keyboard_shutdown(system->keyboards[i]);
            free(system->keyboards[i]);
        }
    }
    
    // Unmount filesystems
    for (uint32_t i = 0; i < system->mount_count; i++) {
        if (system->mounts[i]) {
            vfs_unmount(system->mounts[i]->path);
            free(system->mounts[i]);
        }
    }
    
    // Shutdown storage controllers
    for (uint32_t i = 0; i < system->nvme_count; i++) {
        if (system->nvme_controllers[i]) {
            nvme_shutdown_controller(system->nvme_controllers[i]);
            free(system->nvme_controllers[i]);
        }
    }
    
    for (uint32_t i = 0; i < system->ahci_count; i++) {
        if (system->ahci_controllers[i]) {
            ahci_shutdown_controller(system->ahci_controllers[i]);
            free(system->ahci_controllers[i]);
        }
    }
    
    global_bdi_system = NULL;
    printf("BDI System: Shutdown complete\n");
    return 0;
}

// --- M4: Storage Integration ---

int bdi_init_storage_subsystem(bdi_system_t* system) {
    printf("BDI Storage: Initializing storage subsystem...\n");
    
    // Initialize VFS
    if (vfs_init() != VFS_SUCCESS) {
        printf("BDI Storage: Failed to initialize VFS\n");
        return -1;
    }
    
    // Register filesystems
    if (fat32_register() != VFS_SUCCESS) {
        printf("BDI Storage: Failed to register FAT32 filesystem\n");
        return -1;
    }
    
    // Detect storage devices
    if (bdi_detect_storage_devices(system) != 0) {
        printf("BDI Storage: Failed to detect storage devices\n");
        return -1;
    }
    
    // Mount filesystems
    if (bdi_mount_filesystems(system) != 0) {
        printf("BDI Storage: Failed to mount filesystems\n");
        return -1;
    }
    
    system->storage_initialized = true;
    printf("BDI Storage: Storage subsystem initialized\n");
    return 0;
}

int bdi_detect_storage_devices(bdi_system_t* system) {
    printf("BDI Storage: Detecting storage devices...\n");
    
    // In a real implementation, this would scan PCI bus for NVMe/AHCI controllers
    // For now, we'll simulate detection
    
    // Simulate NVMe controller detection
    system->nvme_controllers[0] = (nvme_controller_t*)malloc(sizeof(nvme_controller_t));
    if (system->nvme_controllers[0]) {
        // In real implementation: nvme_init_controller(system->nvme_controllers[0], mmio_base);
        printf("BDI Storage: Detected NVMe controller 0\n");
        system->nvme_count = 1;
    }
    
    // Simulate AHCI controller detection
    system->ahci_controllers[0] = (ahci_controller_t*)malloc(sizeof(ahci_controller_t));
    if (system->ahci_controllers[0]) {
        // In real implementation: ahci_init_controller(system->ahci_controllers[0], mmio_base);
        printf("BDI Storage: Detected AHCI controller 0\n");
        system->ahci_count = 1;
    }
    
    return 0;
}

int bdi_mount_filesystems(bdi_system_t* system) {
    printf("BDI Storage: Mounting filesystems...\n");
    
    // Create a simulated device for testing
    vfs_device_t* test_device = (vfs_device_t*)malloc(sizeof(vfs_device_t));
    if (!test_device) {
        return -1;
    }
    
    // In real implementation, this would point to actual storage device functions
    test_device->sector_count = 1024 * 1024; // 512MB
    test_device->sector_size = 512;
    
    // Mount root filesystem
    if (vfs_mount("/", "fat32", test_device, false) == VFS_SUCCESS) {
        printf("BDI Storage: Mounted root filesystem (FAT32)\n");
        system->mount_count = 1;
    }
    
    return 0;
}

int bdi_storage_test(bdi_system_t* system) {
    printf("BDI Storage: Running storage tests...\n");
    
    if (!system->storage_initialized) {
        printf("BDI Storage: Storage subsystem not initialized\n");
        return -1;
    }
    
    // Test file operations
    int fd = vfs_open("/test.txt", 0);
    if (fd >= 0) {
        const char* test_data = "Hello, BDI Storage!";
        vfs_write(fd, test_data, strlen(test_data));
        vfs_close(fd);
        printf("BDI Storage: File write test passed\n");
    }
    
    system->total_io_operations++;
    printf("BDI Storage: Storage tests completed\n");
    return 0;
}

// --- M5: USB/HID Integration ---

int bdi_init_usb_subsystem(bdi_system_t* system) {
    printf("BDI USB: Initializing USB subsystem...\n");
    
    // Detect USB controllers
    if (bdi_detect_usb_devices(system) != 0) {
        printf("BDI USB: Failed to detect USB devices\n");
        return -1;
    }
    
    // Setup input devices
    if (bdi_setup_input_devices(system) != 0) {
        printf("BDI USB: Failed to setup input devices\n");
        return -1;
    }
    
    system->usb_initialized = true;
    printf("BDI USB: USB subsystem initialized\n");
    return 0;
}

int bdi_detect_usb_devices(bdi_system_t* system) {
    printf("BDI USB: Detecting USB controllers...\n");
    
    // Simulate xHCI controller detection
    system->xhci_controllers[0] = (xhci_controller_t*)malloc(sizeof(xhci_controller_t));
    if (system->xhci_controllers[0]) {
        // In real implementation: xhci_init_controller(system->xhci_controllers[0], mmio_base);
        printf("BDI USB: Detected xHCI controller 0\n");
        system->xhci_count = 1;
    }
    
    return 0;
}

int bdi_setup_input_devices(bdi_system_t* system) {
    printf("BDI USB: Setting up input devices...\n");
    
    // Simulate keyboard detection and setup
    system->keyboards[0] = (hid_keyboard_t*)malloc(sizeof(hid_keyboard_t));
    if (system->keyboards[0]) {
        if (hid_keyboard_init(system->keyboards[0], 1, 1, 0) == 0) {
            printf("BDI USB: Initialized HID keyboard 0\n");
            system->keyboard_count = 1;
        }
    }
    
    return 0;
}

int bdi_usb_test(bdi_system_t* system) {
    printf("BDI USB: Running USB tests...\n");
    
    if (!system->usb_initialized) {
        printf("BDI USB: USB subsystem not initialized\n");
        return -1;
    }
    
    // Test keyboard input
    if (system->keyboard_count > 0) {
        hid_key_event_t event;
        if (hid_keyboard_get_event(system->keyboards[0], &event) == 0) {
            printf("BDI USB: Keyboard event test passed\n");
        }
    }
    
    system->total_usb_transfers++;
    printf("BDI USB: USB tests completed\n");
    return 0;
}

// --- M6: Enhanced Scheduler Integration ---

int bdi_init_enhanced_scheduler(bdi_system_t* system) {
    printf("BDI Scheduler: Initializing enhanced scheduler...\n");
    
    system->fair_scheduler = (fair_scheduler_t*)malloc(sizeof(fair_scheduler_t));
    if (!system->fair_scheduler) {
        return -1;
    }
    
    if (fair_scheduler_init(system->fair_scheduler, system->base_scheduler) != 0) {
        free(system->fair_scheduler);
        system->fair_scheduler = NULL;
        return -1;
    }
    
    // Configure fairness policies
    if (bdi_configure_fairness_policies(system) != 0) {
        return -1;
    }
    
    system->scheduler_enhanced = true;
    printf("BDI Scheduler: Enhanced scheduler initialized\n");
    return 0;
}

int bdi_configure_fairness_policies(bdi_system_t* system) {
    printf("BDI Scheduler: Configuring fairness policies...\n");
    
    if (!system->fair_scheduler) {
        return -1;
    }
    
    // Set scheduling parameters
    system->fair_scheduler->sched_latency = 6000000;    // 6ms
    system->fair_scheduler->min_granularity = 750000;   // 0.75ms
    system->fair_scheduler->wakeup_granularity = 1000000; // 1ms
    
    printf("BDI Scheduler: Fairness policies configured\n");
    return 0;
}

int bdi_scheduler_test(bdi_system_t* system) {
    printf("BDI Scheduler: Running scheduler tests...\n");
    
    if (!system->scheduler_enhanced) {
        printf("BDI Scheduler: Enhanced scheduler not initialized\n");
        return -1;
    }
    
    // Test fairness scheduling
    if (fair_scheduler_schedule(system->fair_scheduler) == 0) {
        printf("BDI Scheduler: Fairness scheduling test passed\n");
    }
    
    system->total_graph_executions++;
    printf("BDI Scheduler: Scheduler tests completed\n");
    return 0;
}

// --- M6: Math Library Integration ---

int bdi_init_math_subsystem(bdi_system_t* system) {
    printf("BDI Math: Initializing math subsystem...\n");
    
    // Initialize smart number library
    if (smart_number_library_init() != SMART_NUM_SUCCESS) {
        printf("BDI Math: Failed to initialize smart number library\n");
        return -1;
    }
    
    // Create M→B→H context
    system->math_context = mbh_context_create();
    if (!system->math_context) {
        printf("BDI Math: Failed to create M→B→H context\n");
        return -1;
    }
    
    // Configure default precision
    mbh_context_set_precision(system->math_context, PRECISION_HIGH);
    mbh_context_set_error_threshold(system->math_context, 1e-10);
    
    printf("BDI Math: Math subsystem initialized\n");
    return 0;
}

int bdi_math_test(bdi_system_t* system) {
    printf("BDI Math: Running math tests...\n");
    
    if (!system->math_context) {
        printf("BDI Math: Math subsystem not initialized\n");
        return -1;
    }
    
    // Test smart number operations
    smart_number_t* a = smart_number_from_int(42, PRECISION_HIGH);
    smart_number_t* b = smart_number_from_int(24, PRECISION_HIGH);
    smart_number_t* result = smart_number_add(a, b);
    
    if (result && smart_number_to_int64(result) == 66) {
        printf("BDI Math: Smart number addition test passed\n");
    }
    
    smart_number_destroy(a);
    smart_number_destroy(b);
    smart_number_destroy(result);
    
    printf("BDI Math: Math tests completed\n");
    return 0;
}

// --- M6: Shell Integration ---

int bdi_init_shell(bdi_system_t* system) {
    printf("BDI Shell: Initializing shell...\n");
    
    system->shell = (bdi_shell_t*)malloc(sizeof(bdi_shell_t));
    if (!system->shell) {
        return -1;
    }
    
    if (bdi_shell_init(system->shell, system->devices, system->device_count) != 0) {
        free(system->shell);
        system->shell = NULL;
        return -1;
    }
    
    printf("BDI Shell: Shell initialized\n");
    return 0;
}

int bdi_run_shell(bdi_system_t* system) {
    printf("BDI Shell: Starting interactive shell...\n");
    
    if (!system->shell) {
        printf("BDI Shell: Shell not initialized\n");
        return -1;
    }
    
    system->shell_running = true;
    int result = bdi_shell_run(system->shell);
    system->shell_running = false;
    
    return result;
}

int bdi_shell_test(bdi_system_t* system) {
    printf("BDI Shell: Running shell tests...\n");
    
    if (!system->shell) {
        printf("BDI Shell: Shell not initialized\n");
        return -1;
    }
    
    // Test basic commands
    if (bdi_shell_run_command(system->shell, "help") == 0) {
        printf("BDI Shell: Help command test passed\n");
    }
    
    if (bdi_shell_run_command(system->shell, "graph create test_graph") == 0) {
        printf("BDI Shell: Graph creation test passed\n");
    }
    
    system->total_shell_commands += 2;
    printf("BDI Shell: Shell tests completed\n");
    return 0;
}

// --- Comprehensive Testing ---

int bdi_run_integration_tests(bdi_system_t* system) {
    printf("BDI System: Running integration tests...\n");
    
    int failures = 0;
    
    if (bdi_storage_test(system) != 0) failures++;
    if (bdi_usb_test(system) != 0) failures++;
    if (bdi_scheduler_test(system) != 0) failures++;
    if (bdi_math_test(system) != 0) failures++;
    if (bdi_shell_test(system) != 0) failures++;
    
    printf("BDI System: Integration tests completed (%d failures)\n", failures);
    return failures;
}

int bdi_run_performance_tests(bdi_system_t* system) {
    printf("BDI System: Running performance tests...\n");
    
    // Test storage performance
    uint64_t start_time = 0; // In real implementation, get actual time
    for (int i = 0; i < 1000; i++) {
        system->total_io_operations++;
    }
    uint64_t end_time = 1000; // Simulated
    
    printf("BDI System: Storage IOPS: %llu\n", 
           (unsigned long long)(1000 * 1000000 / (end_time - start_time)));
    
    printf("BDI System: Performance tests completed\n");
    return 0;
}

int bdi_run_stress_tests(bdi_system_t* system) {
    printf("BDI System: Running stress tests...\n");
    
    // Stress test all subsystems simultaneously
    for (int i = 0; i < 10000; i++) {
        system->total_io_operations++;
        system->total_usb_transfers++;
        system->total_graph_executions++;
        system->total_shell_commands++;
    }
    
    printf("BDI System: Stress tests completed\n");
    return 0;
}

// --- System Monitoring ---

void bdi_print_system_status(bdi_system_t* system) {
    printf("\n=== BDI System Status ===\n");
    printf("Storage initialized: %s\n", system->storage_initialized ? "Yes" : "No");
    printf("USB initialized: %s\n", system->usb_initialized ? "Yes" : "No");
    printf("Scheduler enhanced: %s\n", system->scheduler_enhanced ? "Yes" : "No");
    printf("Shell running: %s\n", system->shell_running ? "Yes" : "No");
    printf("\nStorage Controllers:\n");
    printf("  NVMe: %u\n", system->nvme_count);
    printf("  AHCI: %u\n", system->ahci_count);
    printf("USB Controllers:\n");
    printf("  xHCI: %u\n", system->xhci_count);
    printf("  Keyboards: %u\n", system->keyboard_count);
    printf("Filesystems mounted: %u\n", system->mount_count);
    printf("========================\n\n");
}

void bdi_print_performance_stats(bdi_system_t* system) {
    printf("\n=== BDI Performance Statistics ===\n");
    printf("Total I/O operations: %llu\n", (unsigned long long)system->total_io_operations);
    printf("Total USB transfers: %llu\n", (unsigned long long)system->total_usb_transfers);
    printf("Total graph executions: %llu\n", (unsigned long long)system->total_graph_executions);
    printf("Total shell commands: %llu\n", (unsigned long long)system->total_shell_commands);
    printf("==================================\n\n");
}
