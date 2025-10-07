
#ifndef BDI_IPC_PIPE_H
#define BDI_IPC_PIPE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Pipe buffer size */
#define PIPE_BUF_SIZE   65536  /* 64KB */
#define PIPE_MAX_SIZE   1048576  /* 1MB max */

/* Pipe flags */
#define PIPE_FLAG_NONBLOCK  (1 << 0)
#define PIPE_FLAG_CLOEXEC   (1 << 1)

/* Pipe structure */
typedef struct pipe {
    uint8_t *buffer;
    size_t size;
    size_t head;
    size_t tail;
    size_t count;
    
    /* Reference counts */
    uint32_t readers;
    uint32_t writers;
    
    /* Synchronization */
    void *read_wait;
    void *write_wait;
    void *lock;
    
    /* Flags */
    uint32_t flags;
    
    /* Statistics */
    uint64_t bytes_read;
    uint64_t bytes_written;
} pipe_t;

/* Pipe file descriptor structure */
typedef struct pipe_fd {
    pipe_t *pipe;
    bool is_read_end;
    uint32_t flags;
} pipe_fd_t;

/* Pipe API */
int pipe_create(int fds[2]);
int pipe_create_flags(int fds[2], uint32_t flags);
int pipe_close(int fd);
ssize_t pipe_read(int fd, void *buf, size_t count);
ssize_t pipe_write(int fd, const void *buf, size_t count);
int pipe_set_size(int fd, size_t size);
size_t pipe_get_size(int fd);

/* Named pipes (FIFOs) */
int fifo_create(const char *path, uint32_t mode);
int fifo_open(const char *path, uint32_t flags);
int fifo_unlink(const char *path);

/* Internal functions */
pipe_t *pipe_alloc(size_t size);
void pipe_free(pipe_t *pipe);
ssize_t pipe_read_internal(pipe_t *pipe, void *buf, size_t count, bool nonblock);
ssize_t pipe_write_internal(pipe_t *pipe, const void *buf, size_t count, bool nonblock);
size_t pipe_available(pipe_t *pipe);
size_t pipe_space(pipe_t *pipe);

#endif /* BDI_IPC_PIPE_H */
