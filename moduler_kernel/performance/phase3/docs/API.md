# Phase 3 API Reference

## NVMe API

### Device Management

```c
// Initialize NVMe subsystem
int nvme_init(void);

// Shutdown NVMe subsystem
void nvme_shutdown(void);

// Probe and enumerate devices
int nvme_probe_devices(nvme_device_t** devices, size_t max_devices);

// Attach to device
nvme_device_t* nvme_attach_device(uint16_t domain, uint8_t bus,
                                   uint8_t device, uint8_t function);

// Detach from device
void nvme_detach_device(nvme_device_t* dev);
```

### Queue Management

```c
// Create admin queue pair
nvme_qpair_t* nvme_create_admin_qpair(nvme_device_t* dev,
                                      uint16_t sq_size, uint16_t cq_size);

// Create I/O queue pair
nvme_qpair_t* nvme_create_io_qpair(nvme_device_t* dev, uint16_t qid,
                                   uint16_t sq_size, uint16_t cq_size);

// Destroy queue pair
void nvme_destroy_qpair(nvme_qpair_t* qpair);

// Submit command
int nvme_submit_command(nvme_qpair_t* qpair, const nvme_command_t* cmd,
                       void* ctx);

// Process completions (polling)
int nvme_process_completions(nvme_qpair_t* qpair, uint32_t max_completions,
                             nvme_io_completion_cb cb);
```

### I/O Operations

```c
// Async read
nvme_io_request_t* nvme_read_async(nvme_qpair_t* qpair, uint32_t nsid,
                                   uint64_t lba, uint32_t lba_count,
                                   void* buffer, size_t buffer_size,
                                   void (*callback)(void* ctx, int status),
                                   void* ctx);

// Async write
nvme_io_request_t* nvme_write_async(nvme_qpair_t* qpair, uint32_t nsid,
                                    uint64_t lba, uint32_t lba_count,
                                    const void* buffer, size_t buffer_size,
                                    void (*callback)(void* ctx, int status),
                                    void* ctx);

// Sync read
int nvme_read(nvme_qpair_t* qpair, uint32_t nsid, uint64_t lba,
              uint32_t lba_count, void* buffer, size_t buffer_size);

// Sync write
int nvme_write(nvme_qpair_t* qpair, uint32_t nsid, uint64_t lba,
               uint32_t lba_count, const void* buffer, size_t buffer_size);
```

### Example Usage

```c
// Initialize and attach to device
nvme_init();
nvme_device_t* dev = nvme_attach_device(0, 0, 4, 0);

// Create I/O queue pair
nvme_qpair_t* qpair = nvme_create_io_qpair(dev, 1, 1024, 1024);

// Perform synchronous read
char buffer[4096];
int ret = nvme_read(qpair, 1, 0, 1, buffer, sizeof(buffer));

// Perform asynchronous write
void write_complete(void* ctx, int status) {
    printf("Write completed with status %d\n", status);
}
nvme_io_request_t* req = nvme_write_async(qpair, 1, 0, 1, buffer,
                                          sizeof(buffer), write_complete, NULL);

// Poll for completion
while (!nvme_io_is_complete(req)) {
    nvme_process_completions(qpair, 0, NULL);
}

// Cleanup
nvme_destroy_qpair(qpair);
nvme_detach_device(dev);
nvme_shutdown();
```

## Network API

(To be documented - stubs currently)

## GPU API

(To be documented - stubs currently)
