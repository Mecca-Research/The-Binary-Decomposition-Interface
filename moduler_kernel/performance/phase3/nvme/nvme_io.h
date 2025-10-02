
/**
 * @file nvme_io.h
 * @brief NVMe I/O operations (read/write with zero-copy)
 * 
 * High-level I/O interface with fiber integration for async operations.
 */

#ifndef PHASE3_NVME_IO_H
#define PHASE3_NVME_IO_H

#include "nvme_queue.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// I/O request structure
typedef struct nvme_io_request {
    nvme_qpair_t* qpair;
    uint32_t nsid;          // Namespace ID
    uint64_t lba;           // Starting LBA
    uint32_t lba_count;     // Number of LBAs
    void* buffer;           // Data buffer
    size_t buffer_size;     // Buffer size in bytes
    bool is_write;          // true for write, false for read
    
    // Completion info
    void* user_ctx;         // User context
    void (*callback)(void* ctx, int status);
    
    // Internal state
    uint16_t cid;           // Command ID
    int status;             // Completion status
    bool completed;         // Completion flag
} nvme_io_request_t;

/**
 * @brief Submit read I/O request
 * 
 * @param qpair Queue pair handle
 * @param nsid Namespace ID
 * @param lba Starting LBA
 * @param lba_count Number of LBAs to read
 * @param buffer Output buffer
 * @param buffer_size Buffer size in bytes
 * @param callback Completion callback (can be NULL)
 * @param ctx User context
 * @return Request handle on success, NULL on failure
 */
nvme_io_request_t* nvme_read_async(nvme_qpair_t* qpair, uint32_t nsid,
                                   uint64_t lba, uint32_t lba_count,
                                   void* buffer, size_t buffer_size,
                                   void (*callback)(void* ctx, int status),
                                   void* ctx);

/**
 * @brief Submit write I/O request
 * 
 * @param qpair Queue pair handle
 * @param nsid Namespace ID
 * @param lba Starting LBA
 * @param lba_count Number of LBAs to write
 * @param buffer Input buffer
 * @param buffer_size Buffer size in bytes
 * @param callback Completion callback (can be NULL)
 * @param ctx User context
 * @return Request handle on success, NULL on failure
 */
nvme_io_request_t* nvme_write_async(nvme_qpair_t* qpair, uint32_t nsid,
                                    uint64_t lba, uint32_t lba_count,
                                    const void* buffer, size_t buffer_size,
                                    void (*callback)(void* ctx, int status),
                                    void* ctx);

/**
 * @brief Submit flush request
 * 
 * @param qpair Queue pair handle
 * @param nsid Namespace ID
 * @param callback Completion callback (can be NULL)
 * @param ctx User context
 * @return Request handle on success, NULL on failure
 */
nvme_io_request_t* nvme_flush_async(nvme_qpair_t* qpair, uint32_t nsid,
                                    void (*callback)(void* ctx, int status),
                                    void* ctx);

/**
 * @brief Wait for I/O request completion (blocking)
 * 
 * @param req Request handle
 * @return 0 on success, negative error code on failure
 */
int nvme_io_wait(nvme_io_request_t* req);

/**
 * @brief Check if I/O request is complete
 * 
 * @param req Request handle
 * @return true if complete, false otherwise
 */
bool nvme_io_is_complete(const nvme_io_request_t* req);

/**
 * @brief Free I/O request
 * 
 * @param req Request handle
 */
void nvme_io_free(nvme_io_request_t* req);

/**
 * @brief Synchronous read (blocking)
 * 
 * @param qpair Queue pair handle
 * @param nsid Namespace ID
 * @param lba Starting LBA
 * @param lba_count Number of LBAs to read
 * @param buffer Output buffer
 * @param buffer_size Buffer size in bytes
 * @return 0 on success, negative error code on failure
 */
int nvme_read(nvme_qpair_t* qpair, uint32_t nsid, uint64_t lba,
              uint32_t lba_count, void* buffer, size_t buffer_size);

/**
 * @brief Synchronous write (blocking)
 * 
 * @param qpair Queue pair handle
 * @param nsid Namespace ID
 * @param lba Starting LBA
 * @param lba_count Number of LBAs to write
 * @param buffer Input buffer
 * @param buffer_size Buffer size in bytes
 * @return 0 on success, negative error code on failure
 */
int nvme_write(nvme_qpair_t* qpair, uint32_t nsid, uint64_t lba,
               uint32_t lba_count, const void* buffer, size_t buffer_size);

/**
 * @brief Synchronous flush (blocking)
 * 
 * @param qpair Queue pair handle
 * @param nsid Namespace ID
 * @return 0 on success, negative error code on failure
 */
int nvme_flush(nvme_qpair_t* qpair, uint32_t nsid);

#ifdef __cplusplus
}
#endif

#endif // PHASE3_NVME_IO_H
