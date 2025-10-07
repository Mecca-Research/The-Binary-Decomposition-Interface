
# BDI Profiling Infrastructure

Runtime profiling system for the BDI compiler and VM.

## Components

### profiler.h/c
Core profiling engine with hooks for:
- Function entry/exit timing
- Memory allocation/deallocation tracking
- Cache hit/miss monitoring
- Branch prediction tracking

### profile_data.h/c
Profile data analysis and aggregation:
- Function statistics (call count, timing, etc.)
- Memory usage statistics
- Cache performance metrics
- Branch prediction statistics

### profile_serializer.h/c
Binary serialization for profile data:
- `.bdi-profile` format for analyzed data
- Session serialization for raw events
- Efficient binary format with compression

### profile_analyzer.h/c
Intelligent profile analysis:
- Bottleneck detection
- Optimization recommendations
- Performance issue identification

## Usage

```c
// Initialize profiler
profiler_init();

// Start profiling session
ProfileSession *session = profiler_start_session();

// Run code with profiling hooks
profiler_hook_function_enter(session, func_id, "my_function");
// ... function code ...
profiler_hook_function_exit(session, func_id);

// Stop session
profiler_stop_session(session);

// Analyze results
ProfileData *data = profile_data_analyze(session);
profile_data_print_summary(data);

// Generate optimization suggestions
OptimizationReport *report = profile_analyzer_generate_suggestions(data);
profile_analyzer_print_report(report);

// Save to file
profile_serializer_save(data, "output.bdi-profile");

// Cleanup
profile_analyzer_free_report(report);
profile_data_free(data);
profiler_cleanup();
```

## .bdi-profile Format

Binary format structure:
- Header (magic, version, timestamp)
- Function statistics array
- Memory statistics
- Cache statistics
- Branch statistics

Magic number: `0x42444950` ("BDIP")
Version: 1
