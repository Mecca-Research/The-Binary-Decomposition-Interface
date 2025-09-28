
// ===================================================================
// BDI Orchestrator - Probe→Plan→Compose→Prove Pipeline
// Main coordination system for modular kernel assembly
// ===================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "../capgraph/capability.h"
#include "../bricks/module.h"
#include "../attention_mm/attention_mm.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// Orchestrator Phases
// ===================================================================

typedef enum {
    BDI_PHASE_PROBE = 0,    // Hardware capability detection
    BDI_PHASE_PLAN,         // Module selection and optimization planning
    BDI_PHASE_COMPOSE,      // System assembly and vtable binding
    BDI_PHASE_PROVE,        // Self-tests and validation
    BDI_PHASE_RUNNING,      // Normal operation
    BDI_PHASE_ERROR         // Error state
} bdi_orchestrator_phase_t;

// ===================================================================
// Optimization Profiles
// ===================================================================

typedef enum {
    BDI_PROFILE_LATENCY = 0,    // Optimize for low latency
    BDI_PROFILE_THROUGHPUT,     // Optimize for high throughput
    BDI_PROFILE_ENERGY,         // Optimize for energy efficiency
    BDI_PROFILE_AI_TRAIN,       // AI training workloads
    BDI_PROFILE_AI_INFERENCE,   // AI inference workloads
    BDI_PROFILE_BALANCED,       // Balanced performance
    BDI_PROFILE_CUSTOM          // Custom profile from file
} bdi_profile_type_t;

typedef struct {
    bdi_profile_type_t type;
    char name[64];
    
    // Optimization objectives
    float weight_latency;       // Weight for latency optimization
    float weight_throughput;    // Weight for throughput optimization
    float weight_energy;        // Weight for energy efficiency
    float weight_memory;        // Weight for memory efficiency
    
    // Constraints
    uint32_t min_freq_mhz;      // Minimum CPU frequency
    uint32_t max_power_watts;   // Maximum power consumption
    bool prefer_hugepages;      // Prefer large pages
    bool numa_spread;           // Spread across NUMA nodes
    
    // Module preferences
    char preferred_modules[16][64];  // Preferred modules by role
    uint32_t preferred_count;
    
    // Memory management policy
    char mm_policy[32];         // Memory management policy name
    bdi_attention_config_t attention_config;  // Attention MM configuration
    
} bdi_optimization_profile_t;

// ===================================================================
// System Composition Plan
// ===================================================================

typedef struct {
    // Selected modules by role
    struct {
        char role[32];
        bdi_module_t* selected_module;
        bdi_module_t* fallback_module;
        float selection_score;
    } module_assignments[64];
    uint32_t assignment_count;
    
    // μABI binding plan
    struct {
        char operation[32];
        void* implementation;
        char source_module[64];
        uint32_t estimated_cycles;
    } uabi_bindings[128];
    uint32_t binding_count;
    
    // Memory layout plan
    struct {
        bdi_memory_pool_t pool_type;
        size_t size_bytes;
        uint32_t numa_node;
        uint32_t flags;
    } memory_layout[16];
    uint32_t layout_count;
    
    // Performance predictions
    float predicted_latency_us;     // Predicted average latency
    float predicted_throughput_ops; // Predicted throughput
    float predicted_power_watts;    // Predicted power consumption
    
    // Cache keys for optimization artifacts
    uint64_t capability_digest;     // Hash of capabilities
    uint64_t profile_digest;        // Hash of profile
    uint64_t plan_digest;           // Hash of complete plan
    
} bdi_composition_plan_t;

// ===================================================================
// Orchestrator State & Context
// ===================================================================

typedef struct {
    // Current phase and status
    bdi_orchestrator_phase_t current_phase;
    bool initialization_complete;
    bool hot_swap_in_progress;
    uint64_t boot_start_time;
    uint64_t phase_start_time;
    
    // Detected capabilities
    bdi_caps_t capabilities;
    uint64_t capability_digest;
    
    // Active profile and plan
    bdi_optimization_profile_t active_profile;
    bdi_composition_plan_t composition_plan;
    
    // Module registry
    bdi_module_registry_t* module_registry;
    
    // Memory manager
    bdi_attention_mm_t* memory_manager;
    
    // Performance monitoring
    struct {
        uint64_t total_boot_time_us;
        uint64_t probe_time_us;
        uint64_t plan_time_us;
        uint64_t compose_time_us;
        uint64_t prove_time_us;
        
        uint32_t successful_boots;
        uint32_t failed_boots;
        uint32_t hot_swaps_completed;
        uint32_t hot_swaps_failed;
        
        float avg_latency_us;
        float avg_throughput_ops;
        float avg_power_watts;
    } performance_stats;
    
    // Error tracking
    uint32_t error_count;
    char last_error[256];
    
} bdi_orchestrator_t;

// ===================================================================
// Orchestrator Lifecycle Functions
// ===================================================================

// Create and initialize orchestrator
bdi_orchestrator_t* bdi_orchestrator_create(void);
void bdi_orchestrator_destroy(bdi_orchestrator_t* orch);

// Main boot sequence - executes full Probe→Plan→Compose→Prove pipeline
bool bdi_orchestrator_boot(bdi_orchestrator_t* orch, const char* profile_name);

// Individual phase execution
bool bdi_orchestrator_probe(bdi_orchestrator_t* orch);
bool bdi_orchestrator_plan(bdi_orchestrator_t* orch, const char* profile_name);
bool bdi_orchestrator_compose(bdi_orchestrator_t* orch);
bool bdi_orchestrator_prove(bdi_orchestrator_t* orch);

// Runtime operations
bool bdi_orchestrator_start_runtime(bdi_orchestrator_t* orch);
void bdi_orchestrator_shutdown(bdi_orchestrator_t* orch);

// ===================================================================
// Profile Management
// ===================================================================

// Load optimization profile from file or built-in
bool bdi_load_profile(bdi_orchestrator_t* orch, const char* profile_name);
bool bdi_load_profile_from_file(bdi_orchestrator_t* orch, const char* filename);

// Built-in profiles
extern const bdi_optimization_profile_t bdi_profile_latency;
extern const bdi_optimization_profile_t bdi_profile_throughput;
extern const bdi_optimization_profile_t bdi_profile_energy;
extern const bdi_optimization_profile_t bdi_profile_ai_train;
extern const bdi_optimization_profile_t bdi_profile_ai_inference;
extern const bdi_optimization_profile_t bdi_profile_balanced;

// Profile utilities
void bdi_print_profile(const bdi_optimization_profile_t* profile);
bool bdi_validate_profile(const bdi_optimization_profile_t* profile);

// ===================================================================
// Module Selection & Planning
// ===================================================================

// Module selection algorithms
bdi_module_t* bdi_select_module_greedy(const char* role, const bdi_caps_t* caps, 
                                      const bdi_optimization_profile_t* profile);
bdi_module_t* bdi_select_module_optimal(const char* role, const bdi_caps_t* caps,
                                       const bdi_optimization_profile_t* profile);

// Plan generation and optimization
bool bdi_generate_composition_plan(bdi_orchestrator_t* orch);
bool bdi_optimize_composition_plan(bdi_orchestrator_t* orch);
bool bdi_validate_composition_plan(bdi_orchestrator_t* orch);

// Plan caching and retrieval
bool bdi_cache_composition_plan(const bdi_composition_plan_t* plan);
bool bdi_load_cached_plan(bdi_orchestrator_t* orch, uint64_t capability_digest, 
                         uint64_t profile_digest);

// ===================================================================
// System Composition & Assembly
// ===================================================================

// μABI binding and vtable setup
bool bdi_bind_uabi_operations(bdi_orchestrator_t* orch);
bool bdi_setup_module_vtables(bdi_orchestrator_t* orch);

// Memory layout and pool setup
bool bdi_setup_memory_pools(bdi_orchestrator_t* orch);
bool bdi_configure_attention_mm(bdi_orchestrator_t* orch);

// Interrupt and timer setup
bool bdi_setup_interrupt_handlers(bdi_orchestrator_t* orch);
bool bdi_setup_timer_subsystem(bdi_orchestrator_t* orch);

// Cache warming and prefetching
bool bdi_warm_caches(bdi_orchestrator_t* orch);
bool bdi_setup_prefetching(bdi_orchestrator_t* orch);

// ===================================================================
// Self-Testing & Validation (Prove Phase)
// ===================================================================

// Comprehensive system self-tests
bool bdi_prove_system_integrity(bdi_orchestrator_t* orch);

// Individual component tests
bool bdi_test_uabi_operations(bdi_orchestrator_t* orch);
bool bdi_test_module_interfaces(bdi_orchestrator_t* orch);
bool bdi_test_memory_manager(bdi_orchestrator_t* orch);
bool bdi_test_performance_targets(bdi_orchestrator_t* orch);

// Security and safety validation
bool bdi_validate_cfi_integrity(bdi_orchestrator_t* orch);
bool bdi_validate_module_signatures(bdi_orchestrator_t* orch);
bool bdi_validate_memory_protection(bdi_orchestrator_t* orch);

// Microbenchmarks and smoke tests
bool bdi_run_microbenchmarks(bdi_orchestrator_t* orch);
bool bdi_run_smoke_tests(bdi_orchestrator_t* orch);

// ===================================================================
// Hot-Swap Coordination
// ===================================================================

// Initiate system-wide hot-swap
bool bdi_orchestrator_hotswap_begin(bdi_orchestrator_t* orch, const char* role, 
                                   bdi_module_t* new_module);

// Hot-swap phases
bool bdi_orchestrator_hotswap_quiesce(bdi_orchestrator_t* orch);
bool bdi_orchestrator_hotswap_transfer(bdi_orchestrator_t* orch);
bool bdi_orchestrator_hotswap_activate(bdi_orchestrator_t* orch);
bool bdi_orchestrator_hotswap_verify(bdi_orchestrator_t* orch);

// Hot-swap completion and rollback
bool bdi_orchestrator_hotswap_commit(bdi_orchestrator_t* orch);
bool bdi_orchestrator_hotswap_rollback(bdi_orchestrator_t* orch);

// ===================================================================
// Performance Monitoring & Adaptation
// ===================================================================

// Runtime performance monitoring
void bdi_orchestrator_update_performance_stats(bdi_orchestrator_t* orch);
void bdi_orchestrator_log_performance_event(bdi_orchestrator_t* orch, 
                                           const char* event, uint64_t value);

// Adaptive optimization
bool bdi_orchestrator_adapt_to_workload(bdi_orchestrator_t* orch);
bool bdi_orchestrator_rebalance_modules(bdi_orchestrator_t* orch);

// Performance reporting
void bdi_print_orchestrator_stats(const bdi_orchestrator_t* orch);
void bdi_export_performance_data(const bdi_orchestrator_t* orch, const char* filename);

// ===================================================================
// Error Handling & Recovery
// ===================================================================

// Error reporting and logging
void bdi_orchestrator_report_error(bdi_orchestrator_t* orch, const char* error_msg);
const char* bdi_orchestrator_get_last_error(const bdi_orchestrator_t* orch);

// Recovery mechanisms
bool bdi_orchestrator_recover_from_error(bdi_orchestrator_t* orch);
bool bdi_orchestrator_fallback_to_safe_mode(bdi_orchestrator_t* orch);

// Panic handling
void bdi_orchestrator_panic_handler(bdi_orchestrator_t* orch, const char* reason);

// ===================================================================
// Integration with Existing BDI Kernel
// ===================================================================

// Integration with existing BDI graph system
bool bdi_orchestrator_integrate_graph_system(bdi_orchestrator_t* orch, void* bdi_graph);
bool bdi_orchestrator_integrate_scheduler(bdi_orchestrator_t* orch, void* scheduler);
bool bdi_orchestrator_integrate_ham(bdi_orchestrator_t* orch, void* ham_vtable);

// Legacy compatibility
bool bdi_orchestrator_enable_legacy_mode(bdi_orchestrator_t* orch);
void bdi_orchestrator_migrate_from_legacy(bdi_orchestrator_t* orch);

#ifdef __cplusplus
}
#endif
