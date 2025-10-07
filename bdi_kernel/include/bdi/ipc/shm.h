
#ifndef BDI_IPC_SHM_H
#define BDI_IPC_SHM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

/* Shared memory flags */
#define SHM_RDONLY      0x1000
#define SHM_RND         0x2000
#define SHM_REMAP       0x4000
#define SHM_EXEC        0x8000

/* Shared memory control commands */
#define IPC_RMID        0
#define IPC_SET         1
#define IPC_STAT        2
#define IPC_INFO        3

/* Permission flags */
#define IPC_CREAT       0x0200
#define IPC_EXCL        0x0400
#define IPC_NOWAIT      0x0800

/* IPC key */
typedef int32_t key_t;
#define IPC_PRIVATE     ((key_t)0)

/* IPC permissions */
typedef struct ipc_perm {
    key_t key;
    uid_t uid;
    gid_t gid;
    uid_t cuid;
    gid_t cgid;
    uint16_t mode;
    uint16_t seq;
} ipc_perm_t;

/* Shared memory segment structure */
typedef struct shm_segment {
    uint32_t id;
    ipc_perm_t perm;
    size_t size;
    void *addr;
    uint32_t attach_count;
    pid_t creator_pid;
    pid_t last_attach_pid;
    pid_t last_detach_pid;
    time_t create_time;
    time_t attach_time;
    time_t detach_time;
    struct shm_segment *next;
} shm_segment_t;

/* Shared memory statistics */
typedef struct shmid_ds {
    ipc_perm_t shm_perm;
    size_t shm_segsz;
    time_t shm_atime;
    time_t shm_dtime;
    time_t shm_ctime;
    pid_t shm_cpid;
    pid_t shm_lpid;
    uint32_t shm_nattch;
} shmid_ds_t;

/* POSIX shared memory object */
typedef struct shm_object {
    char *name;
    size_t size;
    void *addr;
    uint32_t ref_count;
    uint32_t flags;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    time_t create_time;
    time_t modify_time;
    struct shm_object *next;
} shm_object_t;

/* System V Shared Memory API */
int shmget(key_t key, size_t size, int shmflg);
void *shmat(int shmid, const void *shmaddr, int shmflg);
int shmdt(const void *shmaddr);
int shmctl(int shmid, int cmd, struct shmid_ds *buf);

/* POSIX Shared Memory API */
int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);

/* Internal functions */
shm_segment_t *shm_segment_alloc(key_t key, size_t size, int flags);
void shm_segment_free(shm_segment_t *seg);
shm_segment_t *shm_segment_find(int shmid);
shm_segment_t *shm_segment_find_by_key(key_t key);
int shm_segment_attach(shm_segment_t *seg, void *addr, int flags);
int shm_segment_detach(shm_segment_t *seg, void *addr);

shm_object_t *shm_object_alloc(const char *name, size_t size, mode_t mode);
void shm_object_free(shm_object_t *obj);
shm_object_t *shm_object_find(const char *name);

/* Initialization */
int shm_init(void);

#endif /* BDI_IPC_SHM_H */
