
/*
 * VM Bytecode Execution Fuzzing Harness
 * 
 * This harness targets the VM bytecode execution engine and interpreter.
 * It fuzzes bytecode sequences with various opcodes and operands to test:
 * - All VM opcodes and stack operations
 * - Control flow and error handling
 * - Memory safety and infinite loop prevention
 * - Stack overflow and invalid opcodes
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Include VM headers
#include "vm/bci_vm.h"
#include "vm/bci_chunk.h"
// Value types are in bci_chunk.h (ValueArray)
// Memory management handled internally

// Timeout protection for infinite loops
#include <signal.h>
#include <setjmp.h>

static jmp_buf timeout_jmp;
static void timeout_handler(int sig) {
    longjmp(timeout_jmp, 1);
}

// Initialize VM for fuzzing
static VM* init_fuzz_vm(void) {
    VM* vm = malloc(sizeof(VM));
    if (!vm) return NULL;
    
    vm_init(vm);
    return vm;
}

// Cleanup VM after fuzzing
static void cleanup_fuzz_vm(VM* vm) {
    if (vm) {
        vm_free(vm);
        free(vm);
    }
}

// Create chunk from fuzzed bytecode
static Chunk* create_fuzz_chunk(const uint8_t* data, size_t size) {
    if (size < 4) return NULL;
    
    Chunk* chunk = malloc(sizeof(Chunk));
    if (!chunk) return NULL;
    
    chunk_init(chunk);
    
    // Add fuzzed bytecode to chunk
    for (size_t i = 0; i < size && i < 1024; i++) {
        chunk_write(chunk, data[i], i);
    }
    
    return chunk;
}

// LibFuzzer entry point
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Skip empty or too small inputs
    if (size < 1 || size > 4096) return 0;
    
    // Set up timeout protection (1 second)
    signal(SIGALRM, timeout_handler);
    if (setjmp(timeout_jmp) != 0) {
        alarm(0);
        return 0; // Timeout occurred
    }
    alarm(1);
    
    VM* vm = init_fuzz_vm();
    if (!vm) {
        alarm(0);
        return 0;
    }
    
    Chunk* chunk = create_fuzz_chunk(data, size);
    if (!chunk) {
        cleanup_fuzz_vm(vm);
        alarm(0);
        return 0;
    }
    
    // Execute the fuzzed bytecode
    InterpretResult result = vm_interpret(vm, chunk);
    
    // Cleanup
    chunk_free(chunk);
    free(chunk);
    cleanup_fuzz_vm(vm);
    
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
    
    // Read input file
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (size <= 0 || size > 4096) {
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
    
    // Call fuzzer
    int result = LLVMFuzzerTestOneInput(data, size);
    free(data);
    
    return result;
}
#endif
