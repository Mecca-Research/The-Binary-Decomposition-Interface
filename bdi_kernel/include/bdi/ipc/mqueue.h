
#ifndef BDI_IPC_MQUEUE_H
#define BDI_IPC_MQUEUE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <time.h>

/* Message queue limits */
#define MQ_PRIO_MAX     32768
#define MQ_MAX_MSG      10
#define MQ_MAX_MSGSIZE  8192

/* Message queue flags */
#define MQ_FLAG_NONBLOCK    (1 << 0)

/* Message queue attributes */
typedef struct mq_attr {
    long mq_flags;
    long mq_maxmsg;
    long mq_msgsize;
    long mq_curmsgs;
} mq_attr_t;

/* Message structure */
typedef struct mq_message {
    void *data;
    size_t size;
    uint32_t priority;
    struct mq_message *next;
} mq_message_t;

/* POSIX message queue */
typedef struct mqueue {
    char *name;
    mq_attr_t attr;
    mq_message_t *head;
    mq_message_t *tail;
    uint32_t msg_count;
    uint32_t ref_count;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    void *read_wait;
    void *write_wait;
    void *lock;
    struct mqueue *next;
} mqueue_t;

/* System V message queue */
typedef struct msgbuf {
    long mtype;
    char mtext[1];
} msgbuf_t;

typedef struct msqid_ds {
    ipc_perm_t msg_perm;
    time_t msg_stime;
    time_t msg_rtime;
    time_t msg_ctime;
    uint32_t msg_qnum;
    uint32_t msg_qbytes;
    pid_t msg_lspid;
    pid_t msg_lrpid;
} msqid_ds_t;

typedef struct msg_queue {
    uint32_t id;
    ipc_perm_t perm;
    mq_message_t *head;
    mq_message_t *tail;
    uint32_t msg_count;
    uint32_t max_bytes;
    uint32_t current_bytes;
    time_t send_time;
    time_t recv_time;
    time_t change_time;
    pid_t last_send_pid;
    pid_t last_recv_pid;
    void *read_wait;
    void *write_wait;
    void *lock;
    struct msg_queue *next;
} msg_queue_t;

/* POSIX Message Queue API */
int mq_open(const char *name, int oflag, mode_t mode, struct mq_attr *attr);
int mq_close(int mqdes);
int mq_unlink(const char *name);
int mq_send(int mqdes, const char *msg_ptr, size_t msg_len, uint32_t msg_prio);
ssize_t mq_receive(int mqdes, char *msg_ptr, size_t msg_len, uint32_t *msg_prio);
int mq_getattr(int mqdes, struct mq_attr *attr);
int mq_setattr(int mqdes, const struct mq_attr *newattr, struct mq_attr *oldattr);

/* System V Message Queue API */
int msgget(key_t key, int msgflg);
int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg);
int msgctl(int msqid, int cmd, struct msqid_ds *buf);

/* Internal functions */
mqueue_t *mqueue_alloc(const char *name, const mq_attr_t *attr, mode_t mode);
void mqueue_free(mqueue_t *mq);
mqueue_t *mqueue_find(const char *name);
int mqueue_send_internal(mqueue_t *mq, const void *msg, size_t len, uint32_t prio, bool nonblock);
ssize_t mqueue_receive_internal(mqueue_t *mq, void *msg, size_t len, uint32_t *prio, bool nonblock);

msg_queue_t *msg_queue_alloc(key_t key, int flags);
void msg_queue_free(msg_queue_t *mq);
msg_queue_t *msg_queue_find(int msqid);
msg_queue_t *msg_queue_find_by_key(key_t key);

/* Initialization */
int mqueue_init(void);

#endif /* BDI_IPC_MQUEUE_H */
