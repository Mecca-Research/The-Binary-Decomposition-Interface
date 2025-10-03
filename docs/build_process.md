
# BDI Build Process Documentation

## Overview

The BDI project uses a comprehensive Makefile-based build system with support for multiple build modes, optimization levels, and tooling integration.

## Build System Architecture

### Build Modes

The build system supports four primary build modes:

#### 1. Debug Mode (`BUILD_MODE=debug`)
```bash
make BUILD_MODE=debug
```

**Characteristics**:
- Minimal optimization (`-O0`)
- Full debug symbols (`-g3`)
- Debug assertions enabled (`-DDEBUG`)
- Frame pointer preserved (`-fno-omit-frame-pointer`)
- Address Sanitizer enabled (`-fsanitize=address`)
- Undefined Behavior Sanitizer enabled (`-fsanitize=undefined`)

**Use Cases**:
- Development and debugging
- Finding memory errors
- Detecting undefined behavior
- Stack trace generation

#### 2. Release Mode (`BUILD_MODE=release`)
```bash
make BUILD_MODE=release
```

**Characteristics**:
- Maximum optimization (`-O3`)
- Link-time optimization (`-flto`)
- Native architecture tuning (`-march=native -mtune=native`)
- Aggressive inlining
- Vectorization enabled
- Debug symbols stripped

**Use Cases**:
- Production deployments
- Performance benchmarking
- Final releases

#### 3. PGO Generation Mode (`BUILD_MODE=pgo-gen`)
```bash
make BUILD_MODE=pgo-gen
make run-benchmarks  # Generate profile data
```

**Characteristics**:
- Moderate optimization (`-O2`)
- Profile instrumentation (`-fprofile-generate`)
- Profile data directory (`./pgo-data`)
- Debug symbols for profiling

**Use Cases**:
- First phase of profile-guided optimization
- Collecting runtime profile data
- Identifying hot paths

#### 4. PGO Use Mode (`BUILD_MODE=pgo-use`)
```bash
make BUILD_MODE=pgo-use
```

**Characteristics**:
- Maximum optimization (`-O3`)
- Profile-guided optimization (`-fprofile-use`)
- Uses collected profile data
- Link-time optimization
- Native architecture tuning

**Use Cases**:
- Second phase of profile-guided optimization
- Production builds with profile data
- Maximum performance optimization

## Compiler Configuration

### C Standard
```makefile
CFLAGS := -std=c2x
```

The project uses **C23 (C2x)** standard with full feature support:
- `nullptr` keyword
- Attribute annotations (`[[nodiscard]]`, `[[maybe_unused]]`, etc.)
- `static_assert` for compile-time checks
- `typeof` and `auto` for type inference
- Enhanced type generic macros

### Warning Flags
```makefile
CFLAGS += -Wall -Wextra -Wpedantic -Werror
```

- `-Wall`: Enable all common warnings
- `-Wextra`: Enable extra warnings
- `-Wpedantic`: Strict ISO C compliance
- `-Werror`: Treat warnings as errors

### Include Paths
```makefile
CFLAGS += -I. -Ibdi_kernel -Ibdi_kernel/kernel -Ibdi_kernel/device
CFLAGS += -Ibdi_kernel/backend -Ibdi_kernel/fs -Ibdi_kernel/storage
CFLAGS += -Ibdi_kernel/usb -Ibdi_kernel/math -Ibdi_kernel/drivers
CFLAGS += -Ibdi_kernel/syscalls
```

## Build Targets

### Primary Targets

#### `all` (default)
```bash
make
```
Builds all components in the current build mode.

#### `clean`
```bash
make clean
```
Removes all build artifacts, object files, and executables.

#### `test`
```bash
make test
```
Builds and runs the test suite.

#### `install`
```bash
make install PREFIX=/usr/local
```
Installs built binaries and libraries to the specified prefix.

### Component Targets

#### BCI Library
```bash
make bci
```
Builds the Binary Computational Interface library.

#### BTL Library
```bash
make btl
```
Builds the Binary Translation Layer library.

#### Compiler
```bash
make compiler
```
Builds the complete compiler toolchain (lexer, parser, analyzer, codegen).

#### VM
```bash
make vm
```
Builds the virtual machine execution engine.

#### Kernel
```bash
make kernel
```
Builds the kernel layer components.

#### AI Trainer
```bash
make ai_trainer
```
Builds the AI training infrastructure.

## Optimization Flags

### Release Optimizations
```makefile
CFLAGS += -O3                    # Maximum optimization
CFLAGS += -flto                  # Link-time optimization
CFLAGS += -march=native          # Native CPU architecture
CFLAGS += -mtune=native          # Tune for native CPU
CFLAGS += -ffast-math            # Fast floating-point math
CFLAGS += -funroll-loops         # Loop unrolling
CFLAGS += -finline-functions     # Aggressive inlining
CFLAGS += -fomit-frame-pointer   # Omit frame pointer
```

### Vectorization
```makefile
CFLAGS += -ftree-vectorize       # Auto-vectorization
CFLAGS += -fopt-info-vec         # Vectorization reports
```

### Profile-Guided Optimization
```makefile
# Generation phase
CFLAGS += -fprofile-generate -fprofile-dir=./pgo-data

# Use phase
CFLAGS += -fprofile-use -fprofile-dir=./pgo-data
CFLAGS += -fprofile-correction   # Handle profile inconsistencies
```

## Sanitizers

### Address Sanitizer (ASan)
```bash
make BUILD_MODE=debug SANITIZER=address
```

**Detects**:
- Buffer overflows
- Use-after-free
- Use-after-return
- Memory leaks
- Double-free

### Undefined Behavior Sanitizer (UBSan)
```bash
make BUILD_MODE=debug SANITIZER=undefined
```

**Detects**:
- Integer overflow
- Null pointer dereference
- Misaligned access
- Division by zero
- Invalid shifts

### Memory Sanitizer (MSan)
```bash
make BUILD_MODE=debug SANITIZER=memory
```

**Detects**:
- Uninitialized memory reads
- Use of uninitialized values

### Thread Sanitizer (TSan)
```bash
make BUILD_MODE=debug SANITIZER=thread
```

**Detects**:
- Data races
- Deadlocks
- Thread leaks

## Static Analysis

### Clang-Tidy
```bash
make clang-tidy
```

Runs static analysis with clang-tidy using `.clang-tidy` configuration.

**Checks**:
- Bug-prone patterns
- Performance issues
- Modernization opportunities
- Readability improvements
- Security vulnerabilities

### Cppcheck
```bash
make cppcheck
```

Runs cppcheck static analyzer using `cppcheck.cfg` configuration.

**Checks**:
- Memory leaks
- Null pointer dereferences
- Buffer overflows
- Unused variables
- Logic errors

### Clang Static Analyzer
```bash
make scan-build
```

Runs Clang's static analyzer for deep analysis.

## Code Formatting

### Format Check
```bash
make format-check
```

Checks if code adheres to `.clang-format` style guide.

### Format Apply
```bash
make format
```

Automatically formats all source files according to `.clang-format`.

## Code Coverage

### Generate Coverage Report
```bash
make BUILD_MODE=debug coverage
```

**Steps**:
1. Build with coverage instrumentation (`--coverage`)
2. Run test suite
3. Generate coverage data with `lcov`
4. Create HTML report with `genhtml`

**Output**: `coverage/index.html`

### Coverage Targets
```bash
make coverage-report  # Generate HTML report
make coverage-clean   # Clean coverage data
```

## Dependency Management

### Automatic Dependency Generation
```makefile
DEPFLAGS = -MMD -MP -MF $(@:.o=.d)
```

The build system automatically generates and includes dependency files (`.d`) for each source file, ensuring correct incremental builds.

### Dependency Files
- Generated during compilation
- Track header file dependencies
- Automatically included in Makefile
- Cleaned with `make clean`

## Cross-Compilation

### ARM64 Target
```bash
make CROSS_COMPILE=aarch64-linux-gnu-
```

### RISC-V Target
```bash
make CROSS_COMPILE=riscv64-linux-gnu-
```

### Custom Toolchain
```bash
make CC=custom-gcc LD=custom-ld AR=custom-ar
```

## Parallel Builds

### Use Multiple Cores
```bash
make -j$(nproc)
```

Enables parallel compilation using all available CPU cores.

### Recommended Settings
```bash
make -j8  # Use 8 parallel jobs
```

## Installation

### Default Installation
```bash
make install
```

**Default Prefix**: `/usr/local`

**Installed Components**:
- Binaries: `$PREFIX/bin/`
- Libraries: `$PREFIX/lib/`
- Headers: `$PREFIX/include/bdi/`
- Documentation: `$PREFIX/share/doc/bdi/`

### Custom Installation
```bash
make install PREFIX=/opt/bdi DESTDIR=/tmp/staging
```

## Build Artifacts

### Object Files
- Location: `build/obj/`
- Pattern: `*.o`
- Cleaned with: `make clean`

### Dependency Files
- Location: `build/obj/`
- Pattern: `*.d`
- Cleaned with: `make clean`

### Executables
- Location: `build/bin/`
- Cleaned with: `make clean`

### Libraries
- Location: `build/lib/`
- Pattern: `*.a`, `*.so`
- Cleaned with: `make clean`

### Profile Data
- Location: `pgo-data/`
- Pattern: `*.gcda`
- Cleaned with: `make pgo-clean`

## Environment Variables

### Compiler Selection
```bash
export CC=clang
export CXX=clang++
make
```

### Build Mode
```bash
export BUILD_MODE=release
make
```

### Custom Flags
```bash
export EXTRA_CFLAGS="-DCUSTOM_FEATURE"
export EXTRA_LDFLAGS="-lcustom"
make
```

## Troubleshooting

### Build Failures

#### Missing Dependencies
```bash
# Install required packages
sudo apt-get install build-essential gcc-13 clang-16
```

#### Compiler Errors
```bash
# Check compiler version
gcc --version
clang --version

# Ensure C23 support
gcc -std=c2x -E -dM - < /dev/null | grep __STDC_VERSION__
```

#### Linker Errors
```bash
# Check library paths
echo $LD_LIBRARY_PATH

# Verify library existence
ldconfig -p | grep <library>
```

### Performance Issues

#### Slow Compilation
```bash
# Use parallel builds
make -j$(nproc)

# Use ccache
export CC="ccache gcc"
make
```

#### Large Binary Size
```bash
# Strip debug symbols
strip build/bin/*

# Use release mode
make BUILD_MODE=release
```

## Best Practices

### Development Workflow
1. Use debug mode during development
2. Run tests frequently (`make test`)
3. Use sanitizers to catch errors early
4. Run static analysis before commits
5. Format code before commits (`make format`)

### Release Workflow
1. Build with PGO for maximum performance
2. Run full test suite
3. Verify with sanitizers
4. Generate coverage report
5. Run static analysis
6. Build release binaries
7. Strip and package

### Continuous Integration
1. Build in all modes (debug, release, pgo)
2. Run tests with all sanitizers
3. Check code formatting
4. Run static analysis
5. Generate coverage reports
6. Archive build artifacts


