
# BDI Kernel Regression Tests

This directory contains comprehensive regression tests for the BDI Kernel project. These tests ensure that existing functionality continues to work correctly as the codebase evolves.

## Overview

Regression tests verify that:
- **Existing features work correctly**: No functionality is broken by new changes
- **Edge cases are handled**: Boundary conditions and corner cases work as expected
- **Error handling is robust**: Errors are detected and handled gracefully
- **Performance is maintained**: No significant performance degradation

## Test Suites

### 1. VM Regression Tests (`test_regression_vm.c`)

Comprehensive tests for VM functionality:

- **Initialization & Cleanup**: VM and chunk lifecycle
- **Bytecode Operations**: Writing and executing bytecode
- **Constant Management**: Adding and retrieving constants
- **Arithmetic Operations**: ADD, SUBTRACT, MULTIPLY, DIVIDE
- **Negation**: Unary negation operation
- **Complex Expressions**: Nested arithmetic expressions
- **Stack Operations**: Push, pop, and stack management
- **Edge Cases**: Empty chunks, large constant pools
- **VM Reset**: Multiple execution cycles

**Coverage:**
- All VM API functions
- All opcodes
- Stack operations
- Memory management
- Error handling

### 2. JIT Regression Tests (`test_regression_jit.c`)

Tests for JIT compilation functionality:

- **JIT Availability**: Check if JIT is available
- **Basic Compilation**: Bytecode to native code
- **Optimization Correctness**: Optimized == unoptimized results
- **Cache Management**: Cache hits, misses, eviction
- **Error Handling**: Compilation failures, fallback
- **Performance**: Compilation and execution time

**Note:** Some tests are placeholders if JIT is not fully implemented. They document requirements and prepare infrastructure for future testing.

**Planned Tests:**
- JIT compilation correctness
- Optimization passes
- Cache behavior
- Performance benchmarks
- Error recovery

### 3. Graph Regression Tests (`test_regression_graph.c`)

Tests for graph execution functionality:

- **Graph Construction**: Creating nodes and edges
- **Node Execution**: Individual node operations
- **Graph Traversal**: Topological sort, DFS, BFS
- **Optimization Passes**: Constant folding, dead code elimination
- **Error Handling**: Cycle detection, invalid nodes
- **Edge Cases**: Empty graphs, single node graphs
- **Performance**: Construction and execution time

**Note:** Some tests are placeholders if graph execution is not fully implemented.

**Planned Tests:**
- Graph construction
- Node execution
- Traversal algorithms
- Optimization correctness
- Error detection

### 4. Integration Regression Tests (`test_regression_integration.c`)

Tests for component integration:

- **VM + JIT Integration**: Compiled execution
- **VM + Graph Integration**: Graph execution via VM
- **JIT + Graph Integration**: Compiled graph nodes
- **Full System Integration**: All components together
- **End-to-End Workflows**: Realistic usage scenarios
- **Cross-Component Data Flow**: Data passing between components
- **Error Propagation**: Errors across components
- **State Management**: State consistency
- **Performance**: Full system performance

**Coverage:**
- Component interactions
- Data flow
- Error handling
- State management
- Performance

## Running Regression Tests

### Run All Regression Tests

```bash
make regression-test
```

### Run Individual Test Suites

```bash
# VM regression tests
make regression-vm

# JIT regression tests
make regression-jit

# Graph regression tests
make regression-graph

# Integration regression tests
make regression-integration
```

### Using the Regression Test Runner

```bash
# Run all suites
./tests/regression/regression_runner all

# Run specific suite
./tests/regression/regression_runner vm
./tests/regression/regression_runner jit
./tests/regression/regression_runner graph
./tests/regression/regression_runner integration
```

## Test Structure

Each regression test follows this structure:

```c
static bool test_feature_name(void) {
    printf("Testing feature...\n");
    
    // Setup
    VM vm;
    vm_init(&vm);
    
    // Test
    // ... test code ...
    
    // Assertions
    TEST_ASSERT_EQ(expected, actual, "Description");
    
    // Cleanup
    vm_free(&vm);
    
    printf("✓ Feature test passed\n");
    return true;
}
```

## Interpreting Results

### Success Criteria

- ✅ **All tests pass**: No assertion failures
- ✅ **Correct behavior**: Results match expectations
- ✅ **No crashes**: Stable execution
- ✅ **No memory leaks**: Clean resource management

### Test Output

```
=== VM Regression Tests ===

Testing VM initialization and cleanup...
✓ VM init/free test passed
Testing chunk initialization and cleanup...
✓ Chunk init/free test passed
...

╔════════════════════════════════════════════════════════════╗
║                    Test Summary                            ║
╠════════════════════════════════════════════════════════════╣
║  Tests Passed: 14                                          ║
║  Tests Failed: 0                                           ║
╚════════════════════════════════════════════════════════════╝
```

### Failure Analysis

When a test fails:

1. **Check the assertion message**: Describes what failed
2. **Review the test code**: Understand what was being tested
3. **Check recent changes**: What changed since last passing?
4. **Run in debugger**: Step through the failing test
5. **Check logs**: Look for error messages or warnings

## CI/CD Integration

Regression tests are integrated with GitHub Actions:

```yaml
# Trigger manually from GitHub Actions UI
workflow_dispatch:
  inputs:
    suite:
      description: 'Test suite (all/vm/jit/graph/integration)'
      default: 'all'
    platform:
      description: 'Platform (ubuntu/macos/windows)'
      default: 'ubuntu'
```

See `.github/workflows/regression-tests.yml` for details.

## Adding New Regression Tests

To add a new regression test:

1. **Identify the feature to test:**
```c
// Test: New feature description
static bool test_new_feature(void) {
    printf("Testing new feature...\n");
    
    // Setup
    // Test
    // Assert
    // Cleanup
    
    printf("✓ New feature test passed\n");
    return true;
}
```

2. **Add to test suite:**
```c
int main(void) {
    // ... existing tests ...
    all_passed &= test_new_feature();
    // ...
}
```

3. **Document the test:**
- Add description to this README
- Document expected behavior
- Add edge cases to test

4. **Verify the test:**
- Run the test suite
- Verify it passes
- Verify it fails when it should

## Baseline Management

Regression tests can compare against baselines:

```c
// Future feature: baseline comparison
RegressionTestConfig config;
regression_config_init(&config);
config.baseline_path = "baselines/vm_baseline.json";
config.compare_with_baseline = true;
```

**Planned Features:**
- Save test results as baselines
- Compare current results with baselines
- Detect performance regressions
- Track test history

## Troubleshooting

### Common Issues

**Test Failures After Changes:**
- Review what changed
- Check if behavior change is intentional
- Update tests if behavior should change
- Fix code if behavior shouldn't change

**Intermittent Failures:**
- Check for race conditions (if concurrency is added)
- Check for uninitialized memory
- Check for resource leaks
- Add more assertions to narrow down issue

**Performance Degradation:**
- Run performance benchmarks
- Profile the code
- Compare with previous versions
- Identify bottlenecks

**Memory Leaks:**
- Run with valgrind: `valgrind ./test_regression_vm`
- Check cleanup code
- Verify all allocations are freed
- Use memory tracking utilities

## Best Practices

### Writing Regression Tests

1. **Test one thing at a time**: Each test should verify one specific behavior
2. **Use descriptive names**: Test names should describe what they test
3. **Add assertions**: Verify all important conditions
4. **Clean up resources**: Always free allocated memory
5. **Document edge cases**: Explain why edge cases are tested

### Maintaining Regression Tests

1. **Keep tests up to date**: Update tests when APIs change
2. **Add tests for bugs**: When fixing a bug, add a test
3. **Remove obsolete tests**: Remove tests for removed features
4. **Refactor tests**: Keep test code clean and maintainable
5. **Review test coverage**: Ensure all features are tested

## Performance Benchmarks

Expected performance for regression tests:

| Test Suite | Tests | Expected Time |
|------------|-------|---------------|
| VM | 14 | < 1 second |
| JIT | 8 | < 1 second |
| Graph | 9 | < 1 second |
| Integration | 9 | < 2 seconds |
| **Total** | **40** | **< 5 seconds** |

## Future Enhancements

Planned improvements:

- [ ] Baseline comparison system
- [ ] Performance regression detection
- [ ] Code coverage reporting
- [ ] Automated test generation
- [ ] Test result visualization
- [ ] Historical trend analysis
- [ ] Parallel test execution
- [ ] Test result database

## References

- [Test Framework Documentation](../framework/README.md)
- [Stress Tests Documentation](../stress/README.md)
- [VM Documentation](../../vm/README.md)
- [CI/CD Workflows](../../../.github/workflows/README.md)
- [Contributing Guidelines](../../../CONTRIBUTING.md)
