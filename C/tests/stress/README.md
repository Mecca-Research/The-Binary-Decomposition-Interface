
# BDI Kernel Stress Tests

This directory contains comprehensive stress tests for the BDI Kernel project. These tests are designed to push the system to its limits and identify performance bottlenecks, memory leaks, and stability issues under extreme conditions.

## Overview

Stress tests simulate high-load scenarios to verify system behavior under pressure. They complement unit and integration tests by focusing on:

- **Endurance**: System behavior over extended periods
- **Scalability**: Performance with increasing load
- **Resource Management**: Memory, CPU, and stack usage under stress
- **Stability**: Crash resistance and graceful degradation

## Test Suites

### 1. Memory Stress Tests (`test_stress_memory.c`)

Tests intensive memory allocation and deallocation patterns:

- **Rapid Allocation/Deallocation**: Stress test memory allocator with rapid cycles
- **Memory Fragmentation**: Create and test fragmented memory scenarios
- **Large Allocations**: Test multi-megabyte allocations
- **VM Memory Stress**: Stress VM memory management
- **Constant Pool Growth**: Test dynamic array growth under load
- **Memory Pressure**: Simulate near-exhaustion scenarios
- **Sustained Load**: Maintain high memory usage over time
- **Mixed Patterns**: Combine small, medium, and large allocations

**Key Metrics:**
- Allocations per second
- Memory leak detection
- Fragmentation impact
- Peak memory usage

### 2. Stack Stress Tests (`test_stress_stack.c`)

Tests stack behavior under extreme conditions:

- **Deep Recursion**: Test recursion depth limits (up to 1000 levels)
- **Large Stack Frames**: Allocate large arrays on stack
- **VM Stack Operations**: Stress VM stack with many push/pop operations
- **Stack Overflow Detection**: Verify overflow handling
- **Nested Function Calls**: Simulate deep call chains
- **Stack Growth Patterns**: Test gradual and sudden growth
- **Stack Unwinding**: Verify proper cleanup under stress

**Key Metrics:**
- Maximum recursion depth
- Stack frame size limits
- Stack operations per second
- Overflow detection accuracy

### 3. CPU Stress Tests (`test_stress_cpu.c`)

Tests computational performance and sustained CPU load:

- **Intensive Arithmetic**: High-frequency arithmetic operations
- **Complex Computations**: Nested loops and mathematical functions
- **VM Execution Performance**: Bytecode execution throughput
- **Sustained CPU Load**: Continuous computation over time
- **Instruction Dispatch**: Measure VM instruction overhead
- **Mixed Operations**: Combine different arithmetic operations
- **CPU-Bound Workloads**: Pure computation without I/O
- **Long-Running Operations**: Extended computation scenarios

**Key Metrics:**
- Operations per second
- Instruction dispatch overhead
- Sustained throughput
- Performance degradation over time

### 4. Concurrency Stress Tests (`test_stress_concurrency.c`)

Placeholder tests for future concurrency support:

- **Sequential VM Instances**: Baseline for future parallel tests
- **Independent VM State**: Verify state isolation
- **Rapid VM Lifecycle**: Stress creation/destruction
- **Memory Isolation**: Verify separate memory spaces
- **Future Parallel Execution**: Documentation for planned tests
- **Future Thread Safety**: Requirements for concurrent access

**Note:** These are currently placeholder tests since the VM doesn't fully support concurrency yet. They prepare the infrastructure and document requirements for future implementation.

**Planned Future Tests:**
- Parallel VM execution
- Thread-safe memory management
- Race condition detection
- Deadlock prevention
- Concurrent chunk compilation

## Running Stress Tests

### Run All Stress Tests

```bash
make stress-test
```

### Run Individual Test Suites

```bash
# Memory stress tests
make stress-memory

# Stack stress tests
make stress-stack

# CPU stress tests
make stress-cpu

# Concurrency stress tests
make stress-concurrency
```

### Using the Stress Test Runner

```bash
# Run all suites
./tests/stress/stress_runner all

# Run specific suite
./tests/stress/stress_runner memory
./tests/stress/stress_runner stack
./tests/stress/stress_runner cpu
./tests/stress/stress_runner concurrency
```

## Configuration

Stress tests can be configured by modifying constants in the test files:

```c
// Memory stress configuration
#define STRESS_ITERATIONS_LOW 1000
#define STRESS_ITERATIONS_MEDIUM 10000
#define STRESS_ITERATIONS_HIGH 100000
#define LARGE_ALLOCATION_SIZE (10 * 1024 * 1024)  // 10 MB

// Stack stress configuration
#define MAX_RECURSION_DEPTH 1000
#define LARGE_STACK_FRAME_SIZE 10000

// CPU stress configuration
#define CPU_STRESS_ITERATIONS 100000
#define COMPLEX_COMPUTATION_SIZE 10000
```

## Interpreting Results

### Success Criteria

- ✅ **All tests pass**: No crashes, assertions, or errors
- ✅ **No memory leaks**: Memory usage returns to baseline
- ✅ **Stable performance**: No significant degradation over time
- ✅ **Graceful handling**: Errors are detected and handled properly

### Performance Benchmarks

Tests include performance measurements:

```
BENCHMARK: Intensive arithmetic took 123.456 ms
BENCHMARK: VM execution (100 iterations) took 234.567 ms
```

**Expected Performance:**
- Memory operations: < 1ms per 1000 allocations
- Stack operations: < 0.1ms per 1000 push/pop
- CPU operations: > 1M operations per second
- VM execution: < 10ms per 1000 instructions

### Warning Signs

- ⚠️ **Increasing memory usage**: Possible memory leak
- ⚠️ **Performance degradation**: Possible resource exhaustion
- ⚠️ **Crashes or hangs**: Stability issues
- ⚠️ **Assertion failures**: Logic errors under stress

## CI/CD Integration

Stress tests are integrated with GitHub Actions for manual execution:

```yaml
# Trigger manually from GitHub Actions UI
workflow_dispatch:
  inputs:
    duration:
      description: 'Test duration (seconds)'
      default: '300'
    intensity:
      description: 'Test intensity (low/medium/high)'
      default: 'medium'
    suite:
      description: 'Test suite (all/memory/stack/cpu/concurrency)'
      default: 'all'
```

See `.github/workflows/stress-tests.yml` for details.

## Troubleshooting

### Common Issues

**Out of Memory Errors:**
- Reduce `STRESS_ITERATIONS` constants
- Reduce `LARGE_ALLOCATION_SIZE`
- Run tests individually instead of all at once

**Stack Overflow:**
- Reduce `MAX_RECURSION_DEPTH`
- Reduce `LARGE_STACK_FRAME_SIZE`
- Increase system stack size: `ulimit -s unlimited`

**Slow Execution:**
- Reduce iteration counts for faster testing
- Run specific suites instead of all
- Use `low` intensity configuration

**Test Failures:**
- Check system resources (memory, CPU)
- Review test output for specific failures
- Run with verbose output for debugging

## Adding New Stress Tests

To add a new stress test:

1. **Create test function:**
```c
static bool test_new_stress_scenario(void) {
    printf("Running new stress test...\n");
    
    TEST_BENCHMARK_START();
    
    // Your stress test code here
    
    TEST_BENCHMARK_END("New stress test");
    
    printf("✓ New stress test passed\n");
    return true;
}
```

2. **Add to main runner:**
```c
int main(void) {
    // ... existing tests ...
    all_passed &= test_new_stress_scenario();
    // ...
}
```

3. **Update documentation:**
- Add test description to this README
- Document expected behavior
- Add performance benchmarks

## Performance Baselines

These are approximate baselines for reference (system-dependent):

| Test Suite | Metric | Baseline |
|------------|--------|----------|
| Memory | Alloc/dealloc cycles | 100K/sec |
| Memory | Large allocations | 10 MB/sec |
| Stack | Recursion depth | 1000 levels |
| Stack | Stack operations | 1M ops/sec |
| CPU | Arithmetic ops | 10M ops/sec |
| CPU | VM instructions | 100K inst/sec |

## Future Enhancements

Planned improvements for stress tests:

- [ ] Configurable test parameters via command line
- [ ] JSON output for automated analysis
- [ ] Performance regression detection
- [ ] Memory profiling integration
- [ ] Parallel stress test execution
- [ ] Real-time monitoring dashboard
- [ ] Automated performance reports
- [ ] Comparison with baseline metrics

## References

- [Test Framework Documentation](../framework/README.md)
- [VM Documentation](../../vm/README.md)
- [CI/CD Workflows](../../../.github/workflows/README.md)
- [Contributing Guidelines](../../../CONTRIBUTING.md)
