
# JIT Compiler

This directory contains the Just-In-Time (JIT) compiler implementation for the BDI VM.

## Components

### jit_compiler.h/c
Core JIT compiler that integrates with LLVM to compile bytecode to native machine code.

**Features:**
- Multi-tier compilation (interpreter, baseline, optimized)
- LLVM integration for code generation
- Compilation statistics and profiling
- Configurable optimization levels

**Key Functions:**
- `jit_compiler_create()` - Create JIT compiler instance
- `jit_compiler_compile_function()` - Compile bytecode function to native code
- `jit_compiler_execute()` - Execute compiled native code
- `jit_compiler_optimize()` - Recompile with higher optimization tier

### bytecode_compiler.h/c
Translates BCI bytecode to LLVM IR for optimization and native code generation.

**Features:**
- Bytecode to LLVM IR translation
- IR optimization passes
- Function inlining
- Dead code elimination

**Key Functions:**
- `bytecode_compiler_compile_chunk()` - Compile bytecode chunk to IR
- `bytecode_compiler_optimize_ir()` - Run optimization passes
- `bytecode_compiler_inline_functions()` - Inline small functions

### hot_path.h/c
Detects frequently executed code paths for optimization.

**Features:**
- Execution frequency tracking
- Hot path identification
- Configurable thresholds
- Performance statistics

**Key Functions:**
- `hot_path_detector_record_execution()` - Record execution event
- `hot_path_detector_is_hot()` - Check if path is hot
- `hot_path_detector_should_optimize()` - Determine if optimization needed

### tiered_compilation.h/c
Manages tiered compilation strategy and tier transitions.

**Features:**
- Three-tier compilation (interpreter → baseline → optimized)
- Adaptive compilation decisions
- Cost-benefit analysis
- Configurable policies (aggressive, balanced, conservative)

**Key Functions:**
- `tiered_compilation_make_decision()` - Decide compilation strategy
- `tiered_compilation_execute_decision()` - Execute compilation decision
- `tiered_compilation_calculate_benefit()` - Cost-benefit analysis

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                      Tiered Compilation                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Interpreter  │→ │   Baseline   │→ │  Optimized   │      │
│  │   (Tier 0)   │  │   (Tier 1)   │  │   (Tier 2)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Hot Path Detection                        │
│  • Execution frequency tracking                              │
│  • Threshold-based hot path identification                   │
│  • Performance profiling                                     │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Bytecode Compilation                       │
│  • BCI bytecode → LLVM IR translation                        │
│  • IR optimization passes                                    │
│  • Function inlining                                         │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      JIT Compiler                            │
│  • LLVM integration                                          │
│  • Native code generation                                    │
│  • Code cache management                                     │
└─────────────────────────────────────────────────────────────┘
```

## Compilation Flow

1. **Interpretation Phase**
   - Execute bytecode in interpreter
   - Track execution frequency
   - Identify hot paths

2. **Baseline Compilation**
   - Compile hot functions with minimal optimization
   - Fast compilation time
   - Moderate performance improvement

3. **Optimized Compilation**
   - Recompile frequently executed functions
   - Full optimization passes
   - Maximum performance

## Configuration

### Compilation Thresholds
```c
// Default thresholds
interpreter_to_baseline = 100 executions
baseline_to_optimized = 1000 executions
```

### Optimization Levels
- Level 0: No optimization
- Level 1: Basic optimization
- Level 2: Moderate optimization
- Level 3: Aggressive optimization

### Compilation Policies
- **Aggressive**: Quick compilation, early optimization
- **Balanced**: Balance between compilation time and performance
- **Conservative**: Optimize only proven hot paths

## Performance Characteristics

### Baseline Tier
- Compilation time: ~1ms per function
- Speedup: 2-3x over interpreter
- Use case: Frequently executed code

### Optimized Tier
- Compilation time: ~10ms per function
- Speedup: 5-10x over interpreter
- Use case: Very hot paths

## Integration

The JIT compiler integrates with:
- **BCI VM**: Executes compiled code
- **Garbage Collector**: Manages compiled code memory
- **Profiler**: Provides execution statistics

## Future Enhancements

1. **Speculative Optimization**
   - Type specialization
   - Inline caching
   - Deoptimization support

2. **Advanced Optimizations**
   - Loop vectorization
   - Escape analysis
   - Partial evaluation

3. **Profile-Guided Optimization**
   - Collect runtime profiles
   - Optimize based on actual usage patterns

4. **On-Stack Replacement**
   - Replace running interpreted code with compiled code
   - Seamless tier transitions

## Dependencies

- LLVM 14+ (for production use)
- C11 compiler
- POSIX threads (for concurrent compilation)

## Testing

See `C/tests/phase7/test_jit_compiler.c` for comprehensive tests covering:
- JIT compilation
- Tier transitions
- Hot path detection
- Performance benchmarks
