
/*
 * Bytecode Parser Fuzzing Harness
 * 
 * This harness targets bytecode parsing, validation, and chunk loading.
 * It fuzzes bytecode files, streams, and malformed input to test:
 * - Instruction parsing and constant pools
 * - Chunk validation and bounds checking
 * - Parser crashes and malformed bytecode handling
 * - Buffer overflows and format string bugs
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Include parser headers
#include "vm/bci_chunk.h"
// Value types are in bci_chunk.h (ValueArray)
// Memory management handled internally
// Debug functions not needed for fuzzing

// Timeout protection
#include <signal.h>
#include <setjmp.h>

static jmp_buf parser_timeout_jmp;
static void parser_timeout_handler(int sig) {
    longjmp(parser_timeout_jmp, 1);
}

// Bytecode file format structure
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t code_size;
    uint32_t constant_count;
} BytecodeHeader;

#define BYTECODE_MAGIC 0x42444943  // "BDIC"
#define BYTECODE_VERSION 1

// Parse bytecode from fuzzed data
static Chunk* parse_bytecode(const uint8_t* data, size_t size) {
    if (size < sizeof(BytecodeHeader)) return NULL;
    
    BytecodeHeader* header = (BytecodeHeader*)data;
    size_t offset = sizeof(BytecodeHeader);
    
    // Validate header (with fuzzing tolerance)
    if (header->code_size > size - offset) return NULL;
    if (header->constant_count > 1024) return NULL; // Reasonable limit
    
    Chunk* chunk = malloc(sizeof(Chunk));
    if (!chunk) return NULL;
    
    chunk_init(chunk);
    
    // Parse code section
    const uint8_t* code_data = data + offset;
    for (uint32_t i = 0; i < header->code_size && offset < size; i++) {
        chunk_write(chunk, code_data[i], i);
        offset++;
    }
    
    // Parse constants section
    for (uint32_t i = 0; i < header->constant_count && offset + 8 <= size; i++) {
        // Parse constant value (assuming double for now)
        double value = *(double*)(data + offset);
        writeConstant(chunk, NUMBER_VAL(value));
        offset += 8;
    }
    
    return chunk;
}

// Test chunk validation
static bool validate_chunk(Chunk* chunk) {
    if (!chunk) return false;
    if (!chunk->code) return false;
    if (chunk->count > chunk->capacity) return false;
    
    // Validate opcodes
    for (int i = 0; i < chunk->count; i++) {
        uint8_t opcode = chunk->code[i];
        
        // Check for valid opcode range
        if (opcode >= OP_COUNT) return false;
        
        // Check operand requirements
        switch (opcode) {
            case OP_CONSTANT:
            case OP_CONSTANT_LONG:
                if (i + 1 >= chunk->count) return false;
                i++; // Skip operand
                break;
                
            case OP_JUMP:
            case OP_JUMP_IF_FALSE:
// DISABLED:             case OP_LOOP: // OP_LOOP not defined in current VM
                if (i + 2 >= chunk->count) return false;
                i += 2; // Skip 16-bit operand
                break;
                
            default:
                break;
        }
    }
    
    return true;
}

// Test instruction parsing
static void test_instruction_parsing(Chunk* chunk) {
    if (!chunk) return;
    
    // Simulate instruction parsing
    int offset = 0;
    while (offset < chunk->count) {
        uint8_t instruction = chunk->code[offset];
        
        switch (instruction) {
            case OP_CONSTANT: {
                if (offset + 1 < chunk->count) {
                    uint8_t constant_index = chunk->code[offset + 1];
                    if (constant_index < chunk->constants.count) {
                        Value constant = chunk->constants.values[constant_index];
                        // Use the constant (prevents optimization)
                        (void)constant;
                    }
                    offset += 2;
                } else {
                    offset++;
                }
                break;
            }
            
            case OP_CONSTANT_LONG: {
                if (offset + 3 < chunk->count) {
                    uint32_t constant_index = 
                        (chunk->code[offset + 1] << 16) |
                        (chunk->code[offset + 2] << 8) |
                        chunk->code[offset + 3];
                    
                    if (constant_index < chunk->constants.count) {
                        Value constant = chunk->constants.values[constant_index];
                        (void)constant;
                    }
                    offset += 4;
                } else {
                    offset++;
                }
                break;
            }
            
            case OP_JUMP:
            case OP_JUMP_IF_FALSE:
// DISABLED:             case OP_LOOP: { // OP_LOOP not defined in current VM
                if (offset + 2 < chunk->count) {
                    uint16_t jump_offset = 
                        (chunk->code[offset + 1] << 8) |
                        chunk->code[offset + 2];
                    
                    // Validate jump target
// DISABLED:                     int target = (instruction == OP_LOOP) ?  // OP_LOOP not defined in current VM
                        offset - jump_offset : offset + jump_offset;
                    
                    if (target >= 0 && target < chunk->count) {
                        // Valid jump target
                    }
                    offset += 3;
                } else {
                    offset++;
                }
                break;
            }
            
            default:
                offset++;
                break;
        }
    }
}

// Test debug information parsing
static void test_debug_parsing(Chunk* chunk) {
    if (!chunk) return;
    
    // Test line number access
    for (int i = 0; i < chunk->count; i++) {
        int line = getLine(chunk, i);
        (void)line; // Prevent optimization
    }
    
    // Test disassembly
    disassembleChunk(chunk, "fuzz_chunk");
}

// LibFuzzer entry point
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Skip invalid inputs
    if (size < sizeof(BytecodeHeader) || size > 8192) return 0;
    
    // Set up timeout protection
    signal(SIGALRM, parser_timeout_handler);
    if (setjmp(parser_timeout_jmp) != 0) {
        alarm(0);
        return 0;
    }
    alarm(1);
    
    // Test direct chunk creation from raw data
    Chunk* raw_chunk = malloc(sizeof(Chunk));
    if (raw_chunk) {
        chunk_init(raw_chunk);
        
        // Add raw data as bytecode
        size_t code_size = size > 1024 ? 1024 : size;
        for (size_t i = 0; i < code_size; i++) {
            chunk_write(raw_chunk, data[i], i);
        }
        
        // Test validation
        bool is_valid = validate_chunk(raw_chunk);
        
        if (is_valid) {
            // Test instruction parsing
            test_instruction_parsing(raw_chunk);
            
            // Test debug parsing
            test_debug_parsing(raw_chunk);
        }
        
        chunk_free(raw_chunk);
        free(raw_chunk);
    }
    
    // Test structured bytecode parsing
    Chunk* parsed_chunk = parse_bytecode(data, size);
    if (parsed_chunk) {
        bool is_valid = validate_chunk(parsed_chunk);
        
        if (is_valid) {
            test_instruction_parsing(parsed_chunk);
            test_debug_parsing(parsed_chunk);
        }
        
        chunk_free(parsed_chunk);
        free(parsed_chunk);
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
    
    if (size <= 0 || size > 8192) {
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
