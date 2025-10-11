# CRRSS - Code Review, Reliability, and Static Safety System

**Version 1.0.0** | **Phase 1B: Tooling & Automation Stage 2**

## Overview

CRRSS (Code Review, Reliability, and Static Safety System) is a comprehensive tooling suite for kernel development in the Binary Decomposition Interface (BDI) project. It provides automated bug detection, code validation, and memory analysis capabilities to ensure high code quality and system reliability.

### Key Features

- **Bug Prior Mapping Engine (BPME)**: Analyzes code patterns and predicts potential bugs based on historical data
- **Self-Check Internal Validator (SCIV)**: Validates code correctness, consistency, and adherence to coding standards
- **Memory Integration Layer**: Integrates with BDI's memory subsystems (HAM, PMM, VMM) for comprehensive memory analysis
- **Memory-Safety Maniac Profile (MSM)**: Advanced memory safety analysis with real-time tracking and static code analysis
- **Command-Line Tools**: `crrss query`, `crrss stats`, and `crrss msm` for comprehensive code analysis

## Architecture

```
tools/crrss/
├── common/          # Common types and utilities
│   ├── crrss_types.h
│   └── crrss_types.c
├── bpme/           # Bug Prior Mapping Engine
│   ├── bpme.h
│   └── bpme.c
├── sciv/           # Self-Check Internal Validator
│   ├── sciv.h
│   └── sciv.c
├── memory_layer/   # Memory Integration Layer
│   ├── memory_integration.h
│   └── memory_integration.c
├── msm/            # Memory-Safety Maniac Profile
│   ├── msm.h
│   ├── msm.c
│   └── README.md
├── cli/            # Command-Line Interface
│   ├── crrss_cli.h
│   ├── crrss_cli.c
│   └── crrss_main.c
├── tests/          # Comprehensive Test Suite
│   ├── test_bpme.c
│   ├── test_sciv.c
│   ├── test_memory.c
│   └── test_msm.c
└── docs/           # Documentation
```

## Components

### 1. Bug Prior Mapping Engine (BPME)

The BPME analyzes code patterns using historical bug data from the comprehensive bug analysis (PRs #1-165) to predict potential bugs and assign priority levels.

**Key Capabilities:**
- Pattern-based bug detection
- Risk assessment for code changes
- Historical bug pattern tracking
- ML-ready architecture (future enhancement)
- Priority mapping (P0-P3)

**Detected Patterns:**
- Memory leaks
- Use-after-free
- Double-free
- NULL pointer dereference
- Buffer overflow
- Race conditions
- Deadlocks
- Uninitialized variables
- Unchecked return values
- Missing error checks

### 2. Self-Check Internal Validator (SCIV)

The SCIV performs static analysis and validation of kernel code including coding standards compliance, memory management patterns, and error handling validation.

**Key Capabilities:**
- Memory safety validation
- Error handling checks
- NULL pointer checks
- Coding style compliance
- Function complexity metrics
- API usage verification
- Concurrency safety checks
- Resource cleanup validation

**Validation Rules:**
- Memory Safety
- Error Handling
- NULL Checks
- Coding Style
- Function Complexity
- Comment Quality
- Naming Conventions
- API Usage
- Concurrency Safety
- Resource Cleanup

### 3. Memory Integration Layer

Provides tooling access to BDI's memory management systems with comprehensive memory analysis capabilities.

**Key Capabilities:**
- Memory leak detection
- Use-after-free detection
- Double-free detection
- Allocation tracking
- Memory efficiency calculation
- Pattern validation
- Integration with HAM, PMM, VMM

### 4. Memory-Safety Maniac Profile (MSM)

**Phase 1B Stage 3 Implementation**

The MSM provides comprehensive memory safety analysis combining real-time tracking with advanced static analysis.

**Key Capabilities:**
- Real-time allocation/deallocation tracking with metadata
- Pointer lifecycle management and validation
- Use-after-free detection (runtime + static)
- Double-free detection (runtime + static)
- Memory leak detection and analysis
- NULL-check enforcement and analysis
- Buffer overflow detection
- Stack trace capture for debugging
- Comprehensive reporting (text, JSON, HTML)
- Integration with BPME, SCIV, and Memory Layer

**Detected Issue Types:**
- Memory leaks
- Use-after-free
- Double-free
- NULL pointer dereference
- Buffer overflow/underflow
- Uninitialized pointers
- Dangling pointers
- Invalid free operations
- Missing NULL checks
- Unsafe pointer arithmetic

**CLI Usage:**
```bash
# Analyze single file
crrss msm -f kernel/memory.c

# Analyze directory
crrss msm -d moduler_kernel/

# Generate report
crrss msm -d moduler_kernel/ --report msm_report.txt --format text
```

See `msm/README.md` for detailed documentation and API reference.

## Building

### Prerequisites

- CMake 3.16 or higher
- GCC with C23 support or Clang
- pthreads library
- Math library (libm)

### Build Instructions

```bash
# Navigate to CRRSS directory
cd tools/crrss

# Create build directory
mkdir -p build && cd build

# Configure
cmake ..

# Build
make

# Run tests
make check

# Install (optional)
sudo make install
```

### Build Options

- `CRRSS_BUILD_TESTS`: Build test suite (default: ON)
- `CRRSS_ENABLE_ASAN`: Enable AddressSanitizer (default: OFF)
- `CRRSS_BUILD_DOCS`: Build documentation (default: OFF)

### Build Examples

```bash
# Debug build with AddressSanitizer
cmake -DCMAKE_BUILD_TYPE=Debug -DCRRSS_ENABLE_ASAN=ON ..
make

# Release build without tests
cmake -DCMAKE_BUILD_TYPE=Release -DCRRSS_BUILD_TESTS=OFF ..
make

# Build with documentation
cmake -DCRRSS_BUILD_DOCS=ON ..
make docs
```

## Usage

### Command-Line Interface

The `crrss` tool provides several commands for interacting with the system:

```bash
crrss <command> [options]
```

#### Commands

- `query`: Query bug predictions and risk assessments
- `stats`: Display codebase statistics and system health
- `analyze`: Analyze files or directories for bugs
- `validate`: Validate code against standards
- `report`: Generate detailed reports
- `help`: Display help message
- `version`: Display version information

### Query Command

Query bug predictions, risk assessments, and validation results.

```bash
# Query by priority
crrss query --priority P0 --details

# Query by category
crrss query --category memory --details

# Query specific file
crrss query --file moduler_kernel/memory.c --details

# Query with filters
crrss query --priority P1 --category concurrency --max-results 50
```

**Options:**
- `-p, --priority <level>`: Filter by priority (P0, P1, P2, P3)
- `-c, --category <cat>`: Filter by category (memory, concurrency, logic, performance, security)
- `-f, --file <path>`: Query specific file
- `-d, --details`: Show detailed information
- `-n, --max-results <num>`: Maximum results to display

### Stats Command

Display codebase statistics, system health metrics, and analysis results.

```bash
# Show all statistics
crrss stats

# Show statistics for directory
crrss stats --directory moduler_kernel/

# Show memory statistics
crrss stats --memory

# Show validation statistics
crrss stats --validation

# Export to JSON
crrss stats --format json
```

**Options:**
- `-d, --directory <path>`: Analyze directory
- `-m, --memory`: Show memory statistics
- `-v, --validation`: Show validation statistics
- `--format <fmt>`: Output format (text, json, csv)

## Integration with BDI Build System

To integrate CRRSS with the main BDI build system:

### Option 1: Add to Root CMakeLists.txt

```cmake
# Add CRRSS subdirectory
add_subdirectory(tools/crrss)
```

### Option 2: Standalone Build

```bash
cd tools/crrss
mkdir build && cd build
cmake ..
make
sudo make install
```

### Option 3: Use as External Project

```cmake
include(ExternalProject)
ExternalProject_Add(crrss
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/tools/crrss
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
)
```

## Testing

### Running Tests

```bash
# Run all tests
make check

# Run individual test suites
./test_bpme
./test_sciv
./test_memory

# Run with CTest
ctest --output-on-failure
```

### Test Coverage

The test suite includes:
- **BPME Tests**: Initialization, pattern detection, risk assessment, statistics
- **SCIV Tests**: Validation rules, code quality metrics, compliance checks
- **Memory Tests**: Leak detection, allocation tracking, use-after-free detection

## Examples

### Example 1: Analyze a File for Bugs

```c
#include "bpme/bpme.h"

// Initialize BPME
bpme_config_t config = {
    .enable_pattern_matching = true,
    .confidence_threshold = 0.7,
    .max_predictions = 100
};
bpme_context_t* ctx = bpme_initialize(&config);

// Analyze file
bug_prediction_t predictions[100];
uint32_t num_predictions = 0;
bpme_analyze_file(ctx, "memory.c", predictions, 100, &num_predictions);

// Print results
for (uint32_t i = 0; i < num_predictions; i++) {
    printf("Bug: %s at line %u\n", 
           predictions[i].description, 
           predictions[i].line_number);
}

bpme_shutdown(ctx);
```

### Example 2: Validate Code

```c
#include "sciv/sciv.h"

// Initialize SCIV
sciv_config_t config = {
    .enable_strict_mode = true,
    .coding_standard = "kernel"
};
sciv_context_t* ctx = sciv_initialize(&config);

// Validate file
validation_issue_t issues[100];
uint32_t num_issues = 0;
sciv_validate_file(ctx, "driver.c", issues, 100, &num_issues);

// Print issues
for (uint32_t i = 0; i < num_issues; i++) {
    printf("Issue: %s at line %u\n",
           issues[i].message,
           issues[i].line_number);
}

sciv_shutdown(ctx);
```

### Example 3: Check Memory Patterns

```c
#include "memory_layer/memory_integration.h"

// Initialize Memory Integration
memory_integration_config_t config = {
    .enable_leak_detection = true,
    .track_allocations = true
};
memory_integration_context_t* ctx = memory_integration_initialize(&config);

// Detect leaks
leak_detection_report_t report = {0};
report.max_records = 100;
memory_integration_detect_leaks(ctx, &report);

printf("Found %u potential leaks\n", report.potential_leaks);
printf("Total leaked: %lu bytes\n", report.total_leaked_bytes);

memory_integration_shutdown(ctx);
```

## API Reference

### BPME API

```c
bpme_context_t* bpme_initialize(const bpme_config_t* config);
void bpme_shutdown(bpme_context_t* ctx);
crrss_status_t bpme_analyze_file(...);
crrss_status_t bpme_analyze_directory(...);
crrss_status_t bpme_assess_change_risk(...);
crrss_status_t bpme_get_pattern_info(...);
crrss_status_t bpme_query_by_priority(...);
crrss_status_t bpme_query_by_category(...);
```

### SCIV API

```c
sciv_context_t* sciv_initialize(const sciv_config_t* config);
void sciv_shutdown(sciv_context_t* ctx);
crrss_status_t sciv_validate_file(...);
crrss_status_t sciv_validate_directory(...);
crrss_status_t sciv_check_rule(...);
crrss_status_t sciv_configure_rule(...);
crrss_status_t sciv_calculate_complexity(...);
crrss_status_t sciv_get_quality_score(...);
```

### Memory Integration API

```c
memory_integration_context_t* memory_integration_initialize(...);
void memory_integration_shutdown(...);
crrss_status_t memory_integration_analyze(...);
crrss_status_t memory_integration_detect_leaks(...);
crrss_status_t memory_integration_track_allocation(...);
crrss_status_t memory_integration_track_deallocation(...);
crrss_status_t memory_integration_validate_patterns(...);
```

## Configuration

### BPME Configuration

```c
typedef struct {
    const char* knowledge_base_path;  // Path to historical bug database
    bool enable_ml_predictions;        // Use ML-based predictions
    bool enable_pattern_matching;      // Use pattern matching
    double confidence_threshold;       // Minimum confidence (0.0-1.0)
    uint32_t max_predictions;          // Max predictions per file
} bpme_config_t;
```

### SCIV Configuration

```c
typedef struct {
    bool enable_strict_mode;
    bool enable_style_checks;
    bool enable_performance_checks;
    uint32_t max_function_complexity;
    uint32_t max_function_lines;
    uint32_t max_cyclomatic_complexity;
    const char* coding_standard;  // "kernel", "misra", "custom"
} sciv_config_t;
```

### Memory Integration Configuration

```c
typedef struct {
    bool enable_leak_detection;
    bool enable_use_after_free_detection;
    bool enable_double_free_detection;
    bool track_allocations;
    uint32_t max_tracked_allocations;
    const char* memory_subsystem_path;
} memory_integration_config_t;
```

## Performance Considerations

- **BPME**: O(n) complexity for file analysis, where n is number of lines
- **SCIV**: O(n*r) complexity, where n is lines and r is number of rules
- **Memory Integration**: O(1) for tracking operations, O(n) for leak detection

## Known Limitations

1. **Pattern Detection**: Uses heuristic-based detection; may have false positives
2. **Cross-File Analysis**: Limited to single-file analysis in current version
3. **ML Predictions**: Machine learning support is architecture-ready but not yet implemented
4. **Concurrency**: Not yet thread-safe; use separate contexts per thread

## Future Enhancements

- [ ] Machine learning-based bug prediction
- [ ] Cross-file analysis and whole-program analysis
- [ ] Integration with CI/CD pipelines
- [ ] Real-time IDE integration
- [ ] Advanced reporting with graphs and charts
- [ ] Custom pattern definition language
- [ ] Thread-safe operation
- [ ] Incremental analysis support
- [ ] Integration with version control systems

## Contributing

Please follow the BDI project's contribution guidelines. For CRRSS-specific contributions:

1. Ensure all tests pass
2. Add tests for new features
3. Update documentation
4. Follow the kernel coding style
5. Run `make check` before submitting

## License

Same as the BDI project (see root LICENSE file).

## Support

For issues, questions, or contributions:
- GitHub Issues: BDI project repository
- Documentation: `/tools/crrss/docs/`
- Examples: `/tools/crrss/examples/`

## Acknowledgments

CRRSS is built upon the comprehensive bug analysis conducted across PRs #1-165 of the BDI project. Special thanks to all contributors who helped identify and fix bugs, providing the foundation for this tooling system.

## Version History

- **1.0.0** (2025-10-11): Initial release
  - Bug Prior Mapping Engine (BPME)
  - Self-Check Internal Validator (SCIV)
  - Memory Integration Layer
  - Command-line tools (query, stats)
  - Comprehensive test suite
  - Full CMake integration
