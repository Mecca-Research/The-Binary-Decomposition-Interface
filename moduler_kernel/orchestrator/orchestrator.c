
// ===================================================================
// BDI Orchestrator Implementation
// Probe→Plan→Compose→Prove Pipeline Coordination
// ===================================================================

#include "orchestrator.h"
#include "../uabi/uops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// ===================================================================
// Built-in Optimization Profiles
// ===================================================================

const bdi_optimization_profile_t bdi_profile_latency = {
    .type = BDI_PROFILE_LATENCY,
    .name = "latency",
    .weight_latency = 1.0f,
    .weight_throughput = 0.2f,
    .weight_energy = 0.1f,
    .weight_memory = 0.3f,
    .min_freq_mhz = 3000,
    .max_power_watts = 200,
    .prefer_hugepages = true,
    .numa_spread = false,
    .preferred_count = 3,
    .mm_policy = "attention",
    .attention_config = {
        .attention_learning_rate = 0.02f,
        .recency_decay_rate = 0.95f,
        .hotness_learning_rate = 0.1f,
        .regularization_factor = 0.01f,
        .weight_attention = 0.6f,
        .weight_recency = 0.3f,
        .weight_hotness = 0.1f,
        .weight_numa_locality = 0.2f,
        .weight_criticality = 0.4f,
        .eviction_threshold = 0.3f,
        .promotion_threshold = 0.7f,
        .demotion_threshold = 0.2f,
        .update_frequency = 1000,
        .gc_frequency = 10000,
        .enable_prefetching = true,
        .enable_numa_balancing = true
    }
};

const bdi_optimization_profile_t bdi_profile_throughput = {
    .type = BDI_PROFILE_THROUGHPUT,
    .name = "throughput",
    .weight_latency = 0.2f,
    .weight_throughput = 1.0f,
    .weight_energy = 0.1f,
    .weight_memory = 0.4f,
    .min_freq_mhz = 2400,
    .max_power_watts = 300,
    .prefer_hugepages = true,
    .numa_spread = true,
    .preferred_count = 2,
    .mm_policy = "attention",
    .attention_config = {
        .attention_learning_rate = 0.05f,
        .recency_decay_rate = 0.98f,
        .hotness_learning_rate = 0.2f,
        .regularization_factor = 0.005f,
        .weight_attention = 0.4f,
        .weight_recency = 0.2f,
        .weight_hotness = 0.4f,
        .weight_numa_locality = 0.3f,
        .weight_criticality = 0.2f,
        .eviction_threshold = 0.2f,
        .promotion_threshold = 0.8f,
        .demotion_threshold = 0.1f,
        .update_frequency = 500,
        .gc_frequency = 5000,
        .enable_prefetching = true,
        .enable_numa_balancing = true
    }
};

const bdi_optimization_profile_t bdi_profile_ai_train = {
    .type = BDI_PROFILE_AI_TRAIN,
    .name = "ai-train",
    .weight_latency = 0.1f,
    .weight_throughput = 0.8f,
    .weight_energy = 0.1f,
    .weight_memory = 0.6f,
    .min_freq_mhz = 3500,
    .max_power_watts = 400,
    .prefer_hugepages = true,
    .numa_spread = false,
    .preferred_count = 4,
    .mm_policy = "attention",
    .attention_config = {
        .attention_learning_rate = 0.1f,
        .recency_decay_rate = 0.99f,
        .hotness_learning_rate = 0.3f,
        .regularization_factor = 0.001f,
        .weight_attention = 0.5f,
        .weight_recency = 0.1f,
        .weight_hotness = 0.4f,
        .weight_numa_locality = 0.1f,
        .weight_criticality = 0.3f,
        .eviction_threshold = 0.1f,
        .promotion_threshold = 0.9f,
        .demotion_threshold = 0.05f,
        .update_frequency = 100,
        .gc_frequency = 1000,
        .enable_prefetching = true,
        .enable_numa_balancing = false
    }
};

// ===================================================================
// Orchestrator Creation and Destruction
// ===================================================================

bdi_orchestrator_t* bdi_orchestrator_create(void) {
    bdi_orchestrator_t* orch = calloc(1, sizeof(bdi_orchestrator_t));
    if (!orch) return NULL;
    
    // Initialize state
    orch->current_phase = BDI_PHASE_PROBE;
    orch->initialization_complete = false;
    orch->hot_swap_in_progress = false;
    orch->boot_start_time = 0;
    orch->phase_start_time = 0;
    
    // Create module registry
    orch->module_registry = calloc(1, sizeof(bdi_module_registry_t));
    if (!orch->module_registry) {
        free(orch);
        return NULL;
    }
    
    return orch;
}

void bdi_orchestrator_destroy(bdi_orchestrator_t* orch) {
    if (!orch) return;
    
    // Shutdown memory manager
    if (orch->memory_manager) {
        bdi_attention_mm_destroy(orch->memory_manager);
    }
    
    // Free module registry
    if (orch->module_registry) {
        free(orch->module_registry);
    }
    
    free(orch);
}

// ===================================================================
// Timing Utilities
// ===================================================================

static uint64_t get_time_us(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

// ===================================================================
// Phase 1: Probe - Hardware Capability Detection
// ===================================================================

bool bdi_orchestrator_probe(bdi_orchestrator_t* orch) {
    if (!orch) return false;
    
    printf("BDI Orchestrator: Starting PROBE phase...\n");
    orch->current_phase = BDI_PHASE_PROBE;
    orch->phase_start_time = get_time_us();
    
    // Detect hardware capabilities
    bdi_probe_caps(&orch->capabilities);
    
    // Generate capability digest for caching
    orch->capability_digest = bdi_caps_digest(&orch->capabilities);
    
    // Validate detected capabilities
    if (!bdi_validate_caps(&orch->capabilities)) {
        snprintf(orch->last_error, sizeof(orch->last_error),
                "Invalid capabilities detected");
        orch->error_count++;
        return false;
    }
    
    // Print capability summary
    char cap_summary[2048];
    bdi_caps_to_string(&orch->capabilities, cap_summary, sizeof(cap_summary));
    printf("%s\n", cap_summary);
    
    // Record probe time
    uint64_t probe_time = get_time_us() - orch->phase_start_time;
    orch->performance_stats.probe_time_us = probe_time;
    
    printf("BDI Orchestrator: PROBE phase completed in %lu μs\n", probe_time);
    printf("  Capability digest: 0x%016lx\n", orch->capability_digest);
    
    return true;
}

// ===================================================================
// Phase 2: Plan - Module Selection and Optimization Planning
// ===================================================================

bool bdi_orchestrator_plan(bdi_orchestrator_t* orch, const char* profile_name) {
    if (!orch) return false;
    
    printf("BDI Orchestrator: Starting PLAN phase with profile '%s'...\n", 
           profile_name ? profile_name : "default");
    orch->current_phase = BDI_PHASE_PLAN;
    orch->phase_start_time = get_time_us();
    
    // Load optimization profile
    if (!bdi_load_profile(orch, profile_name)) {
        snprintf(orch->last_error, sizeof(orch->last_error),
                "Failed to load profile: %s", profile_name ? profile_name : "default");
        orch->error_count++;
        return false;
    }
    
    // Try to load cached plan first
    uint64_t profile_digest = 0; // Would hash the profile
    if (bdi_load_cached_plan(orch, orch->capability_digest, profile_digest)) {
        printf("BDI Orchestrator: Using cached composition plan\n");
    } else {
        // Generate new composition plan
        if (!bdi_generate_composition_plan(orch)) {
            snprintf(orch->last_error, sizeof(orch->last_error),
                    "Failed to generate composition plan");
            orch->error_count++;
            return false;
        }
        
        // Optimize the plan
        if (!bdi_optimize_composition_plan(orch)) {
            printf("BDI Orchestrator: Warning - plan optimization failed, using basic plan\n");
        }
        
        // Cache the plan for future use
        bdi_cache_composition_plan(&orch->composition_plan);
    }
    
    // Validate the plan
    if (!bdi_validate_composition_plan(orch)) {
        snprintf(orch->last_error, sizeof(orch->last_error),
                "Generated composition plan is invalid");
        orch->error_count++;
        return false;
    }
    
    // Record plan time
    uint64_t plan_time = get_time_us() - orch->phase_start_time;
    orch->performance_stats.plan_time_us = plan_time;
    
    printf("BDI Orchestrator: PLAN phase completed in %lu μs\n", plan_time);
    printf("  Selected %u modules, %u μABI bindings\n",
           orch->composition_plan.assignment_count,
           orch->composition_plan.binding_count);
    
    return true;
}

// ===================================================================
// Phase 3: Compose - System Assembly and Binding
// ===================================================================

bool bdi_orchestrator_compose(bdi_orchestrator_t* orch) {
    if (!orch) return false;
    
    printf("BDI Orchestrator: Starting COMPOSE phase...\n");
    orch->current_phase = BDI_PHASE_COMPOSE;
    orch->phase_start_time = get_time_us();
    
    // Setup memory pools and attention-based memory manager
    if (!bdi_setup_memory_pools(orch)) {
        snprintf(orch->last_error, sizeof(orch->last_error),
                "Failed to setup memory pools");
        orch->error_count++;
        return false;
    }
    
    if (!bdi_configure_attention_mm(orch)) {
        snprintf(orch->last_error, sizeof(orch->last_error),
                "Failed to configure attention memory manager");
        orch->error_count++;
        return false;
    }
    
    // Bind μABI operations to selected implementations
    if (!bdi_bind_uabi_operations(orch)) {
        snprintf(orch->last_error, sizeof(orch->last_error),
                "Failed to bind μABI operations");
        orch->error_count++;
        return false;
    }
    
    // Setup module vtables
    if (!bdi_setup_module_vtables(orch)) {
        snprintf(orch->last_error, sizeof(orch->last_error),
                "Failed to setup module vtables");
        orch->error_count++;
        return false;
    }
    
    // Setup interrupt handlers and timer subsystem
    if (!bdi_setup_interrupt_handlers(orch)) {
        printf("BDI Orchestrator: Warning - interrupt handler setup failed\n");
    }
    
    if (!bdi_setup_timer_subsystem(orch)) {
        printf("BDI Orchestrator: Warning - timer subsystem setup failed\n");
    }
    
    // Warm caches and setup prefetching
    if (!bdi_warm_caches(orch)) {
        printf("BDI Orchestrator: Warning - cache warming failed\n");
    }
    
    if (!bdi_setup_prefetching(orch)) {
        printf("BDI Orchestrator: Warning - prefetching setup failed\n");
    }
    
    // Record compose time
    uint64_t compose_time = get_time_us() - orch->phase_start_time;
    orch->performance_stats.compose_time_us = compose_time;
    
    printf("BDI Orchestrator: COMPOSE phase completed in %lu μs\n", compose_time);
    
    return true;
}

// ===================================================================
// Phase 4: Prove - Self-Tests and Validation
// ===================================================================

bool bdi_orchestrator_prove(bdi_orchestrator_t* orch) {
    if (!orch) return false;
    
    printf("BDI Orchestrator: Starting PROVE phase...\n");
    orch->current_phase = BDI_PHASE_PROVE;
    orch->phase_start_time = get_time_us();
    
    // Test μABI operations
    if (!bdi_test_uabi_operations(orch)) {
        snprintf(orch->last_error, sizeof(orch->last_error),
                "μABI operations failed self-test");
        orch->error_count++;
        return false;
    }
    
    // Test module interfaces
    if (!bdi_test_module_interfaces(orch)) {
        snprintf(orch->last_error, sizeof(orch->last_error),
                "Module interfaces failed self-test");
        orch->error_count++;
        return false;
    }
    
    // Test memory manager
    if (!bdi_test_memory_manager(orch)) {
        snprintf(orch->last_error, sizeof(orch->last_error),
                "Memory manager failed self-test");
        orch->error_count++;
        return false;
    }
    
    // Validate security features
    if (!bdi_validate_cfi_integrity(orch)) {
        printf("BDI Orchestrator: Warning - CFI integrity validation failed\n");
    }
    
    if (!bdi_validate_module_signatures(orch)) {
        printf("BDI Orchestrator: Warning - module signature validation failed\n");
    }
    
    if (!bdi_validate_memory_protection(orch)) {
        printf("BDI Orchestrator: Warning - memory protection validation failed\n");
    }
    
    // Run performance tests
    if (!bdi_test_performance_targets(orch)) {
        printf("BDI Orchestrator: Warning - performance targets not met\n");
    }
    
    // Run microbenchmarks and smoke tests
    if (!bdi_run_microbenchmarks(orch)) {
        printf("BDI Orchestrator: Warning - microbenchmarks failed\n");
    }
    
    if (!bdi_run_smoke_tests(orch)) {
        printf("BDI Orchestrator: Warning - smoke tests failed\n");
    }
    
    // Record prove time
    uint64_t prove_time = get_time_us() - orch->phase_start_time;
    orch->performance_stats.prove_time_us = prove_time;
    
    printf("BDI Orchestrator: PROVE phase completed in %lu μs\n", prove_time);
    
    return true;
}

// ===================================================================
// Main Boot Sequence
// ===================================================================

bool bdi_orchestrator_boot(bdi_orchestrator_t* orch, const char* profile_name) {
    if (!orch) return false;
    
    printf("=== BDI Modular Kernel Boot Sequence ===\n");
    orch->boot_start_time = get_time_us();
    
    // Execute Probe→Plan→Compose→Prove pipeline
    if (!bdi_orchestrator_probe(orch)) {
        printf("BDI Orchestrator: PROBE phase failed - %s\n", orch->last_error);
        orch->performance_stats.failed_boots++;
        return false;
    }
    
    if (!bdi_orchestrator_plan(orch, profile_name)) {
        printf("BDI Orchestrator: PLAN phase failed - %s\n", orch->last_error);
        orch->performance_stats.failed_boots++;
        return false;
    }
    
    if (!bdi_orchestrator_compose(orch)) {
        printf("BDI Orchestrator: COMPOSE phase failed - %s\n", orch->last_error);
        orch->performance_stats.failed_boots++;
        return false;
    }
    
    if (!bdi_orchestrator_prove(orch)) {
        printf("BDI Orchestrator: PROVE phase failed - %s\n", orch->last_error);
        orch->performance_stats.failed_boots++;
        return false;
    }
    
    // Boot completed successfully
    uint64_t total_boot_time = get_time_us() - orch->boot_start_time;
    orch->performance_stats.total_boot_time_us = total_boot_time;
    orch->performance_stats.successful_boots++;
    orch->initialization_complete = true;
    orch->current_phase = BDI_PHASE_RUNNING;
    
    printf("=== BDI Modular Kernel Boot COMPLETED ===\n");
    printf("Total boot time: %lu μs (%.2f ms)\n", 
           total_boot_time, total_boot_time / 1000.0);
    printf("  Probe: %lu μs\n", orch->performance_stats.probe_time_us);
    printf("  Plan:  %lu μs\n", orch->performance_stats.plan_time_us);
    printf("  Compose: %lu μs\n", orch->performance_stats.compose_time_us);
    printf("  Prove: %lu μs\n", orch->performance_stats.prove_time_us);
    
    return true;
}

// ===================================================================
// Profile Management
// ===================================================================

bool bdi_load_profile(bdi_orchestrator_t* orch, const char* profile_name) {
    if (!orch) return false;
    
    // Use default profile if none specified
    if (!profile_name || strcmp(profile_name, "default") == 0) {
        profile_name = "balanced";
    }
    
    // Load built-in profiles
    if (strcmp(profile_name, "latency") == 0) {
        orch->active_profile = bdi_profile_latency;
        strcpy(orch->active_profile.preferred_modules[0], "memcpy.avx2");
        strcpy(orch->active_profile.preferred_modules[1], "paging.attention");
        strcpy(orch->active_profile.preferred_modules[2], "sched.graph-rt");
        return true;
    } else if (strcmp(profile_name, "throughput") == 0) {
        orch->active_profile = bdi_profile_throughput;
        strcpy(orch->active_profile.preferred_modules[0], "memcpy.avx512");
        strcpy(orch->active_profile.preferred_modules[1], "paging.attention");
        return true;
    } else if (strcmp(profile_name, "ai-train") == 0) {
        orch->active_profile = bdi_profile_ai_train;
        strcpy(orch->active_profile.preferred_modules[0], "tensor.amx");
        strcpy(orch->active_profile.preferred_modules[1], "memcpy.avx512");
        strcpy(orch->active_profile.preferred_modules[2], "paging.attention");
        strcpy(orch->active_profile.preferred_modules[3], "sched.graph-rt");
        return true;
    } else if (strcmp(profile_name, "balanced") == 0) {
        // Create balanced profile - FIXED: Use correct enum constant
        orch->active_profile.type = BDI_PROFILE_BALANCED;
        strcpy(orch->active_profile.name, "balanced");
        orch->active_profile.weight_latency = 0.5f;
        orch->active_profile.weight_throughput = 0.5f;
        orch->active_profile.weight_energy = 0.3f;
        orch->active_profile.weight_memory = 0.4f;
        orch->active_profile.min_freq_mhz = 2800;
        orch->active_profile.max_power_watts = 250;
        orch->active_profile.prefer_hugepages = true;
        orch->active_profile.numa_spread = false;
        strcpy(orch->active_profile.mm_policy, "attention");
        orch->active_profile.attention_config = bdi_profile_latency.attention_config;
        return true;
    }
    
    return false;
}

// ===================================================================
// Stub Implementations for Complex Functions
// ===================================================================

bool bdi_generate_composition_plan(bdi_orchestrator_t* orch) {
    if (!orch) return false;
    
    // Initialize plan
    memset(&orch->composition_plan, 0, sizeof(orch->composition_plan));
    
    // Add basic module assignments
    strcpy(orch->composition_plan.module_assignments[0].role, "uop.memcpy");
    orch->composition_plan.module_assignments[0].selected_module = NULL; // Would select actual module
    orch->composition_plan.module_assignments[0].selection_score = 0.9f;
    
    strcpy(orch->composition_plan.module_assignments[1].role, "paging.strategy");
    orch->composition_plan.module_assignments[1].selected_module = NULL;
    orch->composition_plan.module_assignments[1].selection_score = 0.8f;
    
    orch->composition_plan.assignment_count = 2;
    
    // Add μABI bindings
    strcpy(orch->composition_plan.uabi_bindings[0].operation, "memcpy_fast");
    orch->composition_plan.uabi_bindings[0].implementation = NULL; // Would point to actual function
    strcpy(orch->composition_plan.uabi_bindings[0].source_module, "memcpy.avx2");
    orch->composition_plan.uabi_bindings[0].estimated_cycles = 100;
    
    orch->composition_plan.binding_count = 1;
    
    // Set performance predictions
    orch->composition_plan.predicted_latency_us = 10.0f;
    orch->composition_plan.predicted_throughput_ops = 1000000.0f;
    orch->composition_plan.predicted_power_watts = 150.0f;
    
    // Set digests
    orch->composition_plan.capability_digest = orch->capability_digest;
    orch->composition_plan.profile_digest = 0; // Would hash profile
    orch->composition_plan.plan_digest = 0; // Would hash plan
    
    return true;
}

// Stub implementations for other complex functions
bool bdi_optimize_composition_plan(bdi_orchestrator_t* orch) { return true; }
bool bdi_validate_composition_plan(bdi_orchestrator_t* orch) { return true; }
bool bdi_cache_composition_plan(const bdi_composition_plan_t* plan) { return true; }
bool bdi_load_cached_plan(bdi_orchestrator_t* orch, uint64_t cap_digest, uint64_t prof_digest) { return false; }

bool bdi_setup_memory_pools(bdi_orchestrator_t* orch) { return true; }
bool bdi_configure_attention_mm(bdi_orchestrator_t* orch) { 
    orch->memory_manager = bdi_attention_mm_create(&orch->active_profile.attention_config);
    return orch->memory_manager != NULL;
}
bool bdi_bind_uabi_operations(bdi_orchestrator_t* orch) { 
    // Initialize μABI with detected capabilities
    bdi_init_uops(&orch->capabilities);
    return true;
}
bool bdi_setup_module_vtables(bdi_orchestrator_t* orch) { return true; }
bool bdi_setup_interrupt_handlers(bdi_orchestrator_t* orch) { return true; }
bool bdi_setup_timer_subsystem(bdi_orchestrator_t* orch) { return true; }
bool bdi_warm_caches(bdi_orchestrator_t* orch) { return true; }
bool bdi_setup_prefetching(bdi_orchestrator_t* orch) { return true; }

bool bdi_test_uabi_operations(bdi_orchestrator_t* orch) { return bdi_verify_uops(); }
bool bdi_test_module_interfaces(bdi_orchestrator_t* orch) { return true; }
bool bdi_test_memory_manager(bdi_orchestrator_t* orch) { return true; }
bool bdi_test_performance_targets(bdi_orchestrator_t* orch) { return true; }
bool bdi_validate_cfi_integrity(bdi_orchestrator_t* orch) { return true; }
bool bdi_validate_module_signatures(bdi_orchestrator_t* orch) { return true; }
bool bdi_validate_memory_protection(bdi_orchestrator_t* orch) { return true; }
bool bdi_run_microbenchmarks(bdi_orchestrator_t* orch) { return true; }
bool bdi_run_smoke_tests(bdi_orchestrator_t* orch) { return true; }

// ===================================================================
// Performance Monitoring
// ===================================================================

void bdi_print_orchestrator_stats(const bdi_orchestrator_t* orch) {
    if (!orch) return;
    
    printf("\n=== BDI Orchestrator Performance Statistics ===\n");
    printf("Boot Statistics:\n");
    printf("  Successful boots: %u\n", orch->performance_stats.successful_boots);
    printf("  Failed boots: %u\n", orch->performance_stats.failed_boots);
    printf("  Total boot time: %lu μs (%.2f ms)\n", 
           orch->performance_stats.total_boot_time_us,
           orch->performance_stats.total_boot_time_us / 1000.0);
    
    printf("\nPhase Breakdown:\n");
    printf("  Probe: %lu μs (%.1f%%)\n", 
           orch->performance_stats.probe_time_us,
           100.0 * orch->performance_stats.probe_time_us / orch->performance_stats.total_boot_time_us);
    printf("  Plan:  %lu μs (%.1f%%)\n", 
           orch->performance_stats.plan_time_us,
           100.0 * orch->performance_stats.plan_time_us / orch->performance_stats.total_boot_time_us);
    printf("  Compose: %lu μs (%.1f%%)\n", 
           orch->performance_stats.compose_time_us,
           100.0 * orch->performance_stats.compose_time_us / orch->performance_stats.total_boot_time_us);
    printf("  Prove: %lu μs (%.1f%%)\n", 
           orch->performance_stats.prove_time_us,
           100.0 * orch->performance_stats.prove_time_us / orch->performance_stats.total_boot_time_us);
    
    printf("\nHot-swap Statistics:\n");
    printf("  Completed: %u\n", orch->performance_stats.hot_swaps_completed);
    printf("  Failed: %u\n", orch->performance_stats.hot_swaps_failed);
    
    printf("\nRuntime Performance:\n");
    printf("  Average latency: %.2f μs\n", orch->performance_stats.avg_latency_us);
    printf("  Average throughput: %.0f ops/s\n", orch->performance_stats.avg_throughput_ops);
    printf("  Average power: %.1f W\n", orch->performance_stats.avg_power_watts);
    
    printf("\nError Statistics:\n");
    printf("  Total errors: %u\n", orch->error_count);
    if (orch->error_count > 0) {
        printf("  Last error: %s\n", orch->last_error);
    }
    
    printf("===============================================\n");
}
