
// ===================================================================
// DESC: Integration layer for M4-M6 components
// ===================================================================
#ifndef AEON_INTEGRATION_H
#define AEON_INTEGRATION_H

#include "graph/graph.h"
#include "scheduler/scheduler.h"
#include "scheduler/fairness.h"
#include "storage/nvme/nvme.h"
#include "storage/ahci/ahci.h"
#include "fs/vfs/vfs.h"
#include "fs/fat32/fat32.h"
#include "usb/xhci/xhci.h"
#include "usb/hid/hid_keyboard.h"
#include "math/smart_number.h"
#include "../userland/shell/bdi_shell.h"

// --- System Integration Structure ---
typedef struct {
    // Core components (M0-M3)
    BdiGraph* main_graph;
    Scheduler* base_scheduler;
    DeviceVTable** devices;
    size_t device_count;
    
    // M4: Storage subsystem
    nvme_controller_t* nvme_controllers[4];
    uint32_t nvme_count;
    ahci_controller_t* ahci_controllers[4];
    uint32_t ahci_count;
    
    // M4: Filesystem layer
    vfs_mount_t* mounts[16];
    uint32_t mount_count;
    
    // M5: USB/HID subsystem
    xhci_controller_t* xhci_controllers[4];
    uint32_t xhci_count;
    hid_keyboard_t* keyboards[8];
    uint32_t keyboard_count;
    
    // M6: Enhanced scheduling
    fair_scheduler_t* fair_scheduler;
    
    // M6: Smart number context
    mbh_context_t* math_context;
    
    // M6: BDI Shell
    bdi_shell_t* shell;
    
    // System state
    bool storage_initialized;
    bool usb_initialized;
    bool scheduler_enhanced;
    bool shell_running;
    
    // Statistics
    uint64_t total_io_operations;
    uint64_t total_usb_transfers;
    uint64_t total_graph_executions;
    uint64_t total_shell_commands;
    
} bdi_system_t;

// --- Integration Functions ---

// System initialization
int bdi_system_init(bdi_system_t* system);
int bdi_system_shutdown(bdi_system_t* system);

// M4: Storage integration
int bdi_init_storage_subsystem(bdi_system_t* system);
int bdi_detect_storage_devices(bdi_system_t* system);
int bdi_mount_filesystems(bdi_system_t* system);
int bdi_storage_test(bdi_system_t* system);

// M5: USB/HID integration
int bdi_init_usb_subsystem(bdi_system_t* system);
int bdi_detect_usb_devices(bdi_system_t* system);
int bdi_setup_input_devices(bdi_system_t* system);
int bdi_usb_test(bdi_system_t* system);

// M6: Enhanced scheduler integration
int bdi_init_enhanced_scheduler(bdi_system_t* system);
int bdi_configure_fairness_policies(bdi_system_t* system);
int bdi_scheduler_test(bdi_system_t* system);

// M6: Math library integration
int bdi_init_math_subsystem(bdi_system_t* system);
int bdi_math_test(bdi_system_t* system);

// M6: Shell integration
int bdi_init_shell(bdi_system_t* system);
int bdi_run_shell(bdi_system_t* system);
int bdi_shell_test(bdi_system_t* system);

// Comprehensive testing
int bdi_run_integration_tests(bdi_system_t* system);
int bdi_run_performance_tests(bdi_system_t* system);
int bdi_run_stress_tests(bdi_system_t* system);

// System monitoring
void bdi_print_system_status(bdi_system_t* system);
void bdi_print_performance_stats(bdi_system_t* system);

// Global system instance
extern bdi_system_t* global_bdi_system;

#endif // AEON_INTEGRATION_H
