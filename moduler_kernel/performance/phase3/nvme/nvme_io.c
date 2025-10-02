/**
 * @file nvme_io.c
 * @brief NVMe I/O operations implementation
 */

#include "nvme_io.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// I/O completion callback wrapper
static void nvme_io_completion_handler(void* ctx, const nvme_completion_t* cpl) {
    nvme_io_request_t* req = (nvme_io_request_t*)ctx;
    
    // Extract status
    uint16_t status = cpl->status >> 1;
    req->status = (status == 0) ? 0 : -EIO;
    req->completed = true;
    
    // Call user callback if provided
    if (req->callback) {
        req->callback(req->user_ctx, req->status);
    }
}

nvme_io_request_t* nvme_read_async(nvme_qpair_t* qpair, uint32_t nsid,
                                   uint64_t lba, uint32_t lba_count,
                                   void* buffer, size_t buffer_size,
                                   void (*callback)(void* ctx, int status),
                                   void* ctx) {
    if (!qpair || !buffer || buffer_size == 0) {
        return NULL;
    }
    
    // Allocate request structure
    nvme_io_request_t* req = calloc(1, sizeof(nvme_io_request_t));
    if (!req) {
        return NULL;
    }
    
    req->qpair = qpair;
    req->nsid = nsid;
    req->lba = lba;
    req->lba_count = lba_count;
    req->buffer = buffer;
    req->buffer_size = buffer_size;
    req->is_write = false;
    req->callback = callback;
    req->user_ctx = ctx;
    
    // Build NVMe read command
    nvme_command_t cmd = {0};
    cmd.cdw0 = NVME_CMD_READ; // Opcode
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buffer; // Physical address (simplified)
    cmd.cdw10 = (uint32_t)lba;
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = (lba_count - 1) & 0xFFFF; // 0-based value
    
    // Submit command
    int cid = nvme_submit_command(qpair, &cmd, req);
    if (cid < 0) {
        free(req);
        return NULL;
    }
    
    req->cid = cid;
    return req;
}

nvme_io_request_t* nvme_write_async(nvme_qpair_t* qpair, uint32_t nsid,
                                    uint64_t lba, uint32_t lba_count,
                                    const void* buffer, size_t buffer_size,
                                    void (*callback)(void* ctx, int status),
                                    void* ctx) {
    if (!qpair || !buffer || buffer_size == 0) {
        return NULL;
    }
    
    // Allocate request structure
    nvme_io_request_t* req = calloc(1, sizeof(nvme_io_request_t));
    if (!req) {
        return NULL;
    }
    
    req->qpair = qpair;
    req->nsid = nsid;
    req->lba = lba;
    req->lba_count = lba_count;
    req->buffer = (void*)buffer;
    req->buffer_size = buffer_size;
    req->is_write = true;
    req->callback = callback;
    req->user_ctx = ctx;
    
    // Build NVMe write command
    nvme_command_t cmd = {0};
    cmd.cdw0 = NVME_CMD_WRITE; // Opcode
    cmd.nsid = nsid;
    cmd.prp1 = (uint64_t)buffer;
    cmd.cdw10 = (uint32_t)lba;
    cmd.cdw11 = (uint32_t)(lba >> 32);
    cmd.cdw12 = (lba_count - 1) & 0xFFFF;
    
    // Submit command
    int cid = nvme_submit_command(qpair, &cmd, req);
    if (cid < 0) {
        free(req);
        return NULL;
    }
    
    req->cid = cid;
    return req;
}

nvme_io_request_t* nvme_flush_async(nvme_qpair_t* qpair, uint32_t nsid,
                                    void (*callback)(void* ctx, int status),
                                    void* ctx) {
    if (!qpair) {
        return NULL;
    }
    
    // Allocate request structure
    nvme_io_request_t* req = calloc(1, sizeof(nvme_io_request_t));
    if (!req) {
        return NULL;
    }
    
    req->qpair = qpair;
    req->nsid = nsid;
    req->callback = callback;
    req->user_ctx = ctx;
    
    // Build NVMe flush command
    nvme_command_t cmd = {0};
    cmd.cdw0 = NVME_CMD_FLUSH;
    cmd.nsid = nsid;
    
    // Submit command
    int cid = nvme_submit_command(qpair, &cmd, req);
    if (cid < 0) {
        free(req);
        return NULL;
    }
    
    req->cid = cid;
    return req;
}

int nvme_io_wait(nvme_io_request_t* req) {
    if (!req) {
        return -EINVAL;
    }
    
    // Poll for completion
    while (!req->completed) {
        nvme_process_completions(req->qpair, 0, nvme_io_completion_handler);
    }
    
    return req->status;
}

bool nvme_io_is_complete(const nvme_io_request_t* req) {
    return req ? req->completed : false;
}

void nvme_io_free(nvme_io_request_t* req) {
    free(req);
}

int nvme_read(nvme_qpair_t* qpair, uint32_t nsid, uint64_t lba,
              uint32_t lba_count, void* buffer, size_t buffer_size) {
    nvme_io_request_t* req = nvme_read_async(qpair, nsid, lba, lba_count,
                                             buffer, buffer_size, NULL, NULL);
    if (!req) {
        return -ENOMEM;
    }
    
    int status = nvme_io_wait(req);
    nvme_io_free(req);
    return status;
}

int nvme_write(nvme_qpair_t* qpair, uint32_t nsid, uint64_t lba,
               uint32_t lba_count, const void* buffer, size_t buffer_size) {
    nvme_io_request_t* req = nvme_write_async(qpair, nsid, lba, lba_count,
                                              buffer, buffer_size, NULL, NULL);
    if (!req) {
        return -ENOMEM;
    }
    
    int status = nvme_io_wait(req);
    nvme_io_free(req);
    return status;
}

int nvme_flush(nvme_qpair_t* qpair, uint32_t nsid) {
    nvme_io_request_t* req = nvme_flush_async(qpair, nsid, NULL, NULL);
    if (!req) {
        return -ENOMEM;
    }
    
    int status = nvme_io_wait(req);
    nvme_io_free(req);
    return status;
}
