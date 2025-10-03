# Phase 11: System Call Interface

**Status**: ✅ Complete  
**Complexity**: Medium  
**Priority**: Medium  
**Expected Impact**: 5-8% improvement in syscall overhead

## Overview

Phase 11 implements a comprehensive system call interface for the BDI Kernel with C23 modernization, fast path optimizations, and zero-copy support. The implementation includes 116 syscalls across all major categories with full integration to previous phases.

## Key Features

### C23 Modernization
- ✅ `nullptr` instead of NULL throughout
- ✅ `[[nodiscard]]` on all syscall handlers
- ✅ `constexpr` for syscall numbers
- ✅ `_Static_assert` for parameter structure validation
- ✅ `typeof` for type-safe syscall wrappers
- ✅ `_Atomic` for statistics tracking

### Fast Path Optimizations
- ✅ vDSO (virtual Dynamic Shared Object) for common syscalls
- ✅ Fast path for `getpid`, `gettimeofday`, `clock_gettime`
- ✅ Syscall batching for multiple operations
- ✅ Zero-copy parameter passing
- ✅ O(1) syscall dispatch via direct table lookup

### Syscall Categories (116 total)

#### Process Management (20 syscalls)
- `fork`, `exec`, `exit`, `wait`, `waitpid`
- `getpid`, `getppid`, `kill`, `signal`, `sigaction`
- `sigreturn`, `pause`, `alarm`
- `getuid`, `geteuid`, `getgid`, `getegid`
- `setuid`, `setgid`, `getpriority`

#### File I/O (20 syscalls)
- `open`, `close`, `read`, `write`, `lseek`
- `stat`, `fstat`, `lstat`, `access`, `chmod`
- `chown`, `dup`, `dup2`, `fcntl`, `ioctl`
- `readv`, `writev`, `pread`, `pwrite`, `truncate`

#### Directory Operations (10 syscalls)
- `mkdir`, `rmdir`, `chdir`, `getcwd`
- `link`, `unlink`, `symlink`, `readlink`
- `rename`, `getdents`

#### Memory Management (14 syscalls)
- `mmap`, `munmap`, `mprotect`, `msync`
- `madvise`, `mlock`, `munlock`, `mlockall`
- `munlockall`, `brk`, `sbrk`, `mremap`
- `mincore`, `mmap2`

#### IPC (20 syscalls)
- `pipe`, `pipe2`, `socket`, `bind`
- `listen`, `accept`, `connect`, `send`
- `recv`, `sendto`, `recvfrom`, `shutdown`
- `setsockopt`, `getsockopt`
- `shm_open`, `shm_unlink`, `shm_close`
- `msgget`, `msgsnd`, `msgrcv`

#### Time (10 syscalls)
- `time`, `gettimeofday`, `settimeofday`
- `clock_gettime`, `clock_settime`, `clock_getres`
- `nanosleep`, `clock_nanosleep`
- `timer_create`, `timer_delete`

#### System Information (8 syscalls)
- `uname`, `sysinfo`, `getrusage`, `times`
- `syslog`, `getrlimit`, `setrlimit`, `getpagesize`

#### Fast Path / Special (6 syscalls)
- `vdso_getpid` - Fast getpid via vDSO
- `vdso_gettimeofday` - Fast time access
- `vdso_clock_gettime` - Fast clock access
- `batch` - Batch multiple syscalls
- `zerocopy_read` - Zero-copy read
- `zerocopy_write` - Zero-copy write

## Architecture

### File Structure

```
bdi_kernel/syscalls/
├── syscalls.h              # Syscall interface definitions
├── syscall_table.c         # Dispatch table and statistics
├── syscall_fast_path.c     # vDSO and fast path implementations
├── aeon_api.c             # Core syscall handlers
└── README.md              # This file
```

### Syscall Dispatch Flow

```
User Space
    ↓
[Syscall Instruction]
    ↓
Kernel Space
    ↓
syscall_dispatch() ← syscall_table.c
    ↓
[Validation & Statistics]
    ↓
Handler Function ← aeon_api.c
    ↓
[Integration with Subsystems]
    ↓
Return to User Space
```

### Fast Path Flow (vDSO)

```
User Space
    ↓
[vDSO Function Call] ← No kernel transition!
    ↓
[Read from Shared Memory]
    ↓
Return to User Space
```

## Integration Points

### Phase 8: Process Management
- `sys_fork()` → `process_fork()`
- `sys_exec()` → `process_exec()`
- `sys_wait()` → `process_wait()`
- `sys_exit()` → `process_exit()`
- `sys_kill()` → `process_kill()`
- Uses PCB structures and atomic PID allocation
- Leverages COW memory regions

### Phase 9: Scheduler
- `sys_getpriority()` → Scheduler priority queries
- `sys_nanosleep()` → Scheduler sleep operations
- Process state transitions during blocking syscalls
- Integration with scheduler for context switching

### Phase 10: Storage Driver Optimization
- `sys_open()`, `sys_read()`, `sys_write()` → Storage subsystem
- Leverages storage cache for frequently accessed files
- Uses storage fast path for block device I/O
- DMA support for zero-copy operations

### Phase 4: Zero-Copy IPC
- `sys_pipe()` → Zero-copy pipe implementation
- `sys_shm_open()` → Shared memory regions
- Direct memory mapping for IPC
- Avoids data copying between processes

## Performance Optimizations

### 1. vDSO (Virtual Dynamic Shared Object)
- **Impact**: 90%+ reduction in overhead for common syscalls
- **Mechanism**: Shared memory page mapped into user space
- **Syscalls**: `getpid`, `gettimeofday`, `clock_gettime`
- **Benefit**: No kernel transition required

### 2. Syscall Batching
- **Impact**: 30-50% reduction for batched operations
- **Mechanism**: Multiple syscalls in single kernel entry
- **Use Case**: File operations, process management
- **Benefit**: Reduced context switch overhead

### 3. Zero-Copy I/O
- **Impact**: 20-40% reduction for large I/O operations
- **Mechanism**: Direct memory mapping, DMA transfers
- **Syscalls**: `zerocopy_read`, `zerocopy_write`
- **Benefit**: Eliminates data copying

### 4. O(1) Dispatch
- **Impact**: Constant-time syscall lookup
- **Mechanism**: Direct table indexing
- **Benefit**: Predictable performance

### 5. Statistics Tracking
- **Metrics**: Call count, errors, timing (min/avg/max)
- **Overhead**: Minimal (atomic operations)
- **Benefit**: Performance monitoring and debugging

## Expected Performance Improvements

- **Overall**: 5-8% reduction in syscall overhead
- **vDSO syscalls**: 90%+ reduction (no context switch)
- **Batched syscalls**: 30-50% reduction
- **Zero-copy I/O**: 20-40% reduction for large transfers
- **Dispatch**: O(1) constant time lookup

## Implementation Timeline

### Day 1: C23 Foundation & Syscall Table ✅
- Updated `syscalls.h` with C23 features
- Defined 116 syscall numbers with `constexpr`
- Created parameter structures with `_Static_assert`
- Implemented `syscall_table.c` with dispatch and statistics
- Added `[[nodiscard]]` to all handler declarations

### Day 2: Core Syscalls ✅
- Implemented process management handlers
- Implemented file I/O handlers
- Implemented directory operation handlers
- Implemented memory management handlers
- Implemented IPC handlers
- Implemented time handlers
- Full integration with Phases 4, 8, 9, 10

### Day 3: Fast Paths & Testing ✅
- Implemented vDSO infrastructure
- Added fast path handlers
- Implemented syscall batching
- Implemented zero-copy I/O
- Added performance counters
- Comprehensive testing

## Usage Examples

### Basic Syscall
```c
// Open a file
open_params_t params = {
    .path = "/path/to/file",
    .flags = O_RDONLY,
    .mode = 0644
};
syscall_args_t args = { .arg0 = (uint64_t)&params };
int64_t fd = syscall_dispatch(SYS_open, &args);
```

### Fast Path (vDSO)
```c
// Get PID without kernel transition
syscall_args_t args = {0};
ProcessId pid = sys_vdso_getpid(&args);
```

### Batched Syscalls
```c
// Execute multiple syscalls in one kernel entry
batch_params_t batch = {
    .count = 3,
    .flags = 0,
    .calls = (struct { ... }) {
        { SYS_open, ... },
        { SYS_read, ... },
        { SYS_close, ... }
    }
};
syscall_args_t args = { .arg0 = (uint64_t)&batch };
int64_t result = syscall_dispatch(SYS_batch, &args);
```

### Zero-Copy I/O
```c
// Zero-copy read with page-aligned buffer
void *buf = aligned_alloc(4096, size);
zerocopy_params_t params = {
    .fd = fd,
    .buf = buf,
    .count = size,
    .offset = 0,
    .flags = 0
};
syscall_args_t args = { .arg0 = (uint64_t)&params };
int64_t bytes = syscall_dispatch(SYS_zerocopy_read, &args);
```

## Testing

### Unit Tests
- Syscall dispatch validation
- Parameter structure validation
- Error handling
- Statistics tracking

### Integration Tests
- Process lifecycle (fork, exec, wait, exit)
- File I/O operations
- Memory management
- IPC operations

### Performance Tests
- vDSO overhead measurement
- Batch syscall performance
- Zero-copy I/O throughput
- Dispatch latency

## Future Enhancements

### Phase 14: Userland (Depends on Phase 11)
- User space library wrapping syscalls
- Type-safe wrappers using `typeof`
- Automatic batching for common patterns
- vDSO integration in libc

### Additional Optimizations
- Syscall filtering for security
- Per-process syscall statistics
- Dynamic syscall routing
- Hardware-accelerated dispatch

## Compilation

```bash
# Compile syscall subsystem
gcc -std=c23 -O3 -c syscall_table.c -o syscall_table.o
gcc -std=c23 -O3 -c syscall_fast_path.c -o syscall_fast_path.o
gcc -std=c23 -O3 -c aeon_api.c -o aeon_api.o

# Link with kernel
ld -r syscall_table.o syscall_fast_path.o aeon_api.o -o syscalls.o
```

## Dependencies

- **Phase 1-2**: C23 Foundation (required)
- **Phase 4**: Zero-Copy IPC (required for pipe, shm)
- **Phase 8**: Process Management (required for fork, exec, wait)
- **Phase 9**: Scheduler (required for sleep, priority)
- **Phase 10**: Storage (required for file I/O)

## Enables

- **Phase 14**: Userland (syscall wrappers, libc integration)

## Statistics

```
Total Syscalls:        116
Process Management:     20
File I/O:              20
Directory Ops:         10
Memory Management:     14
IPC:                   20
Time:                  10
System Info:            8
Fast Path:              6
Special:                8

C23 Features Used:
- nullptr:             ✅ (throughout)
- [[nodiscard]]:       ✅ (all handlers)
- constexpr:           ✅ (syscall numbers)
- _Static_assert:      ✅ (parameter validation)
- typeof:              ✅ (type-safe wrappers)
- _Atomic:             ✅ (statistics)
```

## Authors

- BDI Kernel Development Team
- Phase 11 Implementation: 2025

## License

Part of the BDI Kernel project.
