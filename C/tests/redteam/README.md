
# BDI Kernel Red-Team Memory Test Suite

## Overview

This comprehensive red-team test suite is designed to stress-test, fuzz, and validate the BDI kernel's memory subsystem under adversarial conditions. The suite covers all major memory components including allocators, NUMA management, huge pages, PMM, VMM, and HAM.

## Test Categories

### 1. Core Allocator Tests (`allocator/`)
- **Fuzz Testing**: Random sizes, alignments, and flags
- **Property Tests**: MemoryStats invariant validation
- **Fault Injection**: NUMA allocation failures
- **Safety Tests**: Double-free and mismatched-size detection
- **Counter Tests**: Underflow/overflow detection

### 2. NUMA & Arena Tests (`numa/`)
- **Multi-threaded Stress**: Concurrent allocation across CPUs
- **Arena Switching**: CPU pinning and migration validation
- **Leak Detection**: Cross-thread memory leak hunting
- **Topology Tests**: NUMA node change simulation
- **Arena Exhaustion**: Resource limit testing

### 3. Huge-Page Tests (`hugepage/`)
- **Boundary Testing**: 2 MiB and 1 GiB thresholds
- **Alignment Validation**: Proper alignment enforcement
- **Counter Integrity**: Drift and corruption detection
- **Misalignment Handling**: Invalid return address testing

### 4. PMM Tests (`pmm/`)
- **Pool Exhaustion**: Page pool depletion scenarios
- **Fallback Testing**: Remote-node allocation validation
- **Double-Free Detection**: Duplicate free operations
- **Address Fuzzing**: Invalid address range testing
- **Refcount Validation**: Underflow detection

### 5. VMM Tests (`vmm/`)
- **Region Overflow**: Region count limit testing
- **Alignment Tests**: Non-page-aligned mapping detection
- **Overlap Protection**: Overlapping range validation
- **Page Table Integrity**: Corruption detection
- **Partial Cleanup**: Incomplete unmapping scenarios

### 6. HAM Tests (`ham/`)
- **Vtable Fuzzing**: Tier transition validation
- **Motif Deduplication**: Hash collision testing
- **Concurrent Safety**: Multi-threaded operation validation
- **Region Management**: Allocation/deallocation stress

### 7. Instrumentation (`instrumentation/`)
- **Sanitizer Integration**: ASAN/UBSAN/TSAN helpers
- **Allocation Tracing**: Memory operation tracking
- **Differential Analysis**: Before/after comparison tools
- **Memory Barrier Validation**: Ordering verification

## Building

### Standard Build
```bash
cd C/tests/redteam
make all
```

### Sanitizer Builds
```bash
# Address Sanitizer (memory errors)
make asan

# Undefined Behavior Sanitizer
make ubsan

# Thread Sanitizer (race conditions)
make tsan

# All sanitizers
make sanitizers
```

### Debug Build
```bash
make debug
```

## Running Tests

### Run All Tests
```bash
./scripts/run_all_tests.sh
```

### Run Specific Category
```bash
./build/test_allocator_redteam
./build/test_numa_arena_redteam
./build/test_hugepage_redteam
./build/test_pmm_redteam
./build/test_vmm_redteam
./build/test_ham_redteam
./build/test_instrumentation
```

### Run with Sanitizers
```bash
./scripts/run_with_sanitizers.sh
```

### Run with Specific Sanitizer
```bash
# Address Sanitizer
./build/asan/test_allocator_redteam

# Thread Sanitizer
./build/tsan/test_numa_arena_redteam
```

## Test Infrastructure

### Test Harness (`common/redteam_harness.{c,h}`)
- Test registration and execution
- Result tracking and reporting
- Timing and performance metrics
- Memory leak detection
- Signal handling for crash detection

### Fault Injection (`common/fault_injection.{c,h}`)
- Configurable failure injection
- Probability-based failures
- Targeted subsystem failures
- Failure pattern recording

### Fuzzing Utilities (`common/fuzzing_utils.{c,h}`)
- Random data generation
- Size and alignment fuzzing
- Flag combination generation
- Corpus management

### Thread Utilities (`common/thread_utils.{c,h}`)
- Thread pool management
- CPU pinning helpers
- Synchronization primitives
- Thread-safe statistics

## Test Output

Tests produce detailed output including:
- Pass/fail status for each test
- Timing information
- Memory statistics
- Failure details with stack traces (when available)
- Sanitizer reports (when enabled)

### Example Output
```
[REDTEAM] Running Allocator Tests...
[PASS] test_alloc_random_sizes (1.234s)
[PASS] test_alloc_alignment_fuzz (0.567s)
[FAIL] test_double_free_detection (0.123s)
  Expected: Assertion failure
  Got: Silent corruption
[PASS] test_stats_invariants (2.345s)

Summary: 3/4 tests passed (75.0%)
Total time: 4.269s
Memory leaks: 0 bytes
```

## Sanitizer Usage

### Address Sanitizer (ASAN)
Detects:
- Use-after-free
- Heap buffer overflow
- Stack buffer overflow
- Memory leaks
- Use-after-return

### Undefined Behavior Sanitizer (UBSAN)
Detects:
- Integer overflow
- Null pointer dereference
- Misaligned access
- Invalid shifts
- Division by zero

### Thread Sanitizer (TSAN)
Detects:
- Data races
- Deadlocks
- Thread leaks
- Improper synchronization

## Integration with CI/CD

The test suite is designed for easy CI/CD integration:

```yaml
# Example GitHub Actions workflow
- name: Run Red-Team Tests
  run: |
    cd C/tests/redteam
    make sanitizers
    ./scripts/run_with_sanitizers.sh
```

## Contributing

When adding new tests:
1. Follow the existing test structure
2. Use the test harness macros
3. Add proper documentation
4. Include both positive and negative test cases
5. Test with all sanitizers
6. Update this README

## Test Coverage Goals

- **Line Coverage**: >90%
- **Branch Coverage**: >85%
- **Function Coverage**: 100% of public APIs
- **Edge Case Coverage**: All known failure modes

## Known Issues

See individual test files for known issues and TODOs.

## License

Part of the BDI Kernel project. See main LICENSE file.

## Authors

BDI Kernel Team - Red-Team Testing Initiative

## References

- BDI Kernel Memory Subsystem Documentation
- Linux Kernel Memory Management
- NUMA Architecture Best Practices
- Sanitizer Documentation
