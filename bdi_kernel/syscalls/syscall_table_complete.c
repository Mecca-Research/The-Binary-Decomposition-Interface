/**
 * @file syscall_table_complete.c
 * @brief Complete System Call Table with All 108 Syscalls
 * 
 * This file extends syscall_table.c to register all 108 syscalls defined
 * in the BDI kernel. It ensures every syscall has a handler registered.
 */

#include "syscalls.h"
#include <errno.h>
#include <stdio.h>

/* External handler declarations from aeon_handlers_extended.c */
extern int64_t sys_sigaction(const syscall_args_t *args);
extern int64_t sys_sigreturn(const syscall_args_t *args);
extern int64_t sys_pause(const syscall_args_t *args);
extern int64_t sys_alarm(const syscall_args_t *args);
extern int64_t sys_getuid(const syscall_args_t *args);
extern int64_t sys_geteuid(const syscall_args_t *args);
extern int64_t sys_getgid(const syscall_args_t *args);
extern int64_t sys_getegid(const syscall_args_t *args);
extern int64_t sys_setuid(const syscall_args_t *args);
extern int64_t sys_setgid(const syscall_args_t *args);
extern int64_t sys_lstat(const syscall_args_t *args);
extern int64_t sys_fcntl(const syscall_args_t *args);
extern int64_t sys_readv(const syscall_args_t *args);
extern int64_t sys_writev(const syscall_args_t *args);
extern int64_t sys_pread(const syscall_args_t *args);
extern int64_t sys_pwrite(const syscall_args_t *args);
extern int64_t sys_truncate(const syscall_args_t *args);
extern int64_t sys_link(const syscall_args_t *args);
extern int64_t sys_symlink(const syscall_args_t *args);
extern int64_t sys_readlink(const syscall_args_t *args);
extern int64_t sys_rename(const syscall_args_t *args);
extern int64_t sys_getdents(const syscall_args_t *args);
extern int64_t sys_msync(const syscall_args_t *args);
extern int64_t sys_madvise(const syscall_args_t *args);
extern int64_t sys_mlock(const syscall_args_t *args);
extern int64_t sys_munlock(const syscall_args_t *args);
extern int64_t sys_mlockall(const syscall_args_t *args);
extern int64_t sys_munlockall(const syscall_args_t *args);
extern int64_t sys_sbrk(const syscall_args_t *args);
extern int64_t sys_mremap(const syscall_args_t *args);
extern int64_t sys_mincore(const syscall_args_t *args);
extern int64_t sys_mmap2(const syscall_args_t *args);
extern int64_t sys_bind(const syscall_args_t *args);
extern int64_t sys_listen(const syscall_args_t *args);
extern int64_t sys_accept(const syscall_args_t *args);
extern int64_t sys_connect(const syscall_args_t *args);
extern int64_t sys_send(const syscall_args_t *args);
extern int64_t sys_recv(const syscall_args_t *args);
extern int64_t sys_sendto(const syscall_args_t *args);
extern int64_t sys_recvfrom(const syscall_args_t *args);
extern int64_t sys_shutdown(const syscall_args_t *args);
extern int64_t sys_setsockopt(const syscall_args_t *args);
extern int64_t sys_getsockopt(const syscall_args_t *args);
extern int64_t sys_shm_unlink(const syscall_args_t *args);
extern int64_t sys_msgget(const syscall_args_t *args);
extern int64_t sys_msgsnd(const syscall_args_t *args);
extern int64_t sys_msgrcv(const syscall_args_t *args);
extern int64_t sys_time(const syscall_args_t *args);
extern int64_t sys_settimeofday(const syscall_args_t *args);
extern int64_t sys_clock_settime(const syscall_args_t *args);
extern int64_t sys_clock_getres(const syscall_args_t *args);
extern int64_t sys_clock_nanosleep(const syscall_args_t *args);
extern int64_t sys_timer_create(const syscall_args_t *args);
extern int64_t sys_timer_delete(const syscall_args_t *args);
extern int64_t sys_uname(const syscall_args_t *args);
extern int64_t sys_sysinfo(const syscall_args_t *args);
extern int64_t sys_getrusage(const syscall_args_t *args);
extern int64_t sys_times(const syscall_args_t *args);
extern int64_t sys_syslog(const syscall_args_t *args);
extern int64_t sys_getrlimit(const syscall_args_t *args);
extern int64_t sys_setrlimit(const syscall_args_t *args);
extern int64_t sys_getpagesize(const syscall_args_t *args);

/**
 * @brief Register all remaining syscalls
 * 
 * This function is called after syscall_init() to register the
 * remaining syscalls that were not registered in the base table.
 * 
 * CRITICAL FIX: This function now actually registers all 108 syscalls
 * instead of just printing a stub message. Without this, all extended
 * syscalls would return -ENOSYS at runtime.
 * 
 * @return 0 on success, negative errno on failure
 */
int syscall_register_extended(void) {
    int result = 0;
    int registered_count = 0;
    
    printf("syscall_table_complete: Registering extended syscalls...\n");
    
    /* ===================================================================
     * Process Management Syscalls (Extended)
     * =================================================================== */
    
    /* Signal handling syscalls (9-12) */
    result |= register_syscall(SYS_sigaction, sys_sigaction, "sigaction", 0);
    result |= register_syscall(SYS_sigreturn, sys_sigreturn, "sigreturn", 0);
    result |= register_syscall(SYS_pause, sys_pause, "pause", 0);
    result |= register_syscall(SYS_alarm, sys_alarm, "alarm", SYSCALL_FLAG_BATCHABLE);
    registered_count += 4;
    
    /* User/Group ID syscalls (13-18) */
    result |= register_syscall(SYS_getuid, sys_getuid, "getuid", 
                              SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_geteuid, sys_geteuid, "geteuid", 
                              SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_getgid, sys_getgid, "getgid", 
                              SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_getegid, sys_getegid, "getegid", 
                              SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_setuid, sys_setuid, "setuid", 0);
    result |= register_syscall(SYS_setgid, sys_setgid, "setgid", 0);
    registered_count += 6;
    
    /* ===================================================================
     * File I/O Syscalls (Extended)
     * =================================================================== */
    
    /* Extended file operations (27, 33-39) */
    result |= register_syscall(SYS_lstat, sys_lstat, "lstat", SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_fcntl, sys_fcntl, "fcntl", SYSCALL_FLAG_BATCHABLE);
    result |= register_syscall(SYS_readv, sys_readv, "readv", SYSCALL_FLAG_ZERO_COPY);
    result |= register_syscall(SYS_writev, sys_writev, "writev", SYSCALL_FLAG_ZERO_COPY);
    result |= register_syscall(SYS_pread, sys_pread, "pread", SYSCALL_FLAG_ZERO_COPY);
    result |= register_syscall(SYS_pwrite, sys_pwrite, "pwrite", SYSCALL_FLAG_ZERO_COPY);
    result |= register_syscall(SYS_truncate, sys_truncate, "truncate", 0);
    registered_count += 7;
    
    /* ===================================================================
     * Directory Operations (Extended)
     * =================================================================== */
    
    /* Extended directory operations (44-49) */
    result |= register_syscall(SYS_link, sys_link, "link", SYSCALL_FLAG_BATCHABLE);
    result |= register_syscall(SYS_symlink, sys_symlink, "symlink", SYSCALL_FLAG_BATCHABLE);
    result |= register_syscall(SYS_readlink, sys_readlink, "readlink", SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_rename, sys_rename, "rename", SYSCALL_FLAG_BATCHABLE);
    result |= register_syscall(SYS_getdents, sys_getdents, "getdents", SYSCALL_FLAG_READONLY);
    registered_count += 5;
    
    /* ===================================================================
     * Memory Management Syscalls (Extended)
     * =================================================================== */
    
    /* Extended memory management (53-63) */
    result |= register_syscall(SYS_msync, sys_msync, "msync", 0);
    result |= register_syscall(SYS_madvise, sys_madvise, "madvise", 0);
    result |= register_syscall(SYS_mlock, sys_mlock, "mlock", 0);
    result |= register_syscall(SYS_munlock, sys_munlock, "munlock", 0);
    result |= register_syscall(SYS_mlockall, sys_mlockall, "mlockall", 0);
    result |= register_syscall(SYS_munlockall, sys_munlockall, "munlockall", 0);
    result |= register_syscall(SYS_sbrk, sys_sbrk, "sbrk", 0);
    result |= register_syscall(SYS_mremap, sys_mremap, "mremap", 0);
    result |= register_syscall(SYS_mincore, sys_mincore, "mincore", SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_mmap2, sys_mmap2, "mmap2", 0);
    registered_count += 10;
    
    /* ===================================================================
     * IPC Syscalls (Extended)
     * =================================================================== */
    
    /* Socket operations (73-83) */
    result |= register_syscall(SYS_bind, sys_bind, "bind", 0);
    result |= register_syscall(SYS_listen, sys_listen, "listen", 0);
    result |= register_syscall(SYS_accept, sys_accept, "accept", 0);
    result |= register_syscall(SYS_connect, sys_connect, "connect", 0);
    result |= register_syscall(SYS_send, sys_send, "send", SYSCALL_FLAG_ZERO_COPY);
    result |= register_syscall(SYS_recv, sys_recv, "recv", SYSCALL_FLAG_ZERO_COPY);
    result |= register_syscall(SYS_sendto, sys_sendto, "sendto", SYSCALL_FLAG_ZERO_COPY);
    result |= register_syscall(SYS_recvfrom, sys_recvfrom, "recvfrom", SYSCALL_FLAG_ZERO_COPY);
    result |= register_syscall(SYS_shutdown, sys_shutdown, "shutdown", 0);
    result |= register_syscall(SYS_setsockopt, sys_setsockopt, "setsockopt", 0);
    result |= register_syscall(SYS_getsockopt, sys_getsockopt, "getsockopt", SYSCALL_FLAG_READONLY);
    registered_count += 11;
    
    /* Shared memory and message queue operations (85-89) */
    result |= register_syscall(SYS_shm_unlink, sys_shm_unlink, "shm_unlink", 0);
    result |= register_syscall(SYS_msgget, sys_msgget, "msgget", 0);
    result |= register_syscall(SYS_msgsnd, sys_msgsnd, "msgsnd", 0);
    result |= register_syscall(SYS_msgrcv, sys_msgrcv, "msgrcv", 0);
    registered_count += 4;
    
    /* ===================================================================
     * Time Syscalls (Extended)
     * =================================================================== */
    
    /* Extended time operations (90, 92, 94-99) */
    result |= register_syscall(SYS_time, sys_time, "time", 
                              SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_settimeofday, sys_settimeofday, "settimeofday", 0);
    result |= register_syscall(SYS_clock_settime, sys_clock_settime, "clock_settime", 0);
    result |= register_syscall(SYS_clock_getres, sys_clock_getres, "clock_getres", 
                              SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_clock_nanosleep, sys_clock_nanosleep, "clock_nanosleep", 0);
    result |= register_syscall(SYS_timer_create, sys_timer_create, "timer_create", 0);
    result |= register_syscall(SYS_timer_delete, sys_timer_delete, "timer_delete", 0);
    registered_count += 7;
    
    /* ===================================================================
     * System Information Syscalls (Extended)
     * =================================================================== */
    
    /* System information operations (100-107) */
    result |= register_syscall(SYS_uname, sys_uname, "uname", SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_sysinfo, sys_sysinfo, "sysinfo", SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_getrusage, sys_getrusage, "getrusage", SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_times, sys_times, "times", SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_syslog, sys_syslog, "syslog", 0);
    result |= register_syscall(SYS_getrlimit, sys_getrlimit, "getrlimit", SYSCALL_FLAG_READONLY);
    result |= register_syscall(SYS_setrlimit, sys_setrlimit, "setrlimit", 0);
    result |= register_syscall(SYS_getpagesize, sys_getpagesize, "getpagesize", 
                              SYSCALL_FLAG_FAST_PATH | SYSCALL_FLAG_READONLY);
    registered_count += 8;
    
    /* ===================================================================
     * Summary
     * =================================================================== */
    
    if (result != 0) {
        fprintf(stderr, "syscall_table_complete: Failed to register some syscalls\n");
        return -EIO;
    }
    
    printf("syscall_table_complete: Successfully registered %d extended syscalls\n", 
           registered_count);
    printf("syscall_table_complete: Total syscalls available: %d\n", SYSCALL_COUNT);
    
    return 0;
}
