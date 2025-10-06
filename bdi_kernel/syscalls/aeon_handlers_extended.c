
/**
 * @file aeon_handlers_extended.c
 * @brief Extended AEON API Handlers
 * 
 * This file implements the remaining syscall handlers that were not
 * implemented in aeon_api.c, completing the full set of 108 syscalls.
 */

#include "syscalls.h"
#include "../process/process.h"
#include "../scheduler/scheduler.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/sysinfo.h>

/* ===================================================================
 * Signal Handling Syscalls
 * =================================================================== */

[[nodiscard]] int64_t sys_sigaction(const syscall_args_t *args) {
    printf("sys_sigaction: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_sigreturn(const syscall_args_t *args) {
    printf("sys_sigreturn: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_pause(const syscall_args_t *args) {
    printf("sys_pause: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_alarm(const syscall_args_t *args) {
    printf("sys_alarm: Not yet implemented\n");
    return -ENOSYS;
}

/* ===================================================================
 * User/Group ID Syscalls
 * =================================================================== */

[[nodiscard]] int64_t sys_getuid(const syscall_args_t *args) {
    (void)args;
    return (int64_t)getuid();
}

[[nodiscard]] int64_t sys_geteuid(const syscall_args_t *args) {
    (void)args;
    return (int64_t)geteuid();
}

[[nodiscard]] int64_t sys_getgid(const syscall_args_t *args) {
    (void)args;
    return (int64_t)getgid();
}

[[nodiscard]] int64_t sys_getegid(const syscall_args_t *args) {
    (void)args;
    return (int64_t)getegid();
}

[[nodiscard]] int64_t sys_setuid(const syscall_args_t *args) {
    uid_t uid = (uid_t)args->arg0;
    int result = setuid(uid);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_setgid(const syscall_args_t *args) {
    gid_t gid = (gid_t)args->arg0;
    int result = setgid(gid);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_chmod(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    mode_t mode = (mode_t)args->arg1;
    
    if (path == nullptr) {
        return -EFAULT;
    }
    
    int result = chmod(path, mode);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_chown(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    uid_t owner = (uid_t)args->arg1;
    gid_t group = (gid_t)args->arg2;
    
    if (path == nullptr) {
        return -EFAULT;
    }
    
    int result = chown(path, owner, group);
    return (result < 0) ? -errno : 0;
}

/* ===================================================================
 * Extended File Operations
 * =================================================================== */

[[nodiscard]] int64_t sys_lstat(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    struct stat *buf = (struct stat *)args->arg1;
    
    if (path == nullptr || buf == nullptr) {
        return -EFAULT;
    }
    
    int result = lstat(path, buf);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_fcntl(const syscall_args_t *args) {
    int fd = (int)args->arg0;
    int cmd = (int)args->arg1;
    unsigned long arg = args->arg2;
    
    int result = fcntl(fd, cmd, arg);
    return (result < 0) ? -errno : result;
}

[[nodiscard]] int64_t sys_readv(const syscall_args_t *args) {
    printf("sys_readv: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_writev(const syscall_args_t *args) {
    printf("sys_writev: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_pread(const syscall_args_t *args) {
    int fd = (int)args->arg0;
    void *buf = (void *)args->arg1;
    size_t count = (size_t)args->arg2;
    off_t offset = (off_t)args->arg3;
    
    if (buf == nullptr) {
        return -EFAULT;
    }
    
    ssize_t result = pread(fd, buf, count, offset);
    return (result < 0) ? -errno : result;
}

[[nodiscard]] int64_t sys_pwrite(const syscall_args_t *args) {
    int fd = (int)args->arg0;
    const void *buf = (const void *)args->arg1;
    size_t count = (size_t)args->arg2;
    off_t offset = (off_t)args->arg3;
    
    if (buf == nullptr) {
        return -EFAULT;
    }
    
    ssize_t result = pwrite(fd, buf, count, offset);
    return (result < 0) ? -errno : result;
}

[[nodiscard]] int64_t sys_truncate(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    off_t length = (off_t)args->arg1;
    
    if (path == nullptr) {
        return -EFAULT;
    }
    
    int result = truncate(path, length);
    return (result < 0) ? -errno : 0;
}

/* ===================================================================
 * Extended Directory Operations
 * =================================================================== */

[[nodiscard]] int64_t sys_link(const syscall_args_t *args) {
    const char *oldpath = (const char *)args->arg0;
    const char *newpath = (const char *)args->arg1;
    
    if (oldpath == nullptr || newpath == nullptr) {
        return -EFAULT;
    }
    
    int result = link(oldpath, newpath);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_symlink(const syscall_args_t *args) {
    const char *target = (const char *)args->arg0;
    const char *linkpath = (const char *)args->arg1;
    
    if (target == nullptr || linkpath == nullptr) {
        return -EFAULT;
    }
    
    int result = symlink(target, linkpath);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_readlink(const syscall_args_t *args) {
    const char *path = (const char *)args->arg0;
    char *buf = (char *)args->arg1;
    size_t bufsiz = (size_t)args->arg2;
    
    if (path == nullptr || buf == nullptr) {
        return -EFAULT;
    }
    
    ssize_t result = readlink(path, buf, bufsiz);
    return (result < 0) ? -errno : result;
}

[[nodiscard]] int64_t sys_rename(const syscall_args_t *args) {
    const char *oldpath = (const char *)args->arg0;
    const char *newpath = (const char *)args->arg1;
    
    if (oldpath == nullptr || newpath == nullptr) {
        return -EFAULT;
    }
    
    int result = rename(oldpath, newpath);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_getdents(const syscall_args_t *args) {
    printf("sys_getdents: Not yet implemented\n");
    return -ENOSYS;
}

/* ===================================================================
 * Extended Memory Management
 * =================================================================== */

[[nodiscard]] int64_t sys_msync(const syscall_args_t *args) {
    void *addr = (void *)args->arg0;
    size_t length = (size_t)args->arg1;
    int flags = (int)args->arg2;
    
    int result = msync(addr, length, flags);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_madvise(const syscall_args_t *args) {
    void *addr = (void *)args->arg0;
    size_t length = (size_t)args->arg1;
    int advice = (int)args->arg2;
    
    int result = madvise(addr, length, advice);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_mlock(const syscall_args_t *args) {
    const void *addr = (const void *)args->arg0;
    size_t len = (size_t)args->arg1;
    
    int result = mlock(addr, len);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_munlock(const syscall_args_t *args) {
    const void *addr = (const void *)args->arg0;
    size_t len = (size_t)args->arg1;
    
    int result = munlock(addr, len);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_mlockall(const syscall_args_t *args) {
    int flags = (int)args->arg0;
    
    int result = mlockall(flags);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_munlockall(const syscall_args_t *args) {
    (void)args;
    
    int result = munlockall();
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_sbrk(const syscall_args_t *args) {
    intptr_t increment = (intptr_t)args->arg0;
    
    void *result = sbrk(increment);
    return (result == (void *)-1) ? -errno : (int64_t)result;
}

[[nodiscard]] int64_t sys_mremap(const syscall_args_t *args) {
    printf("sys_mremap: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_mincore(const syscall_args_t *args) {
    printf("sys_mincore: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_mmap2(const syscall_args_t *args) {
    /* mmap2 is similar to mmap but with page-sized offset */
    return sys_mmap(args);
}

/* ===================================================================
 * Extended IPC Syscalls
 * =================================================================== */

[[nodiscard]] int64_t sys_bind(const syscall_args_t *args) {
    printf("sys_bind: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_listen(const syscall_args_t *args) {
    printf("sys_listen: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_accept(const syscall_args_t *args) {
    printf("sys_accept: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_connect(const syscall_args_t *args) {
    printf("sys_connect: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_send(const syscall_args_t *args) {
    printf("sys_send: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_recv(const syscall_args_t *args) {
    printf("sys_recv: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_sendto(const syscall_args_t *args) {
    printf("sys_sendto: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_recvfrom(const syscall_args_t *args) {
    printf("sys_recvfrom: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_shutdown(const syscall_args_t *args) {
    printf("sys_shutdown: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_setsockopt(const syscall_args_t *args) {
    printf("sys_setsockopt: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_getsockopt(const syscall_args_t *args) {
    printf("sys_getsockopt: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_shm_unlink(const syscall_args_t *args) {
    const char *name = (const char *)args->arg0;
    
    if (name == nullptr) {
        return -EFAULT;
    }
    
    int result = shm_unlink(name);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_msgget(const syscall_args_t *args) {
    printf("sys_msgget: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_msgsnd(const syscall_args_t *args) {
    printf("sys_msgsnd: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_msgrcv(const syscall_args_t *args) {
    printf("sys_msgrcv: Not yet implemented\n");
    return -ENOSYS;
}

/* ===================================================================
 * Extended Time Syscalls
 * =================================================================== */

[[nodiscard]] int64_t sys_time(const syscall_args_t *args) {
    time_t *tloc = (time_t *)args->arg0;
    
    time_t result = time(tloc);
    return (result == (time_t)-1) ? -errno : result;
}

[[nodiscard]] int64_t sys_settimeofday(const syscall_args_t *args) {
    const struct timeval *tv = (const struct timeval *)args->arg0;
    const struct timezone *tz = (const struct timezone *)args->arg1;
    
    int result = settimeofday(tv, tz);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_clock_settime(const syscall_args_t *args) {
    clockid_t clk_id = (clockid_t)args->arg0;
    const struct timespec *tp = (const struct timespec *)args->arg1;
    
    if (tp == nullptr) {
        return -EFAULT;
    }
    
    int result = clock_settime(clk_id, tp);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_clock_getres(const syscall_args_t *args) {
    clockid_t clk_id = (clockid_t)args->arg0;
    struct timespec *res = (struct timespec *)args->arg1;
    
    if (res == nullptr) {
        return -EFAULT;
    }
    
    int result = clock_getres(clk_id, res);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_clock_nanosleep(const syscall_args_t *args) {
    clockid_t clk_id = (clockid_t)args->arg0;
    int flags = (int)args->arg1;
    const struct timespec *request = (const struct timespec *)args->arg2;
    struct timespec *remain = (struct timespec *)args->arg3;
    
    if (request == nullptr) {
        return -EFAULT;
    }
    
    int result = clock_nanosleep(clk_id, flags, request, remain);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_timer_create(const syscall_args_t *args) {
    printf("sys_timer_create: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_timer_delete(const syscall_args_t *args) {
    printf("sys_timer_delete: Not yet implemented\n");
    return -ENOSYS;
}

/* ===================================================================
 * System Information Syscalls
 * =================================================================== */

[[nodiscard]] int64_t sys_uname(const syscall_args_t *args) {
    struct utsname *buf = (struct utsname *)args->arg0;
    
    if (buf == nullptr) {
        return -EFAULT;
    }
    
    int result = uname(buf);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_sysinfo(const syscall_args_t *args) {
    struct sysinfo *info = (struct sysinfo *)args->arg0;
    
    if (info == nullptr) {
        return -EFAULT;
    }
    
    int result = sysinfo(info);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_getrusage(const syscall_args_t *args) {
    int who = (int)args->arg0;
    struct rusage *usage = (struct rusage *)args->arg1;
    
    if (usage == nullptr) {
        return -EFAULT;
    }
    
    int result = getrusage(who, usage);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_times(const syscall_args_t *args) {
    printf("sys_times: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_syslog(const syscall_args_t *args) {
    printf("sys_syslog: Not yet implemented\n");
    return -ENOSYS;
}

[[nodiscard]] int64_t sys_getrlimit(const syscall_args_t *args) {
    int resource = (int)args->arg0;
    struct rlimit *rlim = (struct rlimit *)args->arg1;
    
    if (rlim == nullptr) {
        return -EFAULT;
    }
    
    int result = getrlimit(resource, rlim);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_setrlimit(const syscall_args_t *args) {
    int resource = (int)args->arg0;
    const struct rlimit *rlim = (const struct rlimit *)args->arg1;
    
    if (rlim == nullptr) {
        return -EFAULT;
    }
    
    int result = setrlimit(resource, rlim);
    return (result < 0) ? -errno : 0;
}

[[nodiscard]] int64_t sys_getpagesize(const syscall_args_t *args) {
    (void)args;
    return (int64_t)getpagesize();
}
