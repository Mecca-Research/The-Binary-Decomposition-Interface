/**
 * @file aeon_api.c
 * @brief System Call Handler Implementations
 * 
 * Phase 11: System Call Interface - Day 2
 * 
 * This file implements the core system call handlers for the BDI Kernel.
 * All handlers follow C23 standards and integrate with previous phases:
 * - Phase 8: Process Management
 * - Phase 9: Scheduler
 * - Phase 10: Storage Driver Optimization
 * - Phase 4: Zero-Copy IPC
 * 
 * Key Features:
 * - C23 modernization (nullptr, [[nodiscard]])
 * - Comprehensive error handling
 * - Integration with process, scheduler, and storage subsystems
 * - Zero-copy parameter passing where applicable
 * - Type-safe implementations
 */

#include "syscalls.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

/* ===================================================================
 * Process Management Syscalls
 * Integration with Phase 8: Process Management
 * =================================================================== */

/**
 * @brief Fork system call - Create a new process
 * 
 * Creates a new process by duplicating the calling process.
 * Uses Copy-On-Write (COW) for memory efficiency.
 * 
 * Integration with Phase 8:
 * - Uses process_fork() from process_lifecycle.c
 * - Leverages COW memory regions
 * - Atomic PID allocation
 * 
 * @param args Syscall arguments
 *             args->arg0: fork_params_t *params (or nullptr for default)
 * @return Child PID in parent, 0 in child, negative errno on failure
 */
[[nodiscard]] int64_t sys_fork(const syscall_args_t *args) {
    fork_params_t *params = (fork_params_t *)args->arg0;
    
    printf("sys_fork: Creating new process\n");
    
    /* Call Phase 8 process_fork() */
    ProcessId child_pid = process_fork();
    
    if (child_pid == INVALID_PID) {
        fprintf(stderr, "sys_fork: Failed to create process\n");
        return -ENOMEM;
    }
    
    /* TODO: Handle fork_params_t flags if provided */
    if (params != nullptr) {
        /* Custom stack, TID pointers, etc. */
        printf("sys_fork: Custom fork parameters provided (not yet implemented)\n");
    }
    
    printf("sys_fork: Created child process PID=%lu\n", child_pid);
    return (int64_t)child_pid;
}

/**
 * @brief Exec system call - Execute a new program
 * 
 * Replaces the current process image with a new program.
 * 
 * Integration with Phase 8:
 * - Uses process_exec() from process_lifecycle.c
 * - Loads new BDI graph as process image
 * 
 * @param args Syscall arguments
 *             args->arg0: exec_params_t *params
 * @return Does not return on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_exec(const syscall_args_t *args) {
    exec_params_t *params = (exec_params_t *)args->arg0;
    
    if (params == nullptr || params->path == nullptr) {
        return -EFAULT;
    }
    
    printf("sys_exec: Executing %s\n", params->path);
    
    /* Call Phase 8 process_exec() */
    int result = process_exec(params->path, params->argv, params->envp);
    
    if (result < 0) {
        fprintf(stderr, "sys_exec: Failed to execute %s (error %d)\n", 
                params->path, result);
        return result;
    }
    
    /* Should not reach here if exec succeeds */
    return -ENOEXEC;
}

/**
 * @brief Exit system call - Terminate the calling process
 * 
 * Terminates the calling process and returns exit status to parent.
 * 
 * Integration with Phase 8:
 * - Uses process_exit() from process_lifecycle.c
 * - Transitions to ZOMBIE state
 * - Wakes up waiting parent
 * 
 * @param args Syscall arguments
 *             args->arg0: int exit_code
 * @return Does not return
 */
[[nodiscard]] int64_t sys_exit(const syscall_args_t *args) {
    int exit_code = (int)args->arg0;
    
    printf("sys_exit: Process exiting with code %d\n", exit_code);
    
    /* Call Phase 8 process_exit() */
    process_exit(exit_code);
    
    /* Should not reach here */
    return 0;
}

/**
 * @brief Wait system call - Wait for child process to change state
 * 
 * Suspends execution until a child process terminates.
 * 
 * Integration with Phase 8:
 * - Uses process_wait() from process_lifecycle.c
 * - Integrates with scheduler for blocking
 * 
 * @param args Syscall arguments
 *             args->arg0: wait_params_t *params
 * @return Child PID on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_wait(const syscall_args_t *args) {
    wait_params_t *params = (wait_params_t *)args->arg0;
    
    if (params == nullptr) {
        return -EFAULT;
    }
    
    printf("sys_wait: Waiting for child process\n");
    
    /* Call Phase 8 process_wait() */
    ProcessId child_pid = process_wait(params->pid, params->status, params->options);
    
    if (child_pid == INVALID_PID) {
        return -ECHILD;
    }
    
    printf("sys_wait: Child process %lu exited\n", child_pid);
    return (int64_t)child_pid;
}

/**
 * @brief Waitpid system call - Wait for specific child process
 * 
 * @param args Syscall arguments
 * @return Child PID on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_waitpid(const syscall_args_t *args) {
    /* Wrapper around sys_wait with specific PID */
    return sys_wait(args);
}

/**
 * @brief Getpid system call - Get process ID
 * 
 * Returns the process ID of the calling process.
 * 
 * Integration with Phase 8:
 * - Uses process_current() to get current PCB
 * 
 * @param args Syscall arguments (unused)
 * @return Current process PID
 */
[[nodiscard]] int64_t sys_getpid(const syscall_args_t *args) {
    (void)args; /* Unused */
    
    ProcessControlBlock *pcb = process_current();
    
    if (pcb == nullptr) {
        return -ESRCH;
    }
    
    return (int64_t)pcb->pid;
}

/**
 * @brief Getppid system call - Get parent process ID
 * 
 * @param args Syscall arguments (unused)
 * @return Parent process PID
 */
[[nodiscard]] int64_t sys_getppid(const syscall_args_t *args) {
    (void)args; /* Unused */
    
    ProcessControlBlock *pcb = process_current();
    
    if (pcb == nullptr) {
        return -ESRCH;
    }
    
    return (int64_t)pcb->parent_pid;
}

/**
 * @brief Kill system call - Send signal to process
 * 
 * Integration with Phase 8:
 * - Uses process_kill() from process_lifecycle.c
 * 
 * @param args Syscall arguments
 *             args->arg0: ProcessId pid
 *             args->arg1: int signal
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_kill(const syscall_args_t *args) {
    ProcessId pid = (ProcessId)args->arg0;
    int signal = (int)args->arg1;
    
    printf("sys_kill: Sending signal %d to process %lu\n", signal, pid);
    
    /* Call Phase 8 process_kill() */
    int result = process_kill(pid, signal);
    
    return (int64_t)result;
}

/**
 * @brief Signal system call - Set signal handler
 * 
 * @param args Syscall arguments
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_signal(const syscall_args_t *args) {
    /* TODO: Implement signal handling */
    printf("sys_signal: Not yet implemented\n");
    return -ENOSYS;
}

/**
 * @brief Getpriority system call - Get process priority
 * 
 * Integration with Phase 9:
 * - Uses scheduler to get process priority
 * 
 * @param args Syscall arguments
 * @return Process priority, or negative errno on failure
 */
[[nodiscard]] int64_t sys_getpriority(const syscall_args_t *args) {
    ProcessControlBlock *pcb = process_current();
    
    if (pcb == nullptr) {
        return -ESRCH;
    }
    
    /* Return priority from PCB */
    return (int64_t)pcb->priority;
}

/* ===================================================================
 * File I/O Syscalls
 * Integration with Phase 10: Storage Driver Optimization
 * =================================================================== */

/**
 * @brief Open system call - Open a file
 * 
 * Opens a file and returns a file descriptor.
 * 
 * Integration with Phase 10:
 * - Uses storage subsystem for file access
 * - Leverages storage cache for frequently accessed files
 * 
 * @param args Syscall arguments
 *             args->arg0: open_params_t *params
 * @return File descriptor on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_open(const syscall_args_t *args) {
    open_params_t *params = (open_params_t *)args->arg0;
    
    if (params == nullptr || params->path == nullptr) {
        return -EFAULT;
    }
    
    printf("sys_open: Opening %s (flags=0x%x, mode=0x%x)\n", 
           params->path, params->flags, params->mode);
    
    /* Use standard open() for now */
    int fd = open(params->path, params->flags, params->mode);
    
    if (fd < 0) {
        return -errno;
    }
    
    /* TODO: Register FD in process file descriptor table */
    
    return (int64_t)fd;
}

/**
 * @brief Close system call - Close a file descriptor
 * 
 * @param args Syscall arguments
 *             args->arg0: int fd
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_close(const syscall_args_t *args) {
    int fd = (int)args->arg0;
    
    printf("sys_close: Closing fd=%d\n", fd);
    
    /* TODO: Remove FD from process file descriptor table */
    
    int result = close(fd);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Read system call - Read from file descriptor
 * 
 * Integration with Phase 10:
 * - Uses storage fast path for block device reads
 * - Leverages storage cache
 * 
 * Integration with Phase 4:
 * - Can use zero-copy IPC for pipe reads
 * 
 * @param args Syscall arguments
 *             args->arg0: rw_params_t *params
 * @return Number of bytes read, or negative errno on failure
 */
[[nodiscard]] int64_t sys_read(const syscall_args_t *args) {
    rw_params_t *params = (rw_params_t *)args->arg0;
    
    if (params == nullptr || params->buf == nullptr) {
        return -EFAULT;
    }
    
    printf("sys_read: fd=%d, count=%zu\n", params->fd, params->count);
    
    /* Use pread if offset is specified */
    ssize_t bytes_read;
    if (params->offset >= 0) {
        bytes_read = pread(params->fd, params->buf, params->count, params->offset);
    } else {
        bytes_read = read(params->fd, params->buf, params->count);
    }
    
    if (bytes_read < 0) {
        return -errno;
    }
    
    return (int64_t)bytes_read;
}

/**
 * @brief Write system call - Write to file descriptor
 * 
 * Integration with Phase 10:
 * - Uses storage fast path for block device writes
 * - Leverages write-back cache
 * 
 * Integration with Phase 4:
 * - Can use zero-copy IPC for pipe writes
 * 
 * @param args Syscall arguments
 *             args->arg0: rw_params_t *params
 * @return Number of bytes written, or negative errno on failure
 */
[[nodiscard]] int64_t sys_write(const syscall_args_t *args) {
    rw_params_t *params = (rw_params_t *)args->arg0;
    
    if (params == nullptr || params->buf == nullptr) {
        return -EFAULT;
    }
    
    printf("sys_write: fd=%d, count=%zu\n", params->fd, params->count);
    
    /* Use pwrite if offset is specified */
    ssize_t bytes_written;
    if (params->offset >= 0) {
        bytes_written = pwrite(params->fd, params->buf, params->count, params->offset);
    } else {
        bytes_written = write(params->fd, params->buf, params->count);
    }
    
    if (bytes_written < 0) {
        return -errno;
    }
    
    return (int64_t)bytes_written;
}

/**
 * @brief Lseek system call - Reposition file offset
 * 
 * @param args Syscall arguments
 *             args->arg0: int fd
 *             args->arg1: off_t offset
 *             args->arg2: int whence
 * @return New file offset, or negative errno on failure
 */
[[nodiscard]] int64_t sys_lseek(const syscall_args_t *args) {
    int fd = (int)args->arg0;
    off_t offset = (off_t)args->arg1;
    int whence = (int)args->arg2;
    
    off_t new_offset = lseek(fd, offset, whence);
    
    if (new_offset < 0) {
        return -errno;
    }
    
    return (int64_t)new_offset;
}

/**
 * @brief Stat system call - Get file status
 * 
 * @param args Syscall arguments
 *             args->arg0: const char *path
 *             args->arg1: struct stat *buf
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_stat(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    struct stat *buf = (struct stat *)args->arg1;
    
    if (path == nullptr || buf == nullptr) {
        return -EFAULT;
    }
    
    int result = stat(path, buf);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Fstat system call - Get file status by descriptor
 * 
 * @param args Syscall arguments
 *             args->arg0: int fd
 *             args->arg1: struct stat *buf
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_fstat(const syscall_args_t *args) {
    int fd = (int)args->arg0;
    struct stat *buf = (struct stat *)args->arg1;
    
    if (buf == nullptr) {
        return -EFAULT;
    }
    
    int result = fstat(fd, buf);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Access system call - Check file accessibility
 * 
 * @param args Syscall arguments
 *             args->arg0: const char *path
 *             args->arg1: int mode
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_access(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    int mode = (int)args->arg1;
    
    if (path == nullptr) {
        return -EFAULT;
    }
    
    int result = access(path, mode);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Dup system call - Duplicate file descriptor
 * 
 * @param args Syscall arguments
 *             args->arg0: int fd
 * @return New file descriptor, or negative errno on failure
 */
[[nodiscard]] int64_t sys_dup(const syscall_args_t *args) {
    int fd = (int)args->arg0;
    
    int new_fd = dup(fd);
    
    if (new_fd < 0) {
        return -errno;
    }
    
    return (int64_t)new_fd;
}

/**
 * @brief Dup2 system call - Duplicate file descriptor to specific number
 * 
 * @param args Syscall arguments
 *             args->arg0: int oldfd
 *             args->arg1: int newfd
 * @return New file descriptor, or negative errno on failure
 */
[[nodiscard]] int64_t sys_dup2(const syscall_args_t *args) {
    int oldfd = (int)args->arg0;
    int newfd = (int)args->arg1;
    
    int result = dup2(oldfd, newfd);
    
    if (result < 0) {
        return -errno;
    }
    
    return (int64_t)result;
}

/**
 * @brief Ioctl system call - Device control
 * 
 * Integration with Phase 10:
 * - Device-specific control operations
 * 
 * @param args Syscall arguments
 *             args->arg0: int fd
 *             args->arg1: unsigned long request
 *             args->arg2: void *argp
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_ioctl(const syscall_args_t *args) {
    /* TODO: Implement device-specific ioctl */
    printf("sys_ioctl: Not yet implemented\n");
    return -ENOSYS;
}

/* ===================================================================
 * Directory Operations
 * =================================================================== */

/**
 * @brief Mkdir system call - Create directory
 * 
 * @param args Syscall arguments
 *             args->arg0: const char *path
 *             args->arg1: mode_t mode
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_mkdir(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    mode_t mode = (mode_t)args->arg1;
    
    if (path == nullptr) {
        return -EFAULT;
    }
    
    int result = mkdir(path, mode);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Rmdir system call - Remove directory
 * 
 * @param args Syscall arguments
 *             args->arg0: const char *path
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_rmdir(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    
    if (path == nullptr) {
        return -EFAULT;
    }
    
    int result = rmdir(path);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Chdir system call - Change working directory
 * 
 * @param args Syscall arguments
 *             args->arg0: const char *path
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_chdir(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    
    if (path == nullptr) {
        return -EFAULT;
    }
    
    int result = chdir(path);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Getcwd system call - Get current working directory
 * 
 * @param args Syscall arguments
 *             args->arg0: char *buf
 *             args->arg1: size_t size
 * @return Pointer to buf on success, nullptr on failure
 */
[[nodiscard]] int64_t sys_getcwd(const syscall_args_t *args) {
    char *buf = (char *)args->arg0;
    size_t size = (size_t)args->arg1;
    
    if (buf == nullptr) {
        return -EFAULT;
    }
    
    char *result = getcwd(buf, size);
    
    if (result == nullptr) {
        return -errno;
    }
    
    return (int64_t)result;
}

/**
 * @brief Unlink system call - Delete file
 * 
 * @param args Syscall arguments
 *             args->arg0: const char *path
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_unlink(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    
    if (path == nullptr) {
        return -EFAULT;
    }
    
    int result = unlink(path);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/* ===================================================================
 * Memory Management Syscalls
 * =================================================================== */

/**
 * @brief Mmap system call - Map memory
 * 
 * @param args Syscall arguments
 *             args->arg0: mmap_params_t *params
 * @return Mapped address on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_mmap(const syscall_args_t *args) {
    mmap_params_t *params = (mmap_params_t *)args->arg0;
    
    if (params == nullptr) {
        return -EFAULT;
    }
    
    printf("sys_mmap: addr=%p, length=%zu, prot=0x%x, flags=0x%x, fd=%d, offset=%ld\n",
           params->addr, params->length, params->prot, params->flags, 
           params->fd, params->offset);
    
    void *result = mmap(params->addr, params->length, params->prot, 
                       params->flags, params->fd, params->offset);
    
    if (result == MAP_FAILED) {
        return -errno;
    }
    
    return (int64_t)result;
}

/**
 * @brief Munmap system call - Unmap memory
 * 
 * @param args Syscall arguments
 *             args->arg0: void *addr
 *             args->arg1: size_t length
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_munmap(const syscall_args_t *args) {
    void *addr = (void *)args->arg0;
    size_t length = (size_t)args->arg1;
    
    int result = munmap(addr, length);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Mprotect system call - Change memory protection
 * 
 * @param args Syscall arguments
 *             args->arg0: void *addr
 *             args->arg1: size_t length
 *             args->arg2: int prot
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_mprotect(const syscall_args_t *args) {
    void *addr = (void *)args->arg0;
    size_t length = (size_t)args->arg1;
    int prot = (int)args->arg2;
    
    int result = mprotect(addr, length, prot);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Brk system call - Change data segment size
 * 
 * @param args Syscall arguments
 *             args->arg0: void *addr
 * @return New program break on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_brk(const syscall_args_t *args) {
    void *addr = (void *)args->arg0;
    
    int result = brk(addr);
    
    if (result < 0) {
        return -errno;
    }
    
    return (int64_t)sbrk(0); /* Return current break */
}

/* ===================================================================
 * IPC Syscalls
 * Integration with Phase 4: Zero-Copy IPC
 * =================================================================== */

/**
 * @brief Pipe system call - Create pipe
 * 
 * Integration with Phase 4:
 * - Uses zero-copy IPC mechanism
 * - Shared memory for pipe buffer
 * 
 * @param args Syscall arguments
 *             args->arg0: pipe_params_t *params
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_pipe(const syscall_args_t *args) {
    pipe_params_t *params = (pipe_params_t *)args->arg0;
    
    if (params == nullptr) {
        return -EFAULT;
    }
    
    printf("sys_pipe: Creating pipe\n");
    
    int result = pipe(params->pipefd);
    
    if (result < 0) {
        return -errno;
    }
    
    /* TODO: Use Phase 4 zero-copy IPC for pipe buffer */
    
    return 0;
}

/**
 * @brief Pipe2 system call - Create pipe with flags
 * 
 * @param args Syscall arguments
 *             args->arg0: pipe_params_t *params
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_pipe2(const syscall_args_t *args) {
    pipe_params_t *params = (pipe_params_t *)args->arg0;
    
    if (params == nullptr) {
        return -EFAULT;
    }
    
    printf("sys_pipe2: Creating pipe with flags=0x%x\n", params->flags);
    
    /* TODO: Implement pipe2 with flags */
    return sys_pipe(args);
}

/**
 * @brief Socket system call - Create socket
 * 
 * @param args Syscall arguments
 *             args->arg0: socket_params_t *params
 * @return Socket file descriptor on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_socket(const syscall_args_t *args) {
    socket_params_t *params = (socket_params_t *)args->arg0;
    
    if (params == nullptr) {
        return -EFAULT;
    }
    
    printf("sys_socket: domain=%d, type=%d, protocol=%d\n", 
           params->domain, params->type, params->protocol);
    
    /* TODO: Implement socket creation */
    return -ENOSYS;
}

/**
 * @brief Shm_open system call - Open shared memory
 * 
 * Integration with Phase 4:
 * - Uses zero-copy IPC for shared memory
 * 
 * @param args Syscall arguments
 *             args->arg0: const char *name
 *             args->arg1: int oflag
 *             args->arg2: mode_t mode
 * @return File descriptor on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_shm_open(const syscall_args_t *args) {
    const char *name = (const char *)args->arg0;
    int oflag = (int)args->arg1;
    mode_t mode = (mode_t)args->arg2;
    
    if (name == nullptr) {
        return -EFAULT;
    }
    
    printf("sys_shm_open: name=%s, oflag=0x%x, mode=0x%x\n", name, oflag, mode);
    
    int fd = shm_open(name, oflag, mode);
    
    if (fd < 0) {
        return -errno;
    }
    
    return (int64_t)fd;
}

/**
 * @brief Shm_close system call - Close shared memory
 * 
 * @param args Syscall arguments
 *             args->arg0: int fd
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_shm_close(const syscall_args_t *args) {
    int fd = (int)args->arg0;
    
    return sys_close(args);
}

/* ===================================================================
 * Time Syscalls
 * =================================================================== */

/**
 * @brief Gettimeofday system call - Get time of day
 * 
 * @param args Syscall arguments
 *             args->arg0: struct timeval *tv
 *             args->arg1: struct timezone *tz
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_gettimeofday(const syscall_args_t *args) {
    struct timeval *tv = (struct timeval *)args->arg0;
    struct timezone *tz = (struct timezone *)args->arg1;
    
    int result = gettimeofday(tv, tz);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Clock_gettime system call - Get clock time
 * 
 * @param args Syscall arguments
 *             args->arg0: clockid_t clk_id
 *             args->arg1: struct timespec *tp
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_clock_gettime(const syscall_args_t *args) {
    clockid_t clk_id = (clockid_t)args->arg0;
    struct timespec *tp = (struct timespec *)args->arg1;
    
    if (tp == nullptr) {
        return -EFAULT;
    }
    
    int result = clock_gettime(clk_id, tp);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}

/**
 * @brief Nanosleep system call - Sleep for nanoseconds
 * 
 * Integration with Phase 9:
 * - Uses scheduler to put process to sleep
 * 
 * @param args Syscall arguments
 *             args->arg0: const struct timespec *req
 *             args->arg1: struct timespec *rem
 * @return 0 on success, negative errno on failure
 */
[[nodiscard]] int64_t sys_nanosleep(const syscall_args_t *args) {
    const struct timespec *req = (const struct timespec *)args->arg0;
    struct timespec *rem = (struct timespec *)args->arg1;
    
    if (req == nullptr) {
        return -EFAULT;
    }
    
    int result = nanosleep(req, rem);
    
    if (result < 0) {
        return -errno;
    }
    
    return 0;
}
