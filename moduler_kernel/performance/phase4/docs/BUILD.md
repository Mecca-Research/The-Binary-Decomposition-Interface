# Phase 4 Build System Guide

## Prerequisites

- CMake 3.10+
- Clang 12+ or GCC 9+ (Clang recommended for PGO)
- LLVM tools (llvm-profdata)
- Linux perf tools (for profiling)

## Basic Build

```bash
cd moduler_kernel/performance/phase4
mkdir build && cd build
cmake ..
make -j$(nproc)
```

## PGO Build (Recommended)

### Step 1: Build Instrumented Version

```bash
mkdir build-instrumented && cd build-instrumented
cmake -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_PGO_INSTRUMENTATION=ON \
      ..
make -j$(nproc)
```

### Step 2: Run Benchmarks

```bash
# Run all benchmarks to collect profiles
./bench/bench_phase4
cd ../phase3/build-instrumented
./bench/bench_phase3
cd ../phase2/build-instrumented
./bench/bench_phase2
cd ../phase1/build-instrumented
./bench/bench_phase1
```

### Step 3: Merge Profiles

```bash
cd ../../phase4/build-instrumented
llvm-profdata merge -output=bdi.profdata *.profraw
```

### Step 4: Build Optimized Version

```bash
cd ..
mkdir build-optimized && cd build-optimized
cmake -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_PGO=ON \
      -DPGO_PROFILE_FILE=../build-instrumented/bdi.profdata \
      ..
make -j$(nproc)
```

## LTO Build

```bash
mkdir build-lto && cd build-lto
cmake -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_LTO=ON \
      ..
make -j$(nproc)
```

## Combined PGO + LTO Build (Best Performance)

```bash
mkdir build-final && cd build-final
cmake -DCMAKE_BUILD_TYPE=Release \
      -DENABLE_PGO=ON \
      -DPGO_PROFILE_FILE=../build-instrumented/bdi.profdata \
      -DENABLE_LTO=ON \
      ..
make -j$(nproc)
```

## ISA-Specific Builds

### AVX-512 Build

```bash
cmake -DCMAKE_C_FLAGS="-march=skylake-avx512" ..
```

### AVX2 Build

```bash
cmake -DCMAKE_C_FLAGS="-march=haswell" ..
```

### Native Build (Recommended)

```bash
cmake -DCMAKE_C_FLAGS="-march=native" ..
```

## Testing

```bash
# Run all tests
make test

# Run specific test
./tests/test_cpu_features
./tests/test_perf_collector
```

## Benchmarking

```bash
# Run Phase 4 benchmark
./bench/bench_phase4

# Run with performance monitoring
sudo ./bench/bench_phase4
```

## Installation

```bash
sudo make install
```

## Troubleshooting

### PGO Profile Mismatch

If you see warnings about profile mismatches:
```bash
# Clean and rebuild
rm -rf build-instrumented build-optimized
# Repeat PGO build steps
```

### Permission Denied (perf)

If perf collector fails:
```bash
# Allow perf for non-root users
sudo sysctl -w kernel.perf_event_paranoid=-1
```

### LTO Out of Memory

If LTO fails with OOM:
```bash
# Reduce parallelism
make -j4
# Or disable LTO for large files
```
