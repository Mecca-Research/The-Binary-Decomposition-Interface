
# Phase 2: Memory & Scheduling Optimization

## Overview

Phase 2 builds upon Phase 1's lock-free foundations (45-89% faster than Linux, 63% average) to deliver advanced memory and scheduling optimizations through three critical performance levers:

1. **Per-CPU Arenas & NUMA Pinning**: Attention-guided allocation with NUMA awareness
2. **Predictive Prefetch & Page Policy**: Huge pages, PCID/ASID, and intelligent prefetching
3. **Tickless Time & Wheel/CRDS Scheduler**: Hierarchical timer wheel with constant-time scheduling

**Performance Goal**: Additional 20-30% improvement over Phase 1 baseline.

## Quick Start

### Building

```bash
cd moduler_kernel/performance/phase2
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running Tests

```bash
# Run all tests
make test

# Run specific test suites
./tests/test_numa_topology
./tests/test_per_cpu_arena
./tests/test_huge_pages
./tests/test_timer_wheel
./tests/test_crds_scheduler
```

### Running Benchmarks

```bash
# Run all benchmarks
./bench/bench_phase2

# Run specific benchmarks
./bench/bench_numa_allocation
./bench/bench_prefetch
./bench/bench_scheduler
```

## Directory Structure

```
phase2/
├── numa/                    # NUMA topology and per-CPU arenas
│   ├── numa_topology.{h,c}  # NUMA detection and mapping
│   ├── per_cpu_arena.{h,c}  # Per-CPU arena allocator
│   ├── attention.{h,c}      # Attention-guided allocation policy
│   └── affinity.{h,c}       # Memory affinity tracking
├── prefetch/                # Prefetch and page policy
│   ├── huge_pages.{h,c}     # Huge page allocator
│   ├── pcid.{h,c}           # PCID/ASID management
│   ├── tlb_batch.{h,c}      # Batched TLB invalidation
│   ├── graph_prefetch.{h,c} # Graph edge prefetching
│   ├── hw_prefetch.{h,c}    # Hardware prefetch hints
│   └── fault_predict.{h,c}  # Page fault prediction
├── scheduler/               # Tickless timer and CRDS
│   ├── timer_wheel.{h,c}    # Hierarchical timer wheel
│   ├── tickless.{h,c}       # Tickless operation
│   ├── crds.{h,c}           # CRDS scheduler
│   └── deadline.{h,c}       # Deadline scheduling
├── integration/             # Phase 1 integration
│   ├── phase2_init.{h,c}    # Initialization and setup
│   └── phase1_hooks.{h,c}   # Hooks into Phase 1 components
├── tests/                   # Unit and integration tests
├── bench/                   # Benchmarking suite
└── docs/                    # Documentation
    ├── ARCHITECTURE.md      # Architecture overview
    ├── API.md               # API documentation
    ├── PERFORMANCE.md       # Performance analysis
    └── INTEGRATION.md       # Integration guide
```

## Key Features

### NUMA Optimization

- **Topology Detection**: Automatic NUMA node discovery
- **Per-CPU Arenas**: Lock-free local allocation (2-8MB per CPU)
- **Attention-Guided Policy**: Hot object tracking and migration
- **Affinity Tracking**: Per-object NUMA node tracking
- **Cross-NUMA Optimization**: Minimize remote memory access

### Prefetch & Page Policy

- **Huge Pages**: 2MB/1GB pages with automatic promotion
- **PCID/ASID**: Selective TLB invalidation (4096 contexts)
- **Batched TLB Flush**: Reduce IPI overhead by 70-90%
- **Graph Prefetch**: Prefetch next call targets
- **Hardware Hints**: PREFETCH instructions with stride detection
- **Fault Prediction**: Preallocate pages before faults

### Tickless Scheduler

- **Hierarchical Timer Wheel**: 4-level wheel, O(1) operations
- **Tickless Operation**: No periodic interrupts
- **CRDS Scheduling**: Constant-time rate-monotonic
- **Deadline Support**: Absolute deadline tracking
- **IPI Reduction**: Batch and coalesce timer IPIs
- **Per-CPU Timers**: No global timer lock

## Integration with Phase 1

Phase 2 seamlessly integrates with Phase 1 components:

- **Arena Allocator**: Extended with NUMA awareness
- **Fiber Scheduler**: Enhanced with deadline support
- **Lock-Free Rings**: Use NUMA-local memory
- **Graph Calls**: Prefetch targets before dispatch
- **Zero-Copy IPC**: NUMA-aware allocation

See [INTEGRATION.md](docs/INTEGRATION.md) for details.

## Performance Targets

| Metric | Phase 1 | Phase 2 Target | Improvement |
|--------|---------|----------------|-------------|
| Memory Latency | Baseline | -30% | NUMA locality |
| TLB Miss Rate | Baseline | -80% | Huge pages + PCID |
| Timer IPI Rate | Baseline | -90% | Tickless + batching |
| Context Switch | Baseline | -20% | PCID + prefetch |
| Cross-NUMA Traffic | Baseline | -60% | Attention-guided |
| Overall Performance | +63% | +85-95% | Combined effect |

## API Examples

### NUMA-Aware Allocation

```c
#include "numa/per_cpu_arena.h"

// Initialize per-CPU arenas
per_cpu_arena_init();

// Allocate from local NUMA node
void* ptr = per_cpu_arena_alloc(1024);

// Free back to arena
per_cpu_arena_free(ptr, 1024);
```

### Huge Page Allocation

```c
#include "prefetch/huge_pages.h"

// Allocate 2MB huge page
void* huge_ptr = huge_page_alloc(HUGE_PAGE_2MB);

// Allocate 1GB huge page
void* giant_ptr = huge_page_alloc(HUGE_PAGE_1GB);

// Free huge page
huge_page_free(huge_ptr, HUGE_PAGE_2MB);
```

### Timer Wheel

```c
#include "scheduler/timer_wheel.h"

// Create timer wheel
timer_wheel_t* wheel = timer_wheel_create();

// Add timer (expires in 100ms)
timer_id_t id = timer_wheel_add(wheel, 100, callback, arg);

// Cancel timer
timer_wheel_cancel(wheel, id);

// Advance time and process expired timers
timer_wheel_tick(wheel);
```

### CRDS Scheduler

```c
#include "scheduler/crds.h"

// Create CRDS scheduler
crds_scheduler_t* sched = crds_scheduler_create();

// Add periodic fiber (period=10ms, deadline=10ms)
fiber_id_t fid = crds_scheduler_add_periodic(sched, 10, 10, fiber_func, arg);

// Schedule next fiber
fiber_t* next = crds_scheduler_schedule(sched);

// Run fiber
fiber_run(next);
```

## Configuration

Phase 2 can be configured at compile-time and runtime:

### Compile-Time Options

```cmake
# Enable/disable subsystems
option(PHASE2_NUMA "Enable NUMA optimization" ON)
option(PHASE2_HUGE_PAGES "Enable huge pages" ON)
option(PHASE2_PCID "Enable PCID/ASID" ON)
option(PHASE2_TICKLESS "Enable tickless operation" ON)
option(PHASE2_CRDS "Enable CRDS scheduler" ON)

# Tuning parameters
set(PER_CPU_ARENA_SIZE "4MB" CACHE STRING "Per-CPU arena size")
set(TIMER_WHEEL_LEVELS "4" CACHE STRING "Timer wheel levels")
set(PCID_SPACE_SIZE "4096" CACHE STRING "PCID space size")
```

### Runtime Configuration

```c
// Configure NUMA policy
numa_config_t numa_cfg = {
    .attention_threshold = 1000,  // Migration threshold
    .migration_cost = 100,        // Migration cost estimate
    .rebalance_interval = 1000,   // Rebalance every 1000ms
};
numa_configure(&numa_cfg);

// Configure prefetch
prefetch_config_t prefetch_cfg = {
    .enable_graph_prefetch = true,
    .enable_hw_prefetch = true,
    .prefetch_distance = 64,      // Prefetch 64 bytes ahead
};
prefetch_configure(&prefetch_cfg);

// Configure scheduler
scheduler_config_t sched_cfg = {
    .enable_tickless = true,
    .timer_coalesce_window = 10,  // ±10% coalescing
    .ipi_batch_size = 16,         // Batch 16 IPIs
};
scheduler_configure(&sched_cfg);
```

## Testing

Phase 2 includes comprehensive testing:

- **Unit Tests**: Test individual components in isolation
- **Integration Tests**: Test interaction with Phase 1
- **NUMA Tests**: Validate NUMA locality and migration
- **TLB Tests**: Measure TLB efficiency
- **Timer Tests**: Verify timer accuracy and overhead
- **Scheduler Tests**: Test deadline scheduling

Run tests with:

```bash
make test ARGS="-V"  # Verbose output
```

## Benchmarking

Benchmark suite includes:

- **NUMA Allocation**: Measure local vs remote allocation
- **Prefetch Efficiency**: Measure cache/TLB miss rates
- **Timer Overhead**: Measure timer IPI rate
- **Scheduler Latency**: Measure context switch time
- **End-to-End**: Compare with Phase 1 baseline

Run benchmarks with:

```bash
./bench/bench_phase2 --output=results.json
```

## Documentation

- [ARCHITECTURE.md](docs/ARCHITECTURE.md): Detailed architecture
- [API.md](docs/API.md): Complete API reference
- [PERFORMANCE.md](docs/PERFORMANCE.md): Performance analysis
- [INTEGRATION.md](docs/INTEGRATION.md): Integration guide

## Contributing

Phase 2 is part of the BDI Kernel Performance Mission. Contributions should:

1. Maintain Phase 1 compatibility
2. Follow coding standards (see CONTRIBUTING.md)
3. Include tests and benchmarks
4. Update documentation
5. Preserve performance characteristics

## License

Same as BDI Kernel (see LICENSE in repository root).

## References

1. Varghese & Lauck (1987): "Hashed and Hierarchical Timing Wheels"
2. Liu & Layland (1973): "Rate Monotonic Scheduling"
3. Intel x86-64 Manual: PCID, INVPCID, Huge Pages
4. Linux Kernel: NUMA, TLB, Timer subsystems
5. Phase 1: Lock-free rings, fiber scheduler, arena allocator

## Contact

For questions or issues, please open a GitHub issue or contact the BDI Kernel team.
