
#include "test_framework.h"
#include <unistd.h>
#include <sys/stat.h>

// Global counters
int test_framework_pass_count = 0;
int test_framework_fail_count = 0;
size_t test_framework_memory_allocated = 0;
static size_t memory_checkpoint = 0;

void test_framework_init(void) {
    test_framework_pass_count = 0;
    test_framework_fail_count = 0;
    test_framework_memory_allocated = 0;
    memory_checkpoint = 0;
    
    printf("=== BDI Kernel Test Framework Initialized ===\n");
}

void test_framework_cleanup(void) {
    if (test_framework_memory_allocated > 0) {
        printf("WARNING: Memory leak detected - %zu bytes not freed\n", 
               test_framework_memory_allocated);
    }
}

bool test_framework_run_suite(const test_suite_t* suite) {
    printf("\n--- Running Test Suite: %s ---\n", suite->name);
    
    int suite_pass = 0;
    int suite_fail = 0;
    
    for (size_t i = 0; i < suite->test_count; i++) {
        int before_pass = test_framework_pass_count;
        int before_fail = test_framework_fail_count;
        
        printf("Running test %zu/%zu... ", i + 1, suite->test_count);
        fflush(stdout);
        
        bool result = suite->tests[i]();
        
        int test_pass = test_framework_pass_count - before_pass;
        int test_fail = test_framework_fail_count - before_fail;
        
        if (result && test_fail == 0) {
            printf("PASS (%d assertions)\n", test_pass);
            suite_pass++;
        } else {
            printf("FAIL (%d passed, %d failed)\n", test_pass, test_fail);
            suite_fail++;
        }
    }
    
    printf("Suite Results: %d/%zu tests passed\n", suite_pass, suite->test_count);
    return suite_fail == 0;
}

void test_framework_print_summary(void) {
    int total = test_framework_pass_count + test_framework_fail_count;
    double pass_rate = total > 0 ? (double)test_framework_pass_count / total * 100.0 : 0.0;
    
    printf("\n=== Test Summary ===\n");
    printf("Total Assertions: %d\n", total);
    printf("Passed: %d\n", test_framework_pass_count);
    printf("Failed: %d\n", test_framework_fail_count);
    printf("Pass Rate: %.2f%%\n", pass_rate);
    
    if (test_framework_memory_allocated > 0) {
        printf("Memory Leaks: %zu bytes\n", test_framework_memory_allocated);
    } else {
        printf("Memory: Clean (no leaks detected)\n");
    }
    
    printf("===================\n");
}

void test_framework_memory_checkpoint(void) {
    memory_checkpoint = test_framework_memory_allocated;
}

bool test_framework_memory_verify(const char* message) {
    if (test_framework_memory_allocated != memory_checkpoint) {
        printf("MEMORY LEAK: %s - %zu bytes leaked\n", 
               message, test_framework_memory_allocated - memory_checkpoint);
        return false;
    }
    return true;
}

void* test_malloc(size_t size) {
    void* ptr = malloc(size);
    if (ptr) {
        test_framework_memory_allocated += size;
    }
    return ptr;
}

void test_free(void* ptr) {
    if (ptr) {
        // Note: In a real implementation, we'd track allocation sizes
        // For simplicity, we'll just decrement by a fixed amount
        // This is a limitation of this simple framework
        free(ptr);
        if (test_framework_memory_allocated > 0) {
            test_framework_memory_allocated -= sizeof(void*); // Approximation
        }
    }
}

char* test_create_temp_file(const char* content) {
    static char filename[256];
    snprintf(filename, sizeof(filename), "/tmp/bdi_test_%d_%ld.tmp", 
             getpid(), time(NULL));
    
    FILE* file = fopen(filename, "w");
    if (!file) return NULL;
    
    if (content) {
        fputs(content, file);
    }
    fclose(file);
    
    return filename;
}

void test_remove_temp_file(const char* filename) {
    if (filename) {
        unlink(filename);
    }
}
