
#ifndef BDI_IPC_SEM_H
#define BDI_IPC_SEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

/* Semaphore limits */
#define SEM_VALUE_MAX   32767
#define SEMMNI          128     /* Max number of semaphore sets */
#define SEMMSL          250     /* Max semaphores per set */
#define SEMMNS          (SEMMNI * SEMMSL)  /* Max semaphores system-wide */
#define SEMOPM          32      /* Max operations per semop call */

/* Semaphore flags */
#define SEM_UNDO        0x1000
#define GETVAL          12
#define SETVAL          16
#define GETPID          11
#define GETNCNT         14
#define GETZCNT         15
#define GETALL          13
#define SETALL          17

/* POSIX semaphore */
typedef struct sem {
    char *name;
    uint32_t value;
    uint32_t max_value;
    uint32_t ref_count;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    void *wait_queue;
    void *lock;
    bool is_named;
    struct sem *next;
} sem_t;

/* System V semaphore */
typedef struct sembuf {
    uint16_t sem_num;
    int16_t sem_op;
    int16_t sem_flg;
} sembuf_t;

typedef struct semid_ds {
    ipc_perm_t sem_perm;
    time_t sem_otime;
    time_t sem_ctime;
    uint32_t sem_nsems;
} semid_ds_t;

typedef struct sem_element {
    uint32_t value;
    pid_t last_pid;
    uint32_t ncnt;  /* Number waiting for increase */
    uint32_t zcnt;  /* Number waiting for zero */
} sem_element_t;

typedef struct sem_array {
    uint32_t id;
    ipc_perm_t perm;
    uint32_t nsems;
    sem_element_t *sems;
    time_t otime;
    time_t ctime;
    void *wait_queue;
    void *lock;
    struct sem_array *next;
} sem_array_t;

/* POSIX Semaphore API */
sem_t *sem_open(const char *name, int oflag, mode_t mode, uint32_t value);
int sem_close(sem_t *sem);
int sem_unlink(const char *name);
int sem_init(sem_t *sem, int pshared, uint32_t value);
int sem_destroy(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_timedwait(sem_t *sem, const struct timespec *abs_timeout);
int sem_post(sem_t *sem);
int sem_getvalue(sem_t *sem, int *sval);

/* System V Semaphore API */
int semget(key_t key, int nsems, int semflg);
int semop(int semid, struct sembuf *sops, size_t nsops);
int semctl(int semid, int semnum, int cmd, ...);

/* Internal functions */
sem_t *sem_alloc(const char *name, uint32_t value, mode_t mode);
void sem_free(sem_t *sem);
sem_t *sem_find(const char *name);
int sem_wait_internal(sem_t *sem, bool nonblock, const struct timespec *timeout);
int sem_post_internal(sem_t *sem);

sem_array_t *sem_array_alloc(key_t key, int nsems, int flags);
void sem_array_free(sem_array_t *arr);
sem_array_t *sem_array_find(int semid);
sem_array_t *sem_array_find_by_key(key_t key);
int sem_array_op(sem_array_t *arr, struct sembuf *sops, size_t nsops, bool nonblock);

/* Initialization */
int sem_init_subsystem(void);

#endif /* BDI_IPC_SEM_H */
