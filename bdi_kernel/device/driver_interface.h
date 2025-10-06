
/**
 * @file driver_interface.h
 * @brief Standard Driver Interface
 * 
 * Defines standard file operations and driver registration API
 * for device drivers in the BDI kernel.
 */

#ifndef BDI_DRIVER_INTERFACE_H
#define BDI_DRIVER_INTERFACE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "device_manager.h"

/* File operation flags */
#define O_RDONLY        0x0000
#define O_WRONLY        0x0001
#define O_RDWR          0x0002
#define O_NONBLOCK      0x0004
#define O_APPEND        0x0008
#define O_CREAT         0x0010
#define O_TRUNC         0x0020
#define O_EXCL          0x0040
#define O_SYNC          0x0080

/* Seek whence values */
#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

/* ioctl commands */
#define IOCTL_GET_INFO      0x1000
#define IOCTL_SET_CONFIG    0x1001
#define IOCTL_RESET         0x1002
#define IOCTL_GET_STATS     0x1003

/* mmap protection flags */
#define PROT_NONE       0x0
#define PROT_READ       0x1
#define PROT_WRITE      0x2
#define PROT_EXEC       0x4

/* mmap flags */
#define MAP_SHARED      0x01
#define MAP_PRIVATE     0x02
#define MAP_FIXED       0x10

/* Forward declarations */
struct file;
struct iovec;

/**
 * @brief File operations structure
 * 
 * Standard file operations that drivers must implement
 */
struct file_operations {
    /* Open device */
    int (*open)(struct device *dev, struct file *file, uint32_t flags);
    
    /* Close device */
    int (*close)(struct device *dev, struct file *file);
    
    /* Read from device */
    ssize_t (*read)(struct device *dev, struct file *file, void *buf, 
                   size_t count, uint64_t offset);
    
    /* Write to device */
    ssize_t (*write)(struct device *dev, struct file *file, const void *buf,
                    size_t count, uint64_t offset);
    
    /* Vectored read */
    ssize_t (*readv)(struct device *dev, struct file *file, 
                    const struct iovec *iov, uint32_t iovcnt, uint64_t offset);
    
    /* Vectored write */
    ssize_t (*writev)(struct device *dev, struct file *file,
                     const struct iovec *iov, uint32_t iovcnt, uint64_t offset);
    
    /* Seek */
    int64_t (*seek)(struct device *dev, struct file *file, int64_t offset, int whence);
    
    /* ioctl */
    int (*ioctl)(struct device *dev, struct file *file, uint32_t cmd, void *arg);
    
    /* mmap */
    void *(*mmap)(struct device *dev, struct file *file, void *addr, size_t length,
                 uint32_t prot, uint32_t flags, uint64_t offset);
    
    /* munmap */
    int (*munmap)(struct device *dev, struct file *file, void *addr, size_t length);
    
    /* Poll/select */
    int (*poll)(struct device *dev, struct file *file, uint32_t events);
    
    /* Flush */
    int (*flush)(struct device *dev, struct file *file);
    
    /* Sync */
    int (*sync)(struct device *dev, struct file *file);
};

/**
 * @brief File descriptor structure
 */
struct file {
    struct device *device;
    const struct file_operations *ops;
    uint32_t flags;
    uint64_t offset;
    void *private_data;
    _Atomic uint32_t refcount;
};

/**
 * @brief I/O vector for vectored I/O
 */
struct iovec {
    void *iov_base;
    size_t iov_len;
};

/**
 * @brief DMA buffer descriptor
 */
struct dma_buffer {
    void *virt_addr;            /* Virtual address */
    uint64_t phys_addr;         /* Physical address */
    size_t size;                /* Buffer size */
    uint32_t flags;
    struct device *device;
};

/* DMA buffer flags */
#define DMA_BIDIRECTIONAL   0x0
#define DMA_TO_DEVICE       0x1
#define DMA_FROM_DEVICE     0x2
#define DMA_COHERENT        0x4

/**
 * @brief Allocate a DMA buffer
 * 
 * @param dev Device
 * @param size Buffer size
 * @param flags DMA flags
 * @return DMA buffer or nullptr on failure
 */
struct dma_buffer *dma_alloc_buffer(struct device *dev, size_t size, uint32_t flags);

/**
 * @brief Free a DMA buffer
 * 
 * @param buf DMA buffer
 */
void dma_free_buffer(struct dma_buffer *buf);

/**
 * @brief Map a DMA buffer for device access
 * 
 * @param buf DMA buffer
 * @param direction DMA direction
 * @return 0 on success, negative error code on failure
 */
int dma_map_buffer(struct dma_buffer *buf, uint32_t direction);

/**
 * @brief Unmap a DMA buffer
 * 
 * @param buf DMA buffer
 */
void dma_unmap_buffer(struct dma_buffer *buf);

/**
 * @brief Sync DMA buffer for CPU access
 * 
 * @param buf DMA buffer
 */
void dma_sync_for_cpu(struct dma_buffer *buf);

/**
 * @brief Sync DMA buffer for device access
 * 
 * @param buf DMA buffer
 */
void dma_sync_for_device(struct dma_buffer *buf);

/**
 * @brief Open a device file
 * 
 * @param path Device path
 * @param flags Open flags
 * @return File descriptor or nullptr on failure
 */
struct file *device_open(const char *path, uint32_t flags);

/**
 * @brief Close a device file
 * 
 * @param file File descriptor
 * @return 0 on success, negative error code on failure
 */
int device_close(struct file *file);

/**
 * @brief Read from a device
 * 
 * @param file File descriptor
 * @param buf Buffer
 * @param count Number of bytes to read
 * @return Number of bytes read or negative error code
 */
ssize_t device_read(struct file *file, void *buf, size_t count);

/**
 * @brief Write to a device
 * 
 * @param file File descriptor
 * @param buf Buffer
 * @param count Number of bytes to write
 * @return Number of bytes written or negative error code
 */
ssize_t device_write(struct file *file, const void *buf, size_t count);

/**
 * @brief Seek in a device
 * 
 * @param file File descriptor
 * @param offset Offset
 * @param whence Seek origin
 * @return New offset or negative error code
 */
int64_t device_seek(struct file *file, int64_t offset, int whence);

/**
 * @brief ioctl on a device
 * 
 * @param file File descriptor
 * @param cmd ioctl command
 * @param arg ioctl argument
 * @return 0 on success, negative error code on failure
 */
int device_ioctl(struct file *file, uint32_t cmd, void *arg);

/**
 * @brief mmap a device
 * 
 * @param file File descriptor
 * @param addr Desired address
 * @param length Length
 * @param prot Protection flags
 * @param flags mmap flags
 * @param offset Offset in device
 * @return Mapped address or nullptr on failure
 */
void *device_mmap(struct file *file, void *addr, size_t length,
                 uint32_t prot, uint32_t flags, uint64_t offset);

/**
 * @brief munmap a device mapping
 * 
 * @param file File descriptor
 * @param addr Address
 * @param length Length
 * @return 0 on success, negative error code on failure
 */
int device_munmap(struct file *file, void *addr, size_t length);

#endif /* BDI_DRIVER_INTERFACE_H */
