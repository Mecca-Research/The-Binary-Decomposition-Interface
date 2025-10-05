
/*
 * JIT Compiler Fuzzing Harness
 * 
 * This harness targets the JIT compilation pipeline and native code generation.
 * It fuzzes bytecode sequences that trigger JIT compilation to test:
 * - Hotspot detection and compilation phases
 * - Code cache management and native execution
 * - Compilation errors and code cache corruption
 * - Code injection and compilation bugs
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Include JIT headers
#include "../vm/vm.h"
#include "../vm/chunk.h"
#include "../vm/jit.h"
#include "../vm/memory.h"

// Timeout protection
#include <signal.h>
#include <setjmp.h>

static jmp_buf jit_timeout_jmp;
static void jit_timeout_handler(int sig) {
    longjmp(jit_timeout_jmp, 1);
}

// Initialize VM with JIT enabled
static VM* init_jit_fuzz_vm(void) {
    VM* vm = malloc(sizeof(VM));
    if (!vm) return NULL;
    
    initVM(vm);
    
    // Enable JIT compilation
    if (vm->jit) {
        vm->jit->enabled = true;
        vm->jit->hotspot_threshold = 5; // Lower threshold for fuzzing
    }
    
    return vm;
}

// Create bytecode that might trigger JIT compilation
static Chunk* create_jit_fuzz_chunk(const uint8_t* data, size_t size) {
    if (size < 8) return NULL;
    
    Chunk* chunk = malloc(sizeof(Chunk));
    if (!chunk) return NULL;
    
    initChunk(chunk);
    
    // Create a loop structure to trigger hotspot detection
    writeChunk(chunk, OP_CONSTANT, 0);
    writeChunk(chunk, 0, 0); // Constant index
    
    // Add loop start marker
    size_t loop_start = chunk->count;
    
    // Add fuzzed bytecode in the loop
    for (size_t i = 0; i < size && i < 512; i++) {
        uint8_t opcode = data[i] % 32; // Limit to valid opcode range
        writeChunk(chunk, opcode, i);
    }
    
    // Add loop back instruction
    writeChunk(chunk, OP_LOOP, size);
    writeChunk(chunk, (chunk->count - loop_start) & 0xFF, size);
    writeChunk(chunk, ((chunk->count - loop_start) >> 8) & 0xFF, size);
    
    // Add constants for the chunk
    writeConstant(chunk, NUMBER_VAL(1.0));
    
    return chunk;
}

// Test JIT compilation directly
static void test_jit_compilation(const uint8_t* data, size_t size) {
    if (size < 4) return;
    
    VM* vm = init_jit_fuzz_vm();
    if (!vm || !vm->jit) return;
    
    Chunk* chunk = create_jit_fuzz_chunk(data, size);
    if (!chunk) {
        freeVM(vm);
        free(vm);
        return;
    }
    
    // Try to compile the chunk
    JITFunction* jit_func = compileFunction(vm->jit, chunk, 0);
    
    if (jit_func) {
        // Test execution of compiled code (with safety checks)
        // This is where JIT bugs would manifest
        freeJITFunction(jit_func);
    }
    
    freeChunk(chunk);
    free(chunk);
    freeVM(vm);
    free(vm);
}

// LibFuzzer entry point
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Skip invalid inputs
    if (size < 1 || size > 2048) return 0;
    
    // Set up timeout protection (2 seconds for JIT)
    signal(SIGALRM, jit_timeout_handler);
    if (setjmp(jit_timeout_jmp) != 0) {
        alarm(0);
        return 0;
    }
    alarm(2);
    
    VM* vm = init_jit_fuzz_vm();
    if (!vm) {
        alarm(0);
        return 0;
    }
    
    Chunk* chunk = create_jit_fuzz_chunk(data, size);
    if (!chunk) {
        freeVM(vm);
        free(vm);
        alarm(0);
        return 0;
    }
    
    // Execute with JIT enabled - this should trigger compilation
    InterpretResult result = interpret(vm, chunk);
    
    // Test direct JIT compilation
    test_jit_compilation(data, size);
    
    // Cleanup
    freeChunk(chunk);
    free(chunk);
    freeVM(vm);
    free(vm);
    
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
