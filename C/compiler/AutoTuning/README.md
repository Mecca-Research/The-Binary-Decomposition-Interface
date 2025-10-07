
# BDI Auto-Tuning System

Adaptive optimization system that learns from profiling data and automatically recompiles code with better optimizations.

## Components

### autotuner.h/c
Main auto-tuning orchestrator:
- Profile-based optimization decisions
- Adaptive recompilation triggers
- Performance tracking
- Optimization recommendations

### optimizer_selector.h/c
ML-based optimization selection:
- Strategy selection (aggressive, balanced, conservative, etc.)
- Optimization flag configuration
- Benefit prediction
- ML-guided optimization

### hardware_detector.h/c
Hardware capability detection:
- CPU vendor and model detection
- SIMD feature detection (SSE, AVX, AVX-512)
- Cache size detection
- NUMA topology detection

### recompiler.h/c
Adaptive recompilation system:
- Recompilation queue management
- Priority-based processing
- Optimization flag application
- Compiler invocation

## Usage

```c
// Initialize auto-tuner
AutoTunerConfig config = {
    .enable_hardware_detection = true,
    .enable_profile_based_optimization = true,
    .enable_adaptive_recompilation = true,
    .enable_ml_optimization_selection = true,
    .recompilation_threshold = 0.10,
    .max_recompilation_attempts = 5
};

AutoTuner *tuner = autotuner_init(&config);

// Detect hardware
HardwareCapabilities *hw = hardware_detector_detect();
hardware_detector_print(hw);

// Run program and collect profile
ProfileSession *session = profiler_start_session();
// ... run program ...
profiler_stop_session(session);
ProfileData *profile = profile_data_analyze(session);

// Update auto-tuner
autotuner_update_metrics(tuner, profile);

// Check if recompilation is needed
if (autotuner_should_recompile(tuner, profile)) {
    // Select optimization strategy
    OptimizationStrategy strategy = optimizer_selector_select_strategy(profile);
    OptimizationFlags flags = optimizer_selector_get_flags(strategy);
    
    // Create recompilation request
    RecompilationRequest req = {
        .flags = flags,
        .profile = profile,
        .priority = 10
    };
    strncpy(req.source_file, "program.c", sizeof(req.source_file));
    strncpy(req.output_file, "program.o", sizeof(req.output_file));
    
    // Add to queue and process
    RecompilationQueue *queue = recompiler_create_queue();
    recompiler_add_request(queue, &req);
    recompiler_process_queue(queue);
    
    recompiler_free_queue(queue);
}

// Cleanup
hardware_detector_free(hw);
autotuner_cleanup(tuner);
```

## Optimization Strategies

- **AGGRESSIVE**: Maximum optimization, may increase code size
- **BALANCED**: Good balance between speed and size
- **CONSERVATIVE**: Safe optimizations, minimal code size increase
- **SIZE**: Optimize for code size
- **SPEED**: Optimize for maximum speed
- **ML_GUIDED**: Use ML model to select optimizations

## Hardware Detection

Detects:
- CPU vendor (Intel, AMD, ARM, RISC-V)
- SIMD capabilities (SSE, AVX, AVX-512)
- Cache sizes (L1, L2, L3)
- NUMA topology
- Core and thread count
