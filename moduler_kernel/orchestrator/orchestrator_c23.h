
// ===================================================================
// BDI Orchestrator C23 Enhancements
// Enhanced orchestrator with C23 features integration
// ===================================================================

#pragma once

#include "orchestrator.h"
#include "../ai_trainer/ai_trainer.h"

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// C23 Enhanced Orchestrator Extensions
// ===================================================================

// C23 typeof for type-safe orchestrator operations
#define BDI_ORCHESTRATOR_CAST(ptr, type) \
    ({ \
        typeof(ptr) _ptr = (ptr); \
        (_ptr && sizeof(*_ptr) >= sizeof(type)) ? (type*)_ptr : NULL; \
    })

// C23 constexpr for compile-time validation
constexpr size_t BDI_MAX_AI_TRAINERS = 16;
constexpr size_t BDI_MAX_C23_MODULES = 256;

// ===================================================================
// C23 Enhanced Orchestrator State
// ===================================================================

typedef struct {
    // Base orchestrator
    bdi_orchestrator_t* base_orchestrator;
    
    // AI trainer integration
    bdi_ai_trainer_t* ai_trainers[BDI_MAX_AI_TRAINERS];
    size_t num_ai_trainers;
    
    // C23 enhanced module tracking with _BitInt
    _BitInt(256) active_modules_mask;       // Bitmask of active modules
    _BitInt(128) c23_feature_mask;          // C23 features in use
    
    // Performance metrics with C23 _Decimal precision
    _Decimal64 c23_performance_boost;       // Performance improvement from C23
    _Decimal64 ai_training_efficiency;      // AI training efficiency metric
    
    // Thread-local orchestrator state
    thread_local bool orchestrator_thread_ready;
    thread_local size_t thread_local_operations;
    
} bdi_orchestrator_c23_t;

// ===================================================================
// C23 Feature Detection and Optimization
// ===================================================================

typedef struct {
    // C23 language features
    bool has_typeof;
    bool has_auto;
    bool has_bitint;
    bool has_decimal;
    bool has_constexpr;
    bool has_thread_local;
    bool has_alignas;
    bool has_generic;
    
    // Compiler-specific C23 extensions
    bool has_builtin_assume_aligned;
    bool has_builtin_unreachable;
    bool has_attribute_assume;
    
    // Hardware features for C23 optimization
    bool has_avx512;
    bool has_amx;
    bool has_sve;
    bool has_risc_v_vector;
    
} bdi_c23_capabilities_t;

// ===================================================================
// C23 Enhanced Module Interface
// ===================================================================

typedef struct {
    // Base module interface
    bdi_module_ops_t base_ops;
    
    // C23 enhanced operations
    bool (*probe_c23)(const bdi_caps_t* caps, const bdi_c23_capabilities_t* c23_caps);
    bool (*optimize_with_c23)(bdi_module_t* module, const bdi_c23_capabilities_t* c23_caps);
    
    // AI trainer integration
    bool (*integrate_ai_trainer)(bdi_module_t* module, bdi_ai_trainer_t* trainer);
    bool (*provide_training_data)(bdi_module_t* module, bdi_ai_trainer_t* trainer);
    
    // C23 typeof-based generic operations
    void* (*generic_operation)(bdi_module_t* module, const char* op_name, void* args);
    
} bdi_module_c23_ops_t;

// ===================================================================
// C23 Enhanced Function Declarations
// ===================================================================

// Orchestrator creation with C23 features
bdi_orchestrator_c23_t* bdi_orchestrator_c23_create(const bdi_optimization_profile_t* profile);
void bdi_orchestrator_c23_destroy(bdi_orchestrator_c23_t* orch);

// C23 capability detection
bool bdi_detect_c23_capabilities(bdi_c23_capabilities_t* caps);
void bdi_print_c23_capabilities(const bdi_c23_capabilities_t* caps);

// AI trainer integration
bool bdi_orchestrator_add_ai_trainer(bdi_orchestrator_c23_t* orch, 
                                     bdi_ai_trainer_t* trainer);
bool bdi_orchestrator_train_ai_models(bdi_orchestrator_c23_t* orch);

// C23 module optimization
bool bdi_orchestrator_optimize_modules_c23(bdi_orchestrator_c23_t* orch);
bool bdi_orchestrator_apply_c23_features(bdi_orchestrator_c23_t* orch);

// Performance monitoring with C23 precision
_Decimal64 bdi_orchestrator_measure_c23_performance(bdi_orchestrator_c23_t* orch);
void bdi_orchestrator_update_c23_metrics(bdi_orchestrator_c23_t* orch);

// ===================================================================
// C23 Generic Programming Support
// ===================================================================

// C23 _Generic for type-safe orchestrator operations
#define bdi_orchestrator_configure(orch, config) _Generic((config), \
    bdi_optimization_profile_t*: bdi_orchestrator_configure_profile, \
    bdi_ai_trainer_config_t*: bdi_orchestrator_configure_ai_trainer, \
    bdi_c23_capabilities_t*: bdi_orchestrator_configure_c23, \
    default: bdi_orchestrator_configure_generic \
)(orch, config)

// Configuration functions for different types
bool bdi_orchestrator_configure_profile(bdi_orchestrator_c23_t* orch, 
                                        const bdi_optimization_profile_t* profile);
bool bdi_orchestrator_configure_ai_trainer(bdi_orchestrator_c23_t* orch,
                                           const bdi_ai_trainer_config_t* config);
bool bdi_orchestrator_configure_c23(bdi_orchestrator_c23_t* orch,
                                    const bdi_c23_capabilities_t* caps);
bool bdi_orchestrator_configure_generic(bdi_orchestrator_c23_t* orch, const void* config);

// ===================================================================
// C23 Auto Type Deduction Helpers
// ===================================================================

// C23 auto for automatic type deduction in orchestrator operations
#define BDI_AUTO_CONFIGURE_MODULE(orch, module_name, config) \
    ({ \
        auto _module = bdi_find_module_by_role(module_name); \
        auto _result = bdi_orchestrator_configure(_module, config); \
        _result; \
    })

// C23 typeof for safe module casting
#define BDI_SAFE_MODULE_CAST(module, target_type) \
    ({ \
        typeof(module) _mod = (module); \
        (_mod && _mod->api) ? (target_type*)_mod->api : NULL; \
    })

// ===================================================================
// C23 Constexpr Validation Macros
// ===================================================================

// Compile-time validation of orchestrator configuration
#define BDI_VALIDATE_ORCHESTRATOR_CONFIG(config) \
    static_assert(sizeof(config) > 0, "Configuration cannot be empty"); \
    static_assert(offsetof(typeof(config), type) == 0, "Type must be first field")

// C23 constexpr functions for validation
constexpr bool bdi_is_valid_ai_trainer_count(size_t count) {
    return count > 0 && count <= BDI_MAX_AI_TRAINERS;
}

constexpr bool bdi_is_valid_c23_feature_mask(_BitInt(128) mask) {
    return mask != 0; // At least one C23 feature should be enabled
}

// ===================================================================
// Thread-Local Orchestrator State Management
// ===================================================================

// C23 thread_local storage for orchestrator operations
thread_local bdi_orchestrator_c23_t* current_orchestrator;
thread_local size_t thread_operation_count;
thread_local _Decimal64 thread_performance_score;

// Thread-local state management functions
void bdi_orchestrator_init_thread_state(bdi_orchestrator_c23_t* orch);
void bdi_orchestrator_cleanup_thread_state(void);
bdi_orchestrator_c23_t* bdi_orchestrator_get_current(void);

// ===================================================================
// C23 BitInt Operations for Module Management
// ===================================================================

// Module mask operations using C23 _BitInt
static inline void bdi_set_module_active(bdi_orchestrator_c23_t* orch, size_t module_id) {
    if (module_id < 256) {
        orch->active_modules_mask |= ((_BitInt(256))1 << module_id);
    }
}

static inline void bdi_set_module_inactive(bdi_orchestrator_c23_t* orch, size_t module_id) {
    if (module_id < 256) {
        orch->active_modules_mask &= ~((_BitInt(256))1 << module_id);
    }
}

static inline bool bdi_is_module_active(const bdi_orchestrator_c23_t* orch, size_t module_id) {
    if (module_id >= 256) return false;
    return (orch->active_modules_mask & ((_BitInt(256))1 << module_id)) != 0;
}

// C23 feature mask operations
static inline void bdi_enable_c23_feature(bdi_orchestrator_c23_t* orch, size_t feature_id) {
    if (feature_id < 128) {
        orch->c23_feature_mask |= ((_BitInt(128))1 << feature_id);
    }
}

static inline bool bdi_is_c23_feature_enabled(const bdi_orchestrator_c23_t* orch, size_t feature_id) {
    if (feature_id >= 128) return false;
    return (orch->c23_feature_mask & ((_BitInt(128))1 << feature_id)) != 0;
}

// ===================================================================
// C23 Decimal Arithmetic for Performance Metrics
// ===================================================================

// High-precision performance calculations using C23 _Decimal
static inline _Decimal64 bdi_calculate_performance_improvement(
    _Decimal64 baseline_performance,
    _Decimal64 enhanced_performance) {
    
    if (baseline_performance == 0.0dd) return 0.0dd;
    return (enhanced_performance - baseline_performance) / baseline_performance * 100.0dd;
}

static inline _Decimal64 bdi_calculate_ai_efficiency(
    size_t samples_processed,
    uint64_t training_time_us,
    _Decimal64 accuracy) {
    
    if (training_time_us == 0) return 0.0dd;
    _Decimal64 samples_per_second = (_Decimal64)samples_processed * 1000000.0dd / (_Decimal64)training_time_us;
    return samples_per_second * accuracy;
}

// ===================================================================
// Integration with Existing BDI Systems
// ===================================================================

// Integration with existing orchestrator
bool bdi_orchestrator_c23_integrate_legacy(bdi_orchestrator_c23_t* c23_orch,
                                           bdi_orchestrator_t* legacy_orch);

// Integration with attention memory manager
bool bdi_orchestrator_c23_integrate_attention_mm(bdi_orchestrator_c23_t* orch,
                                                 bdi_attention_mm_t* mm);

// Integration with capability system
bool bdi_orchestrator_c23_update_capabilities(bdi_orchestrator_c23_t* orch,
                                              const bdi_caps_t* caps);

#ifdef __cplusplus
}
#endif

