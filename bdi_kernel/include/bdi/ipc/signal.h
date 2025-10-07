
#ifndef BDI_IPC_SIGNAL_H
#define BDI_IPC_SIGNAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <signal.h>

/* Signal numbers (standard POSIX) */
#define SIGHUP      1
#define SIGINT      2
#define SIGQUIT     3
#define SIGILL      4
#define SIGTRAP     5
#define SIGABRT     6
#define SIGBUS      7
#define SIGFPE      8
#define SIGKILL     9
#define SIGUSR1     10
#define SIGSEGV     11
#define SIGUSR2     12
#define SIGPIPE     13
#define SIGALRM     14
#define SIGTERM     15
#define SIGSTKFLT   16
#define SIGCHLD     17
#define SIGCONT     18
#define SIGSTOP     19
#define SIGTSTP     20
#define SIGTTIN     21
#define SIGTTOU     22
#define SIGURG      23
#define SIGXCPU     24
#define SIGXFSZ     25
#define SIGVTALRM   26
#define SIGPROF     27
#define SIGWINCH    28
#define SIGIO       29
#define SIGPWR      30
#define SIGSYS      31

/* Real-time signals */
#define SIGRTMIN    32
#define SIGRTMAX    64
#define _NSIG       65

/* Signal actions */
#define SIG_DFL     ((void (*)(int))0)
#define SIG_IGN     ((void (*)(int))1)
#define SIG_ERR     ((void (*)(int))-1)

/* sigaction flags */
#define SA_NOCLDSTOP    0x00000001
#define SA_NOCLDWAIT    0x00000002
#define SA_SIGINFO      0x00000004
#define SA_ONSTACK      0x08000000
#define SA_RESTART      0x10000000
#define SA_NODEFER      0x40000000
#define SA_RESETHAND    0x80000000

/* Signal set operations */
#define SIG_BLOCK       0
#define SIG_UNBLOCK     1
#define SIG_SETMASK     2

/* Signal information */
typedef struct siginfo {
    int si_signo;
    int si_errno;
    int si_code;
    pid_t si_pid;
    uid_t si_uid;
    void *si_addr;
    int si_status;
    long si_band;
    union {
        int sival_int;
        void *sival_ptr;
    } si_value;
} siginfo_t;

/* Signal action */
typedef struct sigaction {
    union {
        void (*sa_handler)(int);
        void (*sa_sigaction)(int, siginfo_t *, void *);
    };
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
} sigaction_t;

/* Signal queue entry */
typedef struct sigqueue_entry {
    int signo;
    siginfo_t info;
    struct sigqueue_entry *next;
} sigqueue_entry_t;

/* Per-process signal state */
typedef struct signal_state {
    sigset_t pending;
    sigset_t blocked;
    sigaction_t actions[_NSIG];
    sigqueue_entry_t *queue_head;
    sigqueue_entry_t *queue_tail;
    uint32_t queue_count;
    void *lock;
} signal_state_t;

/* Signal API */
int signal_init(void);
int signal_send(pid_t pid, int signo);
int signal_send_info(pid_t pid, int signo, const siginfo_t *info);
int signal_queue(pid_t pid, int signo, const union sigval value);
int signal_kill(pid_t pid, int signo);
int signal_killpg(pid_t pgrp, int signo);

/* Signal handling */
int signal_action(int signo, const struct sigaction *act, struct sigaction *oldact);
int signal_procmask(int how, const sigset_t *set, sigset_t *oldset);
int signal_pending(sigset_t *set);
int signal_suspend(const sigset_t *mask);
int signal_wait(const sigset_t *set, siginfo_t *info);
int signal_timedwait(const sigset_t *set, siginfo_t *info, const struct timespec *timeout);

/* Signal set operations */
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signo);
int sigdelset(sigset_t *set, int signo);
int sigismember(const sigset_t *set, int signo);

/* Internal functions */
signal_state_t *signal_state_alloc(void);
void signal_state_free(signal_state_t *state);
int signal_deliver(pid_t pid, int signo, const siginfo_t *info);
int signal_check_pending(signal_state_t *state);
int signal_handle(int signo, siginfo_t *info);
bool signal_is_ignored(signal_state_t *state, int signo);
bool signal_is_blocked(signal_state_t *state, int signo);

/* Signal queue management */
int signal_queue_add(signal_state_t *state, int signo, const siginfo_t *info);
int signal_queue_remove(signal_state_t *state, int signo, siginfo_t *info);
void signal_queue_clear(signal_state_t *state);

#endif /* BDI_IPC_SIGNAL_H */
