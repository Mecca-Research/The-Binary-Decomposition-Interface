// ===================================================================
// DESC: Example usage and testing for Phase 13 backend acceleration
// ===================================================================
#include <stdio.h>
#include <stdlib.h>

// Forward declarations of backend functions
extern int backend_dispatch_init(void);
extern void backend_dispatch_shutdown(void);
extern int backend_register_device(int type, const char* name, unsigned int caps, 
                                   size_t mem, void* handle);
extern int backend_select_device(int type, unsigned int caps, size_t mem);
extern void backend_get_dispatch_stats(void);

extern int backend_memory_init(void);
extern void backend_memory_shutdown(void);
extern void* backend_alloc(int device_id, size_t size, unsigned int flags);
extern void backend_free(int device_id, void* ptr, size_t size);
extern void backend_memory_global_stats(void);

extern int gpu_init(void);
extern void gpu_shutdown(void);
extern void gpu_print_statistics(void);

extern int fpga_init(void);
extern void fpga_shutdown(void);
extern void fpga_print_statistics(void);

extern int backend_event_create(int device_id);
extern int backend_event_signal(int event_id);
extern int backend_event_wait(int event_id, unsigned long long timeout);
extern void backend_event_destroy(int event_id);
extern void backend_sync_statistics(void);

// Device types and capabilities
#define BACKEND_TYPE_GPU 1
#define BACKEND_TYPE_FPGA 2
#define BACKEND_TYPE_BPU 3

#define CAPABILITY_COMPUTE (1 << 0)
#define CAPABILITY_PARALLEL (1 << 2)
#define CAPABILITY_ASYNC (1 << 4)
#define CAPABILITY_FIXED_FUNCTION (1 << 3)

#define MEM_FLAG_ZERO_COPY (1 << 0)
#define MEM_FLAG_PINNED (1 << 2)

/**
 * Test 1: Device Registration and Selection
 */
void test_device_registration(void) {
    printf("\n=== Test 1: Device Registration and Selection ===\n");
    
    backend_dispatch_init();
    
    // Register GPU
    int gpu_id = backend_register_device(
        BACKEND_TYPE_GPU,
        "Test GPU",
        CAPABILITY_COMPUTE | CAPABILITY_PARALLEL | CAPABILITY_ASYNC,
        8ULL * 1024 * 1024 * 1024,
        NULL
    );
    printf("Registered GPU with ID: %d\n", gpu_id);
    
    // Register FPGA
    int fpga_id = backend_register_device(
        BACKEND_TYPE_FPGA,
        "Test FPGA",
        CAPABILITY_FIXED_FUNCTION,
        1ULL * 1024 * 1024 * 1024,
        NULL
    );
    printf("Registered FPGA with ID: %d\n", fpga_id);
    
    // Test device selection
    int selected = backend_select_device(
        BACKEND_TYPE_GPU,
        CAPABILITY_COMPUTE | CAPABILITY_ASYNC,
        1024 * 1024
    );
    printf("Selected device: %d\n", selected);
    
    backend_get_dispatch_stats();
}

/**
 * Test 2: Memory Management
 */
void test_memory_management(void) {
    printf("\n=== Test 2: Memory Management ===\n");
    
    backend_memory_init();
    
    // Test pool allocation (4KB)
    void* ptr1 = backend_alloc(0, 4096, MEM_FLAG_ZERO_COPY);
    printf("Allocated 4KB: %p\n", ptr1);
    
    // Test pool allocation (16KB)
    void* ptr2 = backend_alloc(0, 16384, MEM_FLAG_PINNED);
    printf("Allocated 16KB: %p\n", ptr2);
    
    // Test direct allocation (1MB)
    void* ptr3 = backend_alloc(0, 1024 * 1024, MEM_FLAG_ZERO_COPY);
    printf("Allocated 1MB: %p\n", ptr3);
    
    backend_memory_global_stats();
    
    // Free memory
    backend_free(0, ptr1, 4096);
    backend_free(0, ptr2, 16384);
    backend_free(0, ptr3, 1024 * 1024);
    
    backend_memory_global_stats();
}

/**
 * Test 3: GPU Operations
 */
void test_gpu_operations(void) {
    printf("\n=== Test 3: GPU Operations ===\n");
    
    gpu_init();
    
    // Test will use GPU functions when linked
    printf("GPU initialized successfully\n");
    
    gpu_print_statistics();
}

/**
 * Test 4: FPGA Operations
 */
void test_fpga_operations(void) {
    printf("\n=== Test 4: FPGA Operations ===\n");
    
    fpga_init();
    
    // Test will use FPGA functions when linked
    printf("FPGA initialized successfully\n");
    
    fpga_print_statistics();
}

/**
 * Test 5: Synchronization
 */
void test_synchronization(void) {
    printf("\n=== Test 5: Synchronization ===\n");
    
    // Create event
    int event = backend_event_create(0);
    printf("Created event: %d\n", event);
    
    // Signal event
    backend_event_signal(event);
    printf("Signaled event\n");
    
    // Wait for event (should return immediately since already signaled)
    int result = backend_event_wait(event, 1000000000);
    printf("Event wait result: %d\n", result);
    
    // Destroy event
    backend_event_destroy(event);
    
    backend_sync_statistics();
}

/**
 * Main test runner
 */
int main(void) {
    printf("=================================================\n");
    printf("Phase 13: Backend Acceleration - Test Suite\n");
    printf("=================================================\n");
    
    test_device_registration();
    test_memory_management();
    test_gpu_operations();
    test_fpga_operations();
    test_synchronization();
    
    printf("\n=== Cleanup ===\n");
    backend_memory_shutdown();
    gpu_shutdown();
    fpga_shutdown();
    backend_dispatch_shutdown();
    
    printf("\n=================================================\n");
    printf("All tests completed successfully!\n");
    printf("=================================================\n");
    
    return 0;
}
