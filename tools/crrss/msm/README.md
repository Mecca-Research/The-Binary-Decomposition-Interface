# MSM - Memory-Safety Maniac Profile

**Phase 1B Stage 3 Implementation**

## Overview

The Memory-Safety Maniac Profile (MSM) is a comprehensive memory safety analysis system for the BDI project's CRRSS tooling suite. MSM provides advanced capabilities for detecting, tracking, and preventing memory-related bugs and vulnerabilities in C code.

## Features

### Core Capabilities

- **Real-Time Allocation Tracking**: Track all malloc/calloc/realloc/free operations with detailed metadata
- **Pointer Lifecycle Management**: Monitor pointer states from creation through deallocation
- **Use-After-Free Detection**: Identify attempts to access freed memory at runtime and statically
- **Double-Free Detection**: Detect multiple free() calls on the same pointer
- **Memory Leak Detection**: Find unfreed allocations and potential leak patterns
- **NULL-Check Enforcement**: Analyze code for missing NULL checks before pointer dereference
- **Buffer Overflow Detection**: Identify unsafe buffer operations and bounds violations
- **Stack Trace Capture**: Detailed call stack information for allocations and issues
- **Comprehensive Reporting**: Generate detailed reports in text, JSON, and HTML formats

### Integration

- **BPME Integration**: Pattern detection for memory-related bugs
- **SCIV Integration**: Validation rules for memory safety
- **Memory Layer Integration**: Deep integration with BDI memory subsystems
- **CLI Commands**: Full command-line interface through `crrss msm`

## Architecture

```
msm/
├── msm.h          # Public API and type definitions
├── msm.c          # Core implementation
└── README.md      # This file

Internal Components:
├── Context Management      # MSM initialization and configuration
├── Hash Table System       # Fast O(1) lookups for allocations/pointers
├── Allocation Tracker      # malloc/free tracking with metadata
├── Pointer Safety Analyzer # Pointer lifecycle and validation
├── NULL-Check Enforcer     # Static analysis for NULL checks
├── Buffer Overflow Detector # Bounds checking and unsafe function detection
├── Issue Recording System  # Track and categorize detected issues
├── Statistics Engine       # Performance metrics and analytics
├── Report Generator        # Multi-format report generation
└── Integration Layer       # CRRSS component integration
```

## Quick Start

### Basic Usage

```c
#include "msm/msm.h"

// Initialize MSM
msm_config_t config = {
    .tracking_mode = MSM_TRACKING_DETAILED,
    .enable_allocation_tracking = true,
    .enable_pointer_tracking = true,
    .enable_use_after_free_detection = true,
    .enable_double_free_detection = true,
    .enable_leak_detection = true,
    .max_tracked_allocations = 10000
};

msm_context_t* msm = msm_initialize(&config);

// Track allocations
void* ptr = malloc(100);
msm_track_allocation(msm, ptr, 100, __FILE__, __LINE__, __func__);

// Use the pointer...

// Track deallocation
msm_track_deallocation(msm, ptr, __FILE__, __LINE__, __func__);
free(ptr);

// Analyze a file
msm_issue_t issues[100];
uint32_t num_issues;
msm_analyze_file(msm, "myfile.c", issues, 100, &num_issues);

// Generate report
msm_report_t report;
msm_generate_report(msm, &report);
msm_export_report(msm, &report, "msm_report.txt", "text");

// Cleanup
msm_shutdown(msm);
```

### CLI Usage

```bash
# Analyze a single file
crrss msm -f kernel/memory.c

# Analyze directory recursively
crrss msm -d moduler_kernel/

# Generate comprehensive report
crrss msm -d moduler_kernel/ --report msm_report.txt --format text

# Generate JSON report
crrss msm -f kernel/memory.c --report msm_report.json --format json

# Limit issues reported
crrss msm -f kernel/memory.c --max-issues 50
```

## API Reference

### Initialization

```c
msm_context_t* msm_initialize(const msm_config_t* config);
void msm_shutdown(msm_context_t* ctx);
crrss_status_t msm_reset(msm_context_t* ctx);
```

### Allocation Tracking

```c
crrss_status_t msm_track_allocation(
    msm_context_t* ctx,
    void* address,
    size_t size,
    const char* file,
    uint32_t line,
    const char* function
);

crrss_status_t msm_track_deallocation(
    msm_context_t* ctx,
    void* address,
    const char* file,
    uint32_t line,
    const char* function
);

crrss_status_t msm_get_allocation_metadata(
    msm_context_t* ctx,
    void* address,
    allocation_metadata_t* metadata
);
```

### Pointer Safety

```c
crrss_status_t msm_track_pointer(
    msm_context_t* ctx,
    void* pointer_addr,
    void* points_to,
    const char* file,
    uint32_t line,
    const char* function
);

crrss_status_t msm_validate_pointer(
    msm_context_t* ctx,
    void* pointer,
    bool* is_valid
);

crrss_status_t msm_track_pointer_access(
    msm_context_t* ctx,
    void* pointer_addr,
    pointer_access_t access_type,
    const char* file,
    uint32_t line
);
```

### Detection

```c
// Use-after-free detection
crrss_status_t msm_detect_use_after_free(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// Double-free detection
crrss_status_t msm_detect_double_free(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// Memory leak detection
crrss_status_t msm_detect_leaks(
    msm_context_t* ctx,
    allocation_metadata_t* leaks,
    uint32_t max_leaks,
    uint32_t* num_leaks
);

// NULL-check analysis
crrss_status_t msm_analyze_null_checks(
    msm_context_t* ctx,
    const char* file_path,
    null_check_result_t* results,
    uint32_t max_results,
    uint32_t* num_results
);

// Buffer overflow detection
crrss_status_t msm_detect_buffer_overflow(
    msm_context_t* ctx,
    const char* file_path,
    buffer_analysis_result_t* results,
    uint32_t max_results,
    uint32_t* num_results
);
```

### Analysis

```c
// Analyze single file
crrss_status_t msm_analyze_file(
    msm_context_t* ctx,
    const char* file_path,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);

// Analyze directory
crrss_status_t msm_analyze_directory(
    msm_context_t* ctx,
    const char* dir_path,
    msm_report_t* report
);

// Analyze code snippet
crrss_status_t msm_analyze_snippet(
    msm_context_t* ctx,
    const char* code_snippet,
    size_t snippet_length,
    msm_issue_t* issues,
    uint32_t max_issues,
    uint32_t* num_issues
);
```

### Reporting

```c
crrss_status_t msm_get_statistics(
    msm_context_t* ctx,
    msm_statistics_t* stats
);

crrss_status_t msm_generate_report(
    msm_context_t* ctx,
    msm_report_t* report
);

crrss_status_t msm_export_report(
    msm_context_t* ctx,
    const msm_report_t* report,
    const char* output_path,
    const char* format  // "text", "json", "html"
);

crrss_status_t msm_calculate_safety_score(
    msm_context_t* ctx,
    double* score
);
```

## Configuration

### Tracking Modes

- **MSM_TRACKING_DISABLED**: No tracking
- **MSM_TRACKING_BASIC**: Basic tracking without stack traces
- **MSM_TRACKING_DETAILED**: Detailed tracking with stack traces (recommended)
- **MSM_TRACKING_PARANOID**: Maximum safety checks (slower but most thorough)

### Configuration Options

```c
typedef struct {
    msm_tracking_mode_t tracking_mode;
    bool enable_pointer_tracking;
    bool enable_allocation_tracking;
    bool enable_null_check_enforcement;
    bool enable_buffer_overflow_detection;
    bool enable_use_after_free_detection;
    bool enable_double_free_detection;
    bool enable_leak_detection;
    
    uint32_t max_tracked_pointers;
    uint32_t max_tracked_allocations;
    uint32_t max_stack_depth;
    
    bool generate_reports;
    bool enforce_null_checks;
    bool track_allocation_sites;
    const char* report_output_dir;
    
    bool integrate_with_bpme;
    bool integrate_with_sciv;
    bool integrate_with_memory_layer;
} msm_config_t;
```

## Issue Types

MSM detects the following issue types:

1. **MSM_ISSUE_MEMORY_LEAK**: Unfreed allocations
2. **MSM_ISSUE_USE_AFTER_FREE**: Accessing freed memory
3. **MSM_ISSUE_DOUBLE_FREE**: Multiple frees of same pointer
4. **MSM_ISSUE_NULL_DEREF**: NULL pointer dereference
5. **MSM_ISSUE_BUFFER_OVERFLOW**: Buffer overflow/overrun
6. **MSM_ISSUE_BUFFER_UNDERFLOW**: Buffer underflow
7. **MSM_ISSUE_UNINITIALIZED_POINTER**: Using uninitialized pointers
8. **MSM_ISSUE_DANGLING_POINTER**: Pointer to freed memory
9. **MSM_ISSUE_INVALID_FREE**: Freeing invalid pointer
10. **MSM_ISSUE_MISSING_NULL_CHECK**: Missing NULL check before use
11. **MSM_ISSUE_UNSAFE_POINTER_ARITHMETIC**: Unsafe pointer math

## Performance

### Overhead

- Basic tracking: ~5-10% runtime overhead
- Detailed tracking: ~10-20% runtime overhead
- Paranoid mode: ~20-40% runtime overhead

### Optimization Tips

1. Use `MSM_TRACKING_BASIC` for production
2. Use `MSM_TRACKING_DETAILED` for development/debugging
3. Adjust `max_tracked_allocations` based on your workload
4. Disable features you don't need (e.g., pointer tracking if only checking for leaks)

## Testing

Run the MSM test suite:

```bash
cd tools/crrss/build
cmake .. -DCRRSS_BUILD_TESTS=ON
make
./test_msm
```

## Integration with CRRSS

MSM integrates seamlessly with other CRRSS components:

```c
// Integrate with BPME for pattern detection
msm_integrate_bpme(msm_ctx, bpme_ctx);

// Integrate with SCIV for validation
msm_integrate_sciv(msm_ctx, sciv_ctx);

// Integrate with Memory Layer
msm_integrate_memory_layer(msm_ctx, memory_ctx);
```

## Examples

### Example 1: Detect Use-After-Free

```c
msm_context_t* msm = msm_initialize(&config);

void* ptr = malloc(100);
msm_track_allocation(msm, ptr, 100, __FILE__, __LINE__, __func__);

free(ptr);
msm_track_deallocation(msm, ptr, __FILE__, __LINE__, __func__);

// This will be detected!
ptr[0] = 'A';  // Use-after-free

msm_issue_t issues[10];
uint32_t num_issues;
msm_query_issues_by_type(msm, MSM_ISSUE_USE_AFTER_FREE, 
                         issues, 10, &num_issues);
```

### Example 2: Static Analysis

```c
msm_context_t* msm = msm_initialize(&config);

// Analyze file for all issues
msm_issue_t issues[100];
uint32_t num_issues;
msm_analyze_file(msm, "suspicious_code.c", issues, 100, &num_issues);

// Print issues
for (uint32_t i = 0; i < num_issues; i++) {
    printf("%s:%u - %s: %s\n",
           issues[i].file_path,
           issues[i].line_number,
           msm_issue_type_to_string(issues[i].issue_type),
           issues[i].description);
}
```

### Example 3: Generate Report

```c
msm_context_t* msm = msm_initialize(&config);

// Analyze entire codebase
msm_report_t report;
msm_analyze_directory(msm, "moduler_kernel/", &report);

// Export reports in multiple formats
msm_export_report(msm, &report, "msm_report.txt", "text");
msm_export_report(msm, &report, "msm_report.json", "json");
msm_export_report(msm, &report, "msm_report.html", "html");

printf("Safety Score: %.2f\n", report.safety_score);
printf("Overall Risk: %s\n", risk_level_to_string(report.overall_risk));
```

## Best Practices

1. **Initialize MSM early**: Initialize MSM at the start of your program
2. **Track all allocations**: Wrap malloc/free with MSM tracking calls
3. **Use macros**: Define macros for automatic tracking
4. **Regular analysis**: Run static analysis regularly on your codebase
5. **Review reports**: Carefully review MSM reports and fix issues
6. **Integrate CI/CD**: Add MSM checks to your CI/CD pipeline

### Macro Example

```c
#ifdef MSM_ENABLED
#define TRACKED_MALLOC(size) ({ \
    void* ptr = malloc(size); \
    msm_track_allocation(g_msm, ptr, size, __FILE__, __LINE__, __func__); \
    ptr; \
})

#define TRACKED_FREE(ptr) do { \
    msm_track_deallocation(g_msm, ptr, __FILE__, __LINE__, __func__); \
    free(ptr); \
} while(0)
#else
#define TRACKED_MALLOC(size) malloc(size)
#define TRACKED_FREE(ptr) free(ptr)
#endif
```

## Troubleshooting

### High Memory Usage

- Reduce `max_tracked_allocations` and `max_tracked_pointers`
- Use `MSM_TRACKING_BASIC` instead of `DETAILED`
- Disable stack trace capture

### False Positives

- Review and adjust detection thresholds
- Use `msm_reset()` to clear tracking state between tests
- Check for custom memory allocators

### Performance Issues

- Profile with different tracking modes
- Disable unused detection features
- Consider using MSM only in debug builds

## Technical Details

### Thread Safety

MSM is thread-safe. All operations use mutex locks for synchronization. The internal hash table uses per-bucket locks for concurrent access.

### Memory Management

MSM uses internal hash tables with chaining for O(1) average case lookups. Memory overhead is approximately:
- Per allocation: ~200 bytes (with stack trace)
- Per pointer: ~150 bytes (with tracking)

### Static Analysis

MSM's static analysis uses pattern matching and simple heuristics. While effective for common patterns, it may miss complex cases. Combine with runtime tracking for best results.

## Future Enhancements

- Machine learning-based pattern detection
- Integration with Valgrind and AddressSanitizer
- GUI visualization of memory usage
- Real-time monitoring dashboard
- Advanced inter-procedural analysis

## Contributing

See main CRRSS README for contribution guidelines.

## License

Part of the Binary Decomposition Interface (BDI) Project.
Copyright (c) 2025 BDI Development Team.

## Support

For issues, questions, or suggestions, please refer to the main CRRSS documentation or contact the BDI development team.
