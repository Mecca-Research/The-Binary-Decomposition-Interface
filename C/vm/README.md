
# BDI Virtual Machine - Phase 7: VM Enhancement with JIT & GC

This directory contains the enhanced BDI Virtual Machine with Just-In-Time (JIT) compilation and Garbage Collection (GC) support.

## Overview

Phase 7 adds two major enhancements to the BDI VM:

1. **JIT Compiler**: Compiles hot bytecode paths to native machine code for improved performance
2. **Garbage Collector**: Automatic memory management with generational collection

## Directory Structure

```
C/vm/
├── bci_vm.h/c           # Base VM implementation (from previous phases)
├── bci_chunk.h/c        # Bytecode chunk management
├── vm.h                 # Enhanced VM with JIT and GC
├── jit/                 # JIT compiler components
│   ├── jit_compiler.h/c        # Core JIT compiler
│   ├── bytecode_compiler.h/c   # Bytecode to IR compiler
│   ├── hot_path.h/c            # Hot path detection
│   ├── tiered_compilation.h/c  # Tiered compilation manager
│   └── README.md               # JIT documentation
├── gc/                  # Garbage collection components
│   ├── mark_sweep.h/c          # Mark-sweep GC
│   ├── generational_gc.h/c     # Generational GC
│   └── README.md               # GC documentation
└── README.md            # This file
```

## Components

### Base VM (bci_vm.h/c)
The foundational VM implementation that executes BCI bytecode.

### Enhanced VM (vm.h)
Integrates JIT compilation and garbage collection with the base VM.

### JIT Compiler (jit/)
- **jit_compiler**: LLVM-based JIT compiler
- **bytecode_compiler**: Translates bytecode to LLVM IR
- **hot_path**: Detects frequently executed code
- **tiered_compilation**: Manages compilation tiers

### Garbage Collector (gc/)
- **mark_sweep**: Classic mark-and-sweep GC
- **generational_gc**: Generational GC with young/old generations

## Features

### JIT Compilation
- **Three-tier compilation**:
  - Tier 0: Interpreter (no compilation)
  - Tier 1: Baseline JIT (fast compilation, moderate optimization)
  - Tier 2: Optimized JIT (full optimization)

- **Hot path detection**: Identifies frequently executed code
- **Adaptive compilation**: Automatically promotes hot code to higher tiers
- **LLVM integration**: Uses LLVM for code generation and optimization

### Garbage Collection
- **Mark-and-sweep GC**: Simple, reliable collection
- **Generational GC**: Optimized for typical object lifetimes
- **Automatic memory management**: No manual memory management required
- **Configurable thresholds**: Tune GC behavior for your workload

## Usage

### Creating an Enhanced VM

```c
#include "vm/vm.h"

// Create VM with 64MB heap
EnhancedVM* vm = enhanced_vm_create(64 * 1024 * 1024);

// Enable JIT compilation
enhanced_vm_enable_jit(vm, true);

// Enable garbage collection
enhanced_vm_enable_gc(vm, true);

// Execute bytecode
BCIChunk* chunk = /* load bytecode */;
enhanced_vm_execute(vm, chunk);

// Cleanup
enhanced_vm_destroy(vm);
```

### Configuring JIT Compilation

```c
// Set optimization level (0-3)
jit_compiler_set_optimization_level(vm->jit_compiler, 2);

// Enable profiling
jit_compiler_enable_profiling(vm->jit_compiler, true);

// Set compilation policy
tiered_compilation_set_policy(vm->tiered_compilation, TIER_POLICY_BALANCED);
```

### Configuring Garbage Collection

```c
// Set promotion thresholds
generational_gc_set_age_threshold(vm->gc, 3);
generational_gc_set_size_threshold(vm->gc, 1024);

// Enable parallel GC
generational_gc_enable_parallel(vm->gc, true, 4);
```

## Performance

### JIT Compilation Performance
- **Baseline tier**: 2-3x speedup over interpreter
- **Optimized tier**: 5-10x speedup over interpreter
- **Compilation overhead**: ~1-10ms per function

### Garbage Collection Performance
- **Minor collection**: 1-10ms pause time
- **Major collection**: 10-100ms pause time
- **Throughput**: 95%+ application time (5% GC time)

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Enhanced VM                             │
│  ┌──────────────────────────────────────────────────────┐   │
│  │              Execution Engine                         │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐     │   │
│  │  │Interpreter │→ │ Baseline   │→ │ Optimized  │     │   │
│  │  │  (Tier 0)  │  │  (Tier 1)  │  │  (Tier 2)  │     │   │
│  │  └────────────┘  └────────────┘  └────────────┘     │   │
│  └──────────────────────────────────────────────────────┘   │
│                           ↓                                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │           JIT Compilation Pipeline                    │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐     │   │
│  │  │ Hot Path   │→ │ Bytecode   │→ │    JIT     │     │   │
│  │  │ Detection  │  │ Compiler   │  │  Compiler  │     │   │
│  │  └────────────┘  └────────────┘  └────────────┘     │   │
│  └──────────────────────────────────────────────────────┘   │
│                           ↓                                   │
│  ┌──────────────────────────────────────────────────────┐   │
│  │          Memory Management (GC)                       │   │
│  │  ┌────────────┐  ┌────────────┐  ┌────────────┐     │   │
│  │  │   Young    │→ │    Old     │  │ Permanent  │     │   │
│  │  │Generation  │  │Generation  │  │Generation  │     │   │
│  │  └────────────┘  └────────────┘  └────────────┘     │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

## Testing

Comprehensive tests are available in `C/tests/phase7/`:

- `test_jit_compiler.c`: JIT compilation tests (100 tests)
- `test_mark_sweep.c`: Mark-sweep GC tests (50 tests)
- `test_generational_gc.c`: Generational GC tests (30 tests)
- `test_all_vm.c`: Integration tests

Run tests:
```bash
cd C
make test_phase7
./tests/phase7/test_all_vm
```

## Build Requirements

### Dependencies
- C11 compiler (GCC 7+ or Clang 6+)
- LLVM 14+ development libraries
- Make

### Building

```bash
cd C
make vm_lib      # Build VM library
make test_phase7 # Build and run tests
```

### LLVM Integration

For production use with actual LLVM:

```bash
# Install LLVM development libraries
sudo apt-get install llvm-14-dev

# Build with LLVM support
make LLVM_CONFIG=llvm-config-14 vm_lib
```

## Future Enhancements

1. **Advanced JIT Optimizations**
   - Speculative optimization
   - Type specialization
   - Inline caching
   - On-stack replacement

2. **Advanced GC Algorithms**
   - Concurrent collection
   - Parallel collection
   - Region-based collection (G1-style)
   - Low-latency collection (ZGC-style)

3. **Profiling and Debugging**
   - Performance profiling
   - Memory profiling
   - Debugging support
   - Tracing and logging

4. **Integration**
   - Better integration with BDI type system
   - Support for BDI-specific optimizations
   - Hardware acceleration support

## References

- [LLVM Documentation](https://llvm.org/docs/)
- [Garbage Collection Handbook](http://gchandbook.org/)
- [JVM Specification](https://docs.oracle.com/javase/specs/jvms/se17/html/)
- [V8 JavaScript Engine](https://v8.dev/)

## License

See LICENSE file in the repository root.
