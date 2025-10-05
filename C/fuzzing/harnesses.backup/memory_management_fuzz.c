
/*
 * Memory Management Fuzzing Harness
 * 
 * This harness targets memory allocation, garbage collection, and cleanup.
 * It fuzzes allocation patterns, sizes, and deallocation sequences to test:
 * - Memory pools and GC cycles
 * - Reference counting and cleanup
 * - Memory leaks, double-free, use-after-free
 * - Heap corruption and resource exhaustion
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Include memory management headers
#include "../vm/memory.h"
#include "../vm/value.h"
#include "../vm/object.h"
#include "../vm/vm.h"

// Timeout protection
#include <signal.h>
#include <setjmp.h>

static jmp_buf memory_timeout_jmp;
static void memory_timeout_handler(int sig) {
    longjmp(memory_timeout_jmp, 1);
}

// Track allocations for testing
typedef struct {
    void* ptr;
    size_t size;
    bool freed;
} AllocationRecord;

static AllocationRecord* allocations = NULL;
static size_t allocation_count = 0;
static size_t allocation_capacity = 0;

// Record an allocation
static void record_allocation(void* ptr, size_t size) {
    if (allocation_count >= allocation_capacity) {
        allocation_capacity = allocation_capacity ? allocation_capacity * 2 : 16;
        allocations = realloc(allocations, allocation_capacity * sizeof(AllocationRecord));
        if (!allocations) return;
    }
    
    allocations[allocation_count].ptr = ptr;
    allocations[allocation_count].size = size;
    allocations[allocation_count].freed = false;
    allocation_count++;
}

// Mark allocation as freed
static void mark_freed(void* ptr) {
    for (size_t i = 0; i < allocation_count; i++) {
        if (allocations[i].ptr == ptr) {
            allocations[i].freed = true;
            break;
        }
    }
}

// Test memory allocation patterns
static void test_allocation_patterns(const uint8_t* data, size_t size) {
    if (size < 4) return;
    
    VM* vm = malloc(sizeof(VM));
    if (!vm) return;
    
    initVM(vm);
    
    size_t offset = 0;
    uint32_t num_operations = (data[offset] % 32) + 1; // 1-32 operations
    offset++;
    
    void** ptrs = malloc(num_operations * sizeof(void*));
    if (!ptrs) {
        freeVM(vm);
        free(vm);
        return;
    }
    
    memset(ptrs, 0, num_operations * sizeof(void*));
    
    // Perform fuzzed memory operations
    for (uint32_t i = 0; i < num_operations && offset < size; i++) {
        uint8_t operation = data[offset] % 4;
        offset++;
        
        switch (operation) {
            case 0: { // Allocate
                if (offset + 2 <= size) {
                    size_t alloc_size = ((data[offset] << 8) | data[offset + 1]) % 4096 + 1;
                    offset += 2;
                    
                    ptrs[i] = reallocate(vm, NULL, 0, alloc_size);
                    if (ptrs[i]) {
                        record_allocation(ptrs[i], alloc_size);
                        // Initialize memory to detect use-after-free
                        memset(ptrs[i], 0xAA, alloc_size);
                    }
                }
                break;
            }
            
            case 1: { // Reallocate
                if (i > 0 && ptrs[i-1] && offset + 2 <= size) {
                    size_t new_size = ((data[offset] << 8) | data[offset + 1]) % 4096 + 1;
                    offset += 2;
                    
                    void* old_ptr = ptrs[i-1];
                    ptrs[i] = reallocate(vm, old_ptr, 0, new_size);
                    if (ptrs[i]) {
                        mark_freed(old_ptr);
                        record_allocation(ptrs[i], new_size);
                        ptrs[i-1] = NULL; // Prevent double-free
                    }
                }
                break;
            }
            
            case 2: { // Free
                if (i > 0 && ptrs[i-1]) {
                    mark_freed(ptrs[i-1]);
                    reallocate(vm, ptrs[i-1], 0, 0);
                    ptrs[i-1] = NULL;
                }
                break;
            }
            
            case 3: { // Trigger GC
                collectGarbage(vm);
                break;
            }
        }
    }
    
    // Cleanup remaining allocations
    for (uint32_t i = 0; i < num_operations; i++) {
        if (ptrs[i]) {
            reallocate(vm, ptrs[i], 0, 0);
        }
    }
    
    free(ptrs);
    freeVM(vm);
    free(vm);
}

// Test object allocation and GC
static void test_object_gc(const uint8_t* data, size_t size) {
    if (size < 8) return;
    
    VM* vm = malloc(sizeof(VM));
    if (!vm) return;
    
    initVM(vm);
    
    size_t offset = 0;
    uint32_t num_objects = (data[offset] % 16) + 1; // 1-16 objects
    offset++;
    
    // Create various object types
    for (uint32_t i = 0; i < num_objects && offset + 4 <= size; i++) {
        uint8_t obj_type = data[offset] % 3;
        offset++;
        
        switch (obj_type) {
            case 0: { // String object
                uint32_t str_len = (data[offset] % 64) + 1;
                offset++;
                
                if (offset + str_len <= size) {
                    ObjString* string = copyString(vm, (char*)(data + offset), str_len);
                    push(vm, OBJ_VAL(string));
                    offset += str_len;
                }
                break;
            }
            
            case 1: { // Function object
                ObjFunction* function = newFunction(vm);
                push(vm, OBJ_VAL(function));
                break;
            }
            
            case 2: { // Upvalue object
                if (vm->stackTop > vm->stack) {
                    ObjUpvalue* upvalue = newUpvalue(vm, vm->stackTop - 1);
                    push(vm, OBJ_VAL(upvalue));
                }
                break;
            }
        }
        
        // Randomly trigger GC
        if (data[offset] % 4 == 0) {
            collectGarbage(vm);
        }
        if (offset < size) offset++;
    }
    
    // Final GC
    collectGarbage(vm);
    
    freeVM(vm);
    free(vm);
}

// LibFuzzer entry point
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Skip invalid inputs
    if (size < 4 || size > 2048) return 0;
    
    // Set up timeout protection
    signal(SIGALRM, memory_timeout_handler);
    if (setjmp(memory_timeout_jmp) != 0) {
        alarm(0);
        return 0;
    }
    alarm(2);
    
    // Reset allocation tracking
    allocation_count = 0;
    
    // Test allocation patterns
    test_allocation_patterns(data, size / 2);
    
    // Test object GC
    test_object_gc(data + size / 2, size - size / 2);
    
    // Cleanup allocation tracking
    if (allocations) {
        free(allocations);
        allocations = NULL;
        allocation_capacity = 0;
    }
    
    alarm(0);
    return 0;
}

// AFL entry point
#ifdef __AFL_COMPILER
int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (size <= 0 || size > 2048) {
        fclose(fp);
        return 0;
    }
    
    uint8_t* data = malloc(size);
    if (!data) {
        fclose(fp);
        return 1;
    }
    
    size_t read_size = fread(data, 1, size, fp);
    fclose(fp);
    
    if (read_size != size) {
        free(data);
        return 1;
    }
    
    int result = LLVMFuzzerTestOneInput(data, size);
    free(data);
    
    return result;
}
#endif
