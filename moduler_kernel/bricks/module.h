
// ===================================================================
// BDI Module System - Hot-Swappable Kernel "Bricks"
// Dynamic module loading with capability-based selection
// ===================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../capgraph/capability.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// Module Manifest & Metadata
// ===================================================================

#define BDI_MODULE_NAME_MAX 64
#define BDI_MODULE_ROLE_MAX 32
#define BDI_MODULE_VERSION_MAX 16
#define BDI_MODULE_HASH_SIZE 32

typedef struct {
    char name[BDI_MODULE_NAME_MAX];         // Module name (e.g., "memcpy.avx2")
    char role[BDI_MODULE_ROLE_MAX];         // Module role (e.g., "uop.memcpy")
    char version[BDI_MODULE_VERSION_MAX];   // Module version (e.g., "1.0.0")
    
    // Requirements
    char required_caps[256];                // Required capabilities (comma-separated)
    char preferred_profile[64];             // Preferred optimization profile
    uint32_t min_freq_mhz;                  // Minimum CPU frequency required
    uint32_t min_memory_mb;                 // Minimum memory required
    
    // Security & Verification
    uint8_t hash[BDI_MODULE_HASH_SIZE];     // SHA-256 hash of module
    uint32_t signature_offset;              // Offset to digital signature
    uint32_t signature_size;                // Size of digital signature
    
    // Performance characteristics
    uint32_t estimated_cycles;              // Estimated cycles for typical operation
    uint32_t memory_footprint;              // Memory footprint (bytes)
    float performance_score;                // Relative performance score
    
} bdi_module_manifest_t;

// ===================================================================
// Module Interface & Lifecycle
// ===================================================================

typedef struct bdi_module bdi_module_t;

typedef struct {
    // Lifecycle functions
    bool (*probe)(const bdi_caps_t* caps, const char* profile);
    bool (*init)(bdi_module_t* module);
    bool (*start)(bdi_module_t* module);
    bool (*stop)(bdi_module_t* module);
    void (*cleanup)(bdi_module_t* module);
    
    // Hot-swap support
    bool (*quiesce)(bdi_module_t* module);      // Prepare for hot-swap
    bool (*transfer_state)(bdi_module_t* old_module, bdi_module_t* new_module);
    bool (*resume)(bdi_module_t* module);       // Resume after hot-swap
    
    // Self-test and validation
    bool (*self_test)(bdi_module_t* module);
    bool (*verify_integrity)(bdi_module_t* module);
    
} bdi_module_ops_t;

struct bdi_module {
    bdi_module_manifest_t manifest;
    bdi_module_ops_t ops;
    
    // Module state
    enum {
        BDI_MODULE_STATE_UNLOADED,
        BDI_MODULE_STATE_LOADED,
        BDI_MODULE_STATE_INITIALIZED,
        BDI_MODULE_STATE_RUNNING,
        BDI_MODULE_STATE_QUIESCED,
        BDI_MODULE_STATE_ERROR
    } state;
    
    // API vtable - role-specific function pointers
    void* api;                              // Points to role-specific vtable
    
    // Runtime information
    uint64_t load_time;                     // When module was loaded
    uint64_t start_time;                    // When module started
    uint32_t reference_count;               // Number of active references
    uint32_t error_count;                   // Number of errors encountered
    
    // Performance statistics
    uint64_t call_count;                    // Number of API calls
    uint64_t total_cycles;                  // Total cycles consumed
    uint64_t max_cycles;                    // Maximum cycles for single call
    
    // Private module data
    void* private_data;                     // Module-specific private data
    size_t private_data_size;               // Size of private data
};

// ===================================================================
// Module Registry & Management
// ===================================================================

typedef struct {
    bdi_module_t* modules[256];             // Array of loaded modules
    uint32_t module_count;                  // Number of loaded modules
    
    // Role-based lookup table
    struct {
        char role[BDI_MODULE_ROLE_MAX];
        bdi_module_t* active_module;        // Currently active module for role
        bdi_module_t* candidates[16];       // Candidate modules for role
        uint32_t candidate_count;
    } roles[64];
    uint32_t role_count;
    
    // Hot-swap coordination
    bool hot_swap_in_progress;
    char hot_swap_role[BDI_MODULE_ROLE_MAX];
    bdi_module_t* hot_swap_old;
    bdi_module_t* hot_swap_new;
    
} bdi_module_registry_t;

// Global module registry
extern bdi_module_registry_t bdi_module_registry;

// ===================================================================
// Module Loading & Registration
// ===================================================================

// Load module from file or memory
bdi_module_t* bdi_load_module(const char* path);
bdi_module_t* bdi_load_module_from_memory(const void* data, size_t size);

// Register module with the system
bool bdi_register_module(bdi_module_t* module);
bool bdi_unregister_module(bdi_module_t* module);

// Find modules by role or capability
bdi_module_t* bdi_find_module_by_role(const char* role);
bdi_module_t** bdi_find_modules_by_capability(const char* capability, uint32_t* count);

// Module selection based on capabilities and profile
bdi_module_t* bdi_select_best_module(const char* role, const bdi_caps_t* caps, const char* profile);

// ===================================================================
// Hot-Swap Operations
// ===================================================================

typedef enum {
    BDI_HOTSWAP_OK = 0,
    BDI_HOTSWAP_ERROR_MODULE_BUSY,
    BDI_HOTSWAP_ERROR_QUIESCE_FAILED,
    BDI_HOTSWAP_ERROR_STATE_TRANSFER_FAILED,
    BDI_HOTSWAP_ERROR_NEW_MODULE_FAILED,
    BDI_HOTSWAP_ERROR_ROLLBACK_FAILED
} bdi_hotswap_result_t;

// Initiate hot-swap of module for given role
bdi_hotswap_result_t bdi_hotswap_module(const char* role, bdi_module_t* new_module);

// Check if hot-swap is possible for role
bool bdi_can_hotswap_role(const char* role);

// Get hot-swap status
bool bdi_is_hotswap_in_progress(void);
const char* bdi_get_hotswap_role(void);

// ===================================================================
// Module API Definitions for Common Roles
// ===================================================================

// μABI operation module API
typedef struct {
    void (*memcpy_fast)(void* dst, const void* src, size_t n);
    void (*memset_fast)(void* dst, int c, size_t n);
    // ... other μABI operations
} bdi_uop_module_api_t;

// Memory management module API
typedef struct {
    void* (*alloc)(size_t size, uint32_t flags);
    void (*free)(void* ptr);
    void* (*realloc)(void* ptr, size_t new_size);
    bool (*set_attention)(void* ptr, float attention_score);
    // ... other memory operations
} bdi_mm_module_api_t;

// Scheduler module API
typedef struct {
    bool (*schedule)(void);
    bool (*add_task)(void* task);
    bool (*remove_task)(void* task);
    void (*set_priority)(void* task, int priority);
    // ... other scheduler operations
} bdi_sched_module_api_t;

// ===================================================================
// Module Utilities & Helpers
// ===================================================================

// Verify module signature and integrity
bool bdi_verify_module_signature(const bdi_module_t* module);
bool bdi_verify_module_hash(const bdi_module_t* module);

// Module performance profiling
void bdi_profile_module_start(bdi_module_t* module);
void bdi_profile_module_end(bdi_module_t* module);
void bdi_get_module_stats(const bdi_module_t* module, char* buffer, size_t buffer_size);

// Module dependency management
bool bdi_check_module_dependencies(const bdi_module_t* module, const bdi_caps_t* caps);
void bdi_resolve_module_dependencies(bdi_module_t* module);

// Error handling
const char* bdi_module_error_string(int error_code);
void bdi_log_module_error(const bdi_module_t* module, const char* error_msg);

// ===================================================================
// Module Development Helpers
// ===================================================================

// Macros for defining modules
#define BDI_MODULE_DEFINE(name, role, version) \
    static bdi_module_manifest_t _module_manifest = { \
        .name = name, \
        .role = role, \
        .version = version \
    }; \
    static bdi_module_t _module = { \
        .manifest = _module_manifest \
    };

#define BDI_MODULE_EXPORT(module_ptr) \
    bdi_module_t* bdi_module_get(void) { return module_ptr; }

// Helper for implementing probe function
#define BDI_MODULE_PROBE_CAPS(caps_check) \
    static bool _module_probe(const bdi_caps_t* caps, const char* profile) { \
        return (caps_check); \
    }

#ifdef __cplusplus
}
#endif
