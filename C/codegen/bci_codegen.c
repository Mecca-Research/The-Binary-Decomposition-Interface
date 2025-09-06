// ===================================================================
// DESC: Implementation of the Code Generator.
// ===================================================================

#include "bci_codegen.h"
#include <stdio.h>
#include <stdlib.h>

// --- Forward Declarations ---
static void generate_node(CodeGenerator* codegen, AstNode* node);

// --- Helper Functions to Emit Bytecode ---

// Writes a single byte to the chunk.
static void emit_byte(CodeGenerator* codegen, uint8_t byte, int line) {
    chunk_write(codegen->compiling_chunk, byte, line);
}

// Writes two bytes to the chunk.
static void emit_bytes(CodeGenerator* codegen, uint8_t byte1, uint8_t byte2, int line) {
    emit_byte(codegen, byte1, line);
    emit_byte(codegen, byte2, line);
}

// Emits an OP_CONSTANT instruction.
static void emit_constant(CodeGenerator* codegen, double value, int line) {
    int constant_index = chunk_add_constant(codegen->compiling_chunk, value);
    if (constant_index > 255) {
        // Handle case where we have more than 256 constants.
        // For now, we'll just error. A real compiler would add an OP_CONSTANT_LONG.
        fprintf(stderr, "Too many constants in one chunk.\n");
        codegen->had_error = true;
        return;
    }
    emit_bytes(codegen, OP_CONSTANT, (uint8_t)constant_index, line);
}

// --- Recursive Traversal Functions ---

// Main dispatcher function to generate code for any AST node.
static void generate_node(CodeGenerator* codegen, AstNode* node) {
    if (!node) return;

    switch (node->kind) {
        case AST_NODE_LITERAL: {
            // Our VM currently uses doubles for all values.
            emit_constant(codegen, (double)node->as.literal.value.i64, node->line);
            break;
        }
        case AST_NODE_BINARY_OP: {
            generate_node(codegen, node->as.binary_op.left);
            generate_node(codegen, node->as.binary_op.right);

            const char* op = node->as.binary_op.op;
            if (strcmp(op, "+") == 0)      emit_byte(codegen, OP_ADD, node->line);
            else if (strcmp(op, "-") == 0) emit_byte(codegen, OP_SUBTRACT, node->line);
            else if (strcmp(op, "*") == 0) emit_byte(codegen, OP_MULTIPLY, node->line);
            else if (strcmp(op, "/") == 0) emit_byte(codegen, OP_DIVIDE, node->line);
            // Add cases for comparison operators later.
            break;
        }
        case AST_NODE_UNARY_OP: {
             if (node->as.binary_op.op && strcmp(node->as.binary_op.op, "-") == 0) {
                generate_node(codegen, node->as.binary_op.right);
                emit_byte(codegen, OP_NEGATE, node->line);
            }
            // Add other unary ops like '!' later.
            break;
        }
        case AST_NODE_PROGRAM:
        case AST_NODE_BLOCK: {
            for (size_t i = 0; i < node->as.block->len; ++i) {
                generate_node(codegen, node->as.block->data[i]);
            }
            break;
        }
        // Add cases for statements and declarations as they are added to the language.
        default:
            fprintf(stderr, "Codegen Error: Unknown AST node kind %d\n", node->kind);
            codegen->had_error = true;
            break;
    }
}


// --- Code Generator Public API Implementation ---

void codegen_init(CodeGenerator* codegen, Chunk* chunk) {
    codegen->compiling_chunk = chunk;
    codegen->had_error = false;
}

void codegen_free(CodeGenerator* codegen) {
    // Does not own the chunk, nothing to free for now.
}

bool codegen_generate(CodeGenerator* codegen, AstNode* program) {
    generate_node(codegen, program);

    // After all statements, emit a final return to exit the VM.
    if (!codegen->had_error) {
        emit_byte(codegen, OP_RETURN, program->line);
    }

    return !codegen->had_error;
}
