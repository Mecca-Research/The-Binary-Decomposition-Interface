# TDT - Test-Driven Timmy Profile

## Overview

The **Test-Driven Timmy Profile (TDT)** is a comprehensive test generation and coverage analysis system for the CRRSS framework. TDT automatically generates unit tests and integration tests using multiple strategies, supports multiple testing frameworks, and provides detailed coverage analysis to identify gaps and improve code quality.

## Features

### 1. Multi-Strategy Test Generation

TDT supports three primary test generation strategies:

#### Pattern-Based Generation
- Analyzes common code patterns (loops, conditionals, error handling)
- Generates tests for each identified pattern
- Handles edge cases and boundary conditions
- Detects memory-related patterns (malloc/free, use-after-free)

#### Coverage-Driven Generation
- Analyzes code paths and branch coverage
- Generates tests to maximize coverage
- Identifies untested branches and paths
- Targets specific coverage goals

#### Specification-Based Generation
- Parses function signatures and parameters
- Extracts documentation from comments
- Generates tests based on function specifications
- Creates comprehensive test suites for each function

### 2. Multi-Framework Test Generation

TDT can generate tests for multiple testing frameworks:

#### Custom CRRSS Framework
- Matches existing `test_*.c` pattern in CRRSS
- Uses CRRSS test infrastructure
- Simple, lightweight test format

#### Unity Framework
- Industry-standard embedded testing framework
- Includes Unity headers and macros
- Setup/teardown support

#### Check Framework
- Popular C unit testing framework
- Suite and test case structure
- Comprehensive assertion library

### 3. Comprehensive Coverage Analysis

TDT provides detailed coverage analysis:

#### Line Coverage
- Tracks which lines are executed
- Identifies uncovered lines
- Calculates line coverage percentage

#### Branch Coverage
- Identifies all branches (if/else, switch, loops)
- Tracks which branches are taken
- Calculates branch coverage percentage
- Identifies partially covered branches

#### Function Coverage
- Tracks which functions are called
- Identifies untested functions
- Calculates function coverage percentage

### 4. Test Types

TDT generates different types of tests:

- **Unit Tests**: Testing individual functions in isolation
- **Integration Tests**: Testing module interactions
- **Edge Case Tests**: Testing boundary conditions and special cases
- **Error Handling Tests**: Testing error paths and exception handling
- **Boundary Tests**: Testing minimum, maximum, and zero values

### 5. CRRSS Integration

TDT integrates with other CRRSS modules:

#### MSM Integration
- Generate tests for memory safety issues
- Test malloc/free patterns
- Test buffer overflow scenarios
- Test use-after-free detection

#### STP Integration
- Generate tests for type safety issues
- Test type conversions
- Test struct alignment

#### BPME Integration
- Generate tests for predicted bugs
- Test error handling paths
- Test high-risk code paths

## Usage

### Basic Initialization

```c
#include "tdt.h"

// Configure TDT
tdt_config_t config = {
    .strategy = TDT_STRATEGY_ALL,
    .framework = TDT_FRAMEWORK_CUSTOM,
    .test_type = TDT_TEST_TYPE_BOTH,
    .auto_generate_tests = true,
    .track_line_coverage = true,
    .track_branch_coverage = true,
    .track_function_coverage = true,
    .target_coverage_percentage = 80.0,
    .max_tests_per_function = 5,
    .generate_edge_case_tests = true,
    .generate_error_handling_tests = true,
    .generate_boundary_tests = true,
    .test_output_directory = "/tmp/tdt_tests",
    .generate_reports = true,
    .verbose_output = true
};

// Initialize TDT
tdt_context_t* ctx = tdt_init(&config);
if (!ctx) {
    fprintf(stderr, "Failed to initialize TDT\n");
    return -1;
}
```

### Generate Tests for a File

```c
// Analyze and generate tests
tdt_generation_result_t result;
crrss_status_t status = tdt_generate_tests(ctx, "path/to/source.c", &result);

if (status == CRRSS_SUCCESS && result.success) {
    printf("Generated %u tests\n", result.tests_generated);
    printf("  Unit tests: %u\n", result.unit_tests_generated);
    printf("  Integration tests: %u\n", result.integration_tests_generated);
    printf("  Edge case tests: %u\n", result.edge_case_tests_generated);
}
```

### Analyze Coverage

```c
// Analyze file coverage
tdt_file_coverage_t coverage;
status = tdt_analyze_coverage(ctx, "path/to/source.c", &coverage);

if (status == CRRSS_SUCCESS) {
    printf("Coverage Analysis:\n");
    printf("  Line coverage: %.2f%%\n", coverage.line_coverage_percent);
    printf("  Branch coverage: %.2f%%\n", coverage.branch_coverage_percent);
    printf("  Function coverage: %.2f%%\n", coverage.function_coverage_percent);
}
```

### Identify Coverage Gaps

```c
// Find coverage gaps
tdt_coverage_gap_t gaps[100];
uint32_t num_gaps = 0;
status = tdt_identify_coverage_gaps(ctx, "path/to/source.c", 
                                      gaps, 100, &num_gaps);

if (status == CRRSS_SUCCESS) {
    printf("Found %u coverage gaps:\n", num_gaps);
    for (uint32_t i = 0; i < num_gaps; i++) {
        printf("  Line %u: %s - %s\n", 
               gaps[i].line_number,
               gaps[i].gap_type,
               gaps[i].suggested_test);
    }
}
```

### Generate Report

```c
// Generate comprehensive report
tdt_report_t report;
status = tdt_generate_report(ctx, &report);

if (status == CRRSS_SUCCESS) {
    printf("TDT Report:\n");
    printf("  Total tests generated: %u\n", 
           report.statistics.total_tests_generated);
    printf("  Coverage gaps identified: %u\n", 
           report.statistics.coverage_gaps_identified);
    printf("  Overall test quality: %.2f\n", 
           report.overall_test_quality_score);
    
    // Export to file
    tdt_export_report(ctx, &report, "/tmp/tdt_report.txt", "text");
}
```

### Cleanup

```c
// Cleanup when done
tdt_cleanup(ctx);
```

## Integration Examples

### With MSM (Memory Safety Monitor)

```c
// Initialize MSM
msm_context_t* msm_ctx = msm_initialize(&msm_config);

// Integrate with TDT
tdt_integrate_msm(tdt_ctx, msm_ctx);

// Generate memory safety tests
tdt_generation_result_t result;
tdt_msm_generate_memory_safety_tests(tdt_ctx, msm_ctx, 
                                      "path/to/source.c", &result);
```

### With STP (Strict Typist Profile)

```c
// Initialize STP
stp_context_t* stp_ctx = stp_initialize(&stp_config);

// Integrate with TDT
tdt_integrate_stp(tdt_ctx, stp_ctx);

// Generate type safety tests
tdt_generation_result_t result;
tdt_stp_generate_type_safety_tests(tdt_ctx, stp_ctx,
                                     "path/to/source.c", &result);
```

### With BPME (Bug Prediction & Mitigation Engine)

```c
// Initialize BPME
bpme_context_t* bpme_ctx = bpme_initialize(&bpme_config);

// Integrate with TDT
tdt_integrate_bpme(tdt_ctx, bpme_ctx);

// Generate tests for predicted bugs
tdt_generation_result_t result;
tdt_bpme_generate_bug_pattern_tests(tdt_ctx, bpme_ctx,
                                      "path/to/source.c", &result);
```

## Configuration Options

### Generation Strategy
- `TDT_STRATEGY_PATTERN_BASED`: Use pattern-based generation
- `TDT_STRATEGY_COVERAGE_DRIVEN`: Use coverage-driven generation
- `TDT_STRATEGY_SPECIFICATION_BASED`: Use specification-based generation
- `TDT_STRATEGY_ALL`: Use all strategies

### Test Framework
- `TDT_FRAMEWORK_CUSTOM`: Custom CRRSS framework
- `TDT_FRAMEWORK_UNITY`: Unity testing framework
- `TDT_FRAMEWORK_CHECK`: Check testing framework
- `TDT_FRAMEWORK_ALL`: Generate for all frameworks

### Test Type
- `TDT_TEST_TYPE_UNIT`: Generate unit tests only
- `TDT_TEST_TYPE_INTEGRATION`: Generate integration tests only
- `TDT_TEST_TYPE_BOTH`: Generate both unit and integration tests

## API Reference

### Core Functions

- `tdt_init()`: Initialize TDT system
- `tdt_cleanup()`: Cleanup and free resources
- `tdt_reset()`: Reset TDT state
- `tdt_configure()`: Update configuration

### Test Generation

- `tdt_analyze_file()`: Analyze file for test generation
- `tdt_generate_tests()`: Generate tests for a file
- `tdt_generate_function_tests()`: Generate tests for specific function
- `tdt_generate_directory_tests()`: Generate tests for directory

### Coverage Analysis

- `tdt_analyze_coverage()`: Analyze coverage for a file
- `tdt_calculate_line_coverage()`: Calculate line coverage
- `tdt_calculate_branch_coverage()`: Calculate branch coverage
- `tdt_calculate_function_coverage()`: Calculate function coverage
- `tdt_identify_coverage_gaps()`: Identify coverage gaps

### Reporting

- `tdt_generate_report()`: Generate comprehensive report
- `tdt_export_report()`: Export report to file
- `tdt_get_statistics()`: Get TDT statistics

### Integration

- `tdt_integrate_msm()`: Integrate with MSM
- `tdt_integrate_stp()`: Integrate with STP
- `tdt_integrate_bpme()`: Integrate with BPME

## Best Practices

1. **Start with Pattern-Based Generation**: Use pattern-based generation to quickly identify common issues
2. **Target Coverage Goals**: Set realistic coverage targets (e.g., 80%) and generate tests to meet them
3. **Review Generated Tests**: Always review and customize generated tests
4. **Integrate with Other Modules**: Use MSM, STP, and BPME integration for comprehensive testing
5. **Regular Coverage Analysis**: Regularly analyze coverage to identify gaps
6. **Generate Edge Case Tests**: Enable edge case and boundary testing for robustness
7. **Export Reports**: Export reports for documentation and tracking

## Examples

### Example 1: Quick Test Generation

```c
tdt_config_t config = {
    .strategy = TDT_STRATEGY_PATTERN_BASED,
    .framework = TDT_FRAMEWORK_CUSTOM,
    .test_type = TDT_TEST_TYPE_UNIT,
    .max_tests_per_function = 3,
    .verbose_output = true
};

tdt_context_t* ctx = tdt_init(&config);
tdt_generation_result_t result;
tdt_generate_tests(ctx, "mycode.c", &result);
printf("Generated %u tests\n", result.tests_generated);
tdt_cleanup(ctx);
```

### Example 2: Comprehensive Coverage Analysis

```c
tdt_config_t config = {
    .strategy = TDT_STRATEGY_COVERAGE_DRIVEN,
    .framework = TDT_FRAMEWORK_CUSTOM,
    .test_type = TDT_TEST_TYPE_BOTH,
    .track_line_coverage = true,
    .track_branch_coverage = true,
    .track_function_coverage = true,
    .target_coverage_percentage = 90.0,
    .verbose_output = true
};

tdt_context_t* ctx = tdt_init(&config);

// Analyze coverage
tdt_file_coverage_t coverage;
tdt_analyze_coverage(ctx, "mycode.c", &coverage);

// Identify gaps
tdt_coverage_gap_t gaps[50];
uint32_t num_gaps;
tdt_identify_coverage_gaps(ctx, "mycode.c", gaps, 50, &num_gaps);

// Generate tests to fill gaps
tdt_generation_result_t result;
tdt_generate_tests(ctx, "mycode.c", &result);

tdt_cleanup(ctx);
```

## Building

TDT is built as part of the CRRSS framework:

```bash
cd tools/crrss
make clean
make
make test-tdt
```

## Testing

Run the TDT test suite:

```bash
cd tools/crrss
make test-tdt
```

Or run all CRRSS tests:

```bash
make test
```

## Phase 2 Stage 2 Implementation

This TDT implementation is part of Phase 2 Stage 2 of the CRRSS framework development. It provides:

- ✅ Comprehensive test generation (pattern-based, coverage-driven, specification-based)
- ✅ Multi-framework support (Custom, Unity, Check)
- ✅ Detailed coverage analysis (line, branch, function)
- ✅ Integration with MSM, STP, and BPME
- ✅ Comprehensive testing and documentation

## Future Enhancements

Planned enhancements for future versions:

1. **Machine Learning Integration**: Use ML to predict which tests are most valuable
2. **Mutation Testing**: Generate mutants to test test suite quality
3. **Property-Based Testing**: Generate property-based tests
4. **Test Prioritization**: Prioritize tests based on risk and coverage
5. **Continuous Integration**: Direct CI/CD integration
6. **Test Reduction**: Identify and remove redundant tests
7. **Parallel Test Generation**: Generate tests in parallel for speed

## Contributing

When contributing to TDT:

1. Follow existing code style and patterns
2. Add comprehensive tests for new features
3. Update documentation
4. Ensure all tests pass
5. Create clear commit messages

## License

Part of the CRRSS framework for the BDI kernel project.
