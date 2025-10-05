
// ===================================================================
// DESC: Bytecode Code Generator Implementation
// ===================================================================
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Opcode names for disassembly
static const char* opcode_names[] = {
    [OPCODE_RETURN] = "RETURN",
    [OPCODE_CONSTANT] = "CONSTANT",
    [OPCODE_NEGATE] = "NEGATE",
    [OPCODE_ADD] = "ADD",
    [OPCODE_SUBTRACT] = "SUBTRACT",
    [OPCODE_MULTIPLY] = "MULTIPLY",
    [OPCODE_DIVIDE] = "DIVIDE",
    [OPCODE_MODULO] = "MODULO",
    [OPCODE_POWER] = "POWER",
    [OPCODE_POP] = "POP",
    [OPCODE_DUP] = "DUP",
    [OPCODE_SWAP] = "SWAP",
    [OPCODE_EQUAL] = "EQUAL",
    [OPCODE_NOT_EQUAL] = "NOT_EQUAL",
    [OPCODE_GREATER] = "GREATER",
    [OPCODE_GREATER_EQUAL] = "GREATER_EQUAL",
    [OPCODE_LESS] = "LESS",
    [OPCODE_LESS_EQUAL] = "LESS_EQUAL",
    [OPCODE_NOT] = "NOT",
    [OPCODE_AND] = "AND",
    [OPCODE_OR] = "OR",
    [OPCODE_JUMP] = "JUMP",
    [OPCODE_JUMP_IF_FALSE] = "JUMP_IF_FALSE",
    [OPCODE_JUMP_IF_TRUE] = "JUMP_IF_TRUE",
    [OPCODE_LOOP] = "LOOP",
    [OPCODE_GET_LOCAL] = "GET_LOCAL",
    [OPCODE_SET_LOCAL] = "SET_LOCAL",
    [OPCODE_GET_GLOBAL] = "GET_GLOBAL",
    [OPCODE_SET_GLOBAL] = "SET_GLOBAL",
    [OPCODE_DEFINE_GLOBAL] = "DEFINE_GLOBAL",
    [OPCODE_CALL] = "CALL",
    [OPCODE_CALL_NATIVE] = "CALL_NATIVE",
    [OPCODE_DEFINE_FUNCTION] = "DEFINE_FUNCTION",
    [OPCODE_CLOSURE] = "CLOSURE",
    [OPCODE_GET_UPVALUE] = "GET_UPVALUE",
    [OPCODE_SET_UPVALUE] = "SET_UPVALUE",
    [OPCODE_CLOSE_UPVALUE] = "CLOSE_UPVALUE",
    [OPCODE_BUILD_ARRAY] = "BUILD_ARRAY",
    [OPCODE_GET_INDEX] = "GET_INDEX",
    [OPCODE_SET_INDEX] = "SET_INDEX",
    [OPCODE_BUILD_OBJECT] = "BUILD_OBJECT",
    [OPCODE_GET_PROPERTY] = "GET_PROPERTY",
    [OPCODE_SET_PROPERTY] = "SET_PROPERTY",
    [OPCODE_CAST] = "CAST",
    [OPCODE_TYPEOF] = "TYPEOF",
    [OPCODE_PRINT] = "PRINT",
    [OPCODE_ASSERT] = "ASSERT",
    [OPCODE_NOP] = "NOP",
    [OPCODE_HALT] = "HALT",
};

// Create a new code generator
CodeGenerator* codegen_create(void) {
    CodeGenerator* codegen = malloc(sizeof(CodeGenerator));
    if (!codegen) return NULL;
    
    codegen->current_function = NULL;
    codegen->symbol_table = NULL;
    codegen->global_count = 0;
    codegen->jump_patch_count = 0;
    codegen->had_error = false;
    codegen->error_message = NULL;
    codegen->error_line = 0;
    codegen->optimize = false;
    codegen->debug_info = true;
    
    return codegen;
}

// Destroy code generator
void codegen_destroy(CodeGenerator* codegen) {
    if (!codegen) return;
    
    // Free function contexts
    while (codegen->current_function) {
        FunctionContext* enclosing = codegen->current_function->enclosing;
        free(codegen->current_function);
        codegen->current_function = enclosing;
    }
    
    free(codegen);
}

// Create a new function context
FunctionContext* function_context_create(FunctionContext* enclosing, const char* name) {
    FunctionContext* context = malloc(sizeof(FunctionContext));
    if (!context) return NULL;
    
    context->enclosing = enclosing;
    context->name = name;
    context->arity = 0;
    context->local_count = 0;
    context->scope_depth = 0;
    context->upvalue_count = 0;
    context->chunk = NULL;
    
    return context;
}

// Emit a single byte
void codegen_emit_byte(CodeGenerator* codegen, uint8_t byte) {
    chunk_write(codegen->current_function->chunk, byte, 0);
}

// Emit two bytes
void codegen_emit_bytes(CodeGenerator* codegen, uint8_t byte1, uint8_t byte2) {
    codegen_emit_byte(codegen, byte1);
    codegen_emit_byte(codegen, byte2);
}

// Emit a constant
void codegen_emit_constant(CodeGenerator* codegen, double value) {
    int constant_index = chunk_add_constant(codegen->current_function->chunk, value);
    if (constant_index < 256) {
        codegen_emit_bytes(codegen, OPCODE_CONSTANT, (uint8_t)constant_index);
    } else {
        codegen_error(codegen, "Too many constants in one chunk");
    }
}

// Emit a jump instruction and return its offset for later patching
int codegen_emit_jump(CodeGenerator* codegen, uint8_t opcode) {
    codegen_emit_byte(codegen, opcode);
    codegen_emit_bytes(codegen, 0xff, 0xff);  // Placeholder
    return codegen->current_function->chunk->count - 2;
}

// Patch a jump instruction
void codegen_patch_jump(CodeGenerator* codegen, int offset) {
    Chunk* chunk = codegen->current_function->chunk;
    int jump = chunk->count - offset - 2;
    
    if (jump > UINT16_MAX) {
        codegen_error(codegen, "Too much code to jump over");
        return;
    }
    
    chunk->code[offset] = (jump >> 8) & 0xff;
    chunk->code[offset + 1] = jump & 0xff;
}

// Emit a loop instruction
void codegen_emit_loop(CodeGenerator* codegen, int loop_start) {
    codegen_emit_byte(codegen, OPCODE_LOOP);
    
    int offset = codegen->current_function->chunk->count - loop_start + 2;
    if (offset > UINT16_MAX) {
        codegen_error(codegen, "Loop body too large");
        return;
    }
    
    codegen_emit_bytes(codegen, (offset >> 8) & 0xff, offset & 0xff);
}

// Begin a new scope
void codegen_begin_scope(CodeGenerator* codegen) {
    codegen->current_function->scope_depth++;
}

// End current scope
void codegen_end_scope(CodeGenerator* codegen) {
    FunctionContext* function = codegen->current_function;
    function->scope_depth--;
    
    // Pop local variables that go out of scope
    while (function->local_count > 0 &&
           function->locals[function->local_count - 1].depth > function->scope_depth) {
        
        if (function->locals[function->local_count - 1].is_captured) {
            codegen_emit_byte(codegen, OPCODE_CLOSE_UPVALUE);
        } else {
            codegen_emit_byte(codegen, OPCODE_POP);
        }
        function->local_count--;
    }
}

// Add a local variable
int codegen_add_local(CodeGenerator* codegen, const char* name) {
    FunctionContext* function = codegen->current_function;
    
    if (function->local_count >= 256) {
        codegen_error(codegen, "Too many local variables in function");
        return -1;
    }
    
    Local* local = &function->locals[function->local_count];
    local->name = name;
    local->depth = function->scope_depth;
    local->is_captured = false;
    
    return function->local_count++;
}

// Resolve a local variable
int codegen_resolve_local(CodeGenerator* codegen, const char* name) {
    FunctionContext* function = codegen->current_function;
    
    for (int i = function->local_count - 1; i >= 0; i--) {
        if (strcmp(function->locals[i].name, name) == 0) {
            return i;
        }
    }
    
    return -1;  // Not found
}

// Add an upvalue
static int add_upvalue(FunctionContext* function, uint8_t index, bool is_local) {
    // Check if upvalue already exists
    for (int i = 0; i < function->upvalue_count; i++) {
        Upvalue* upvalue = &function->upvalues[i];
        if (upvalue->index == index && upvalue->is_local == is_local) {
            return i;
        }
    }
    
    if (function->upvalue_count >= 256) {
        return -1;  // Too many upvalues
    }
    
    function->upvalues[function->upvalue_count].is_local = is_local;
    function->upvalues[function->upvalue_count].index = index;
    return function->upvalue_count++;
}

// Resolve an upvalue (captured variable)
int codegen_resolve_upvalue(CodeGenerator* codegen, FunctionContext* function, const char* name) {
    if (function->enclosing == NULL) {
        return -1;  // No enclosing function
    }
    
    // Try to resolve in enclosing function's locals
    int local = -1;
    FunctionContext* enclosing = function->enclosing;
    for (int i = enclosing->local_count - 1; i >= 0; i--) {
        if (strcmp(enclosing->locals[i].name, name) == 0) {
            local = i;
            break;
        }
    }
    
    if (local != -1) {
        enclosing->locals[local].is_captured = true;
        return add_upvalue(function, (uint8_t)local, true);
    }
    
    // Try to resolve in enclosing function's upvalues
    int upvalue = codegen_resolve_upvalue(codegen, enclosing, name);
    if (upvalue != -1) {
        return add_upvalue(function, (uint8_t)upvalue, false);
    }
    
    return -1;  // Not found
}

// Add a global variable
int codegen_add_global(CodeGenerator* codegen, const char* name) {
    if (codegen->global_count >= 256) {
        codegen_error(codegen, "Too many global variables");
        return -1;
    }
    
    // Check if global already exists
    for (int i = 0; i < codegen->global_count; i++) {
        if (strcmp(codegen->globals[i], name) == 0) {
            return i;
        }
    }
    
    codegen->globals[codegen->global_count] = name;
    return codegen->global_count++;
}

// Error reporting
void codegen_error(CodeGenerator* codegen, const char* message) {
    codegen->had_error = true;
    codegen->error_message = message;
}

void codegen_error_at(CodeGenerator* codegen, int line, const char* message) {
    codegen->had_error = true;
    codegen->error_message = message;
    codegen->error_line = line;
}

// Generate code for a literal
void codegen_literal(CodeGenerator* codegen, AstNode* node) {
    assert(node->kind == AST_NODE_LITERAL);
    
    AstLiteral* literal = &node->as.literal;
    
    // For now, only handle numeric literals
    // Check if type is an integer type
    if (literal->type && (literal->type->kind == BCI_TYPE_I8 || 
                          literal->type->kind == BCI_TYPE_I16 ||
                          literal->type->kind == BCI_TYPE_I32 ||
                          literal->type->kind == BCI_TYPE_I64 ||
                          literal->type->kind == BCI_TYPE_U8 ||
                          literal->type->kind == BCI_TYPE_U16 ||
                          literal->type->kind == BCI_TYPE_U32 ||
                          literal->type->kind == BCI_TYPE_U64)) {
        codegen_emit_constant(codegen, (double)literal->value.i64);
    } else if (literal->type && (literal->type->kind == BCI_TYPE_F32 ||
                                 literal->type->kind == BCI_TYPE_F64)) {
        codegen_emit_constant(codegen, literal->value.f64);
    } else {
        // Default: treat as double
        codegen_emit_constant(codegen, literal->value.f64);
    }
}

// Generate code for a binary operation
void codegen_binary_op(CodeGenerator* codegen, AstNode* node) {
    assert(node->kind == AST_NODE_BINARY_OP);
    
    AstBinaryOp* binop = &node->as.binary_op;
    
    // Handle short-circuit operators specially
    if (strcmp(binop->op, "&&") == 0) {
        // Short-circuit AND: left && right
        // Evaluate left operand first
        codegen_node(codegen, binop->left);
        
        // If left is false, skip right evaluation and keep false on stack
        int end_jump = codegen_emit_jump(codegen, OPCODE_JUMP_IF_FALSE);
        
        // Left was true, pop it and evaluate right
        codegen_emit_byte(codegen, OPCODE_POP);
        codegen_node(codegen, binop->right);
        
        // Patch the jump to here (if left was false, we skip right and keep left's false value)
        codegen_patch_jump(codegen, end_jump);
        return;
    } else if (strcmp(binop->op, "||") == 0) {
        // Short-circuit OR: left || right
        // Evaluate left operand first
        codegen_node(codegen, binop->left);
        
        // If left is true, skip right evaluation and keep true on stack
        int end_jump = codegen_emit_jump(codegen, OPCODE_JUMP_IF_TRUE);
        
        // Left was false, pop it and evaluate right
        codegen_emit_byte(codegen, OPCODE_POP);
        codegen_node(codegen, binop->right);
        
        // Patch the jump to here (if left was true, we skip right and keep left's true value)
        codegen_patch_jump(codegen, end_jump);
        return;
    }
    
    // For all other operators, evaluate both operands
    codegen_node(codegen, binop->left);
    codegen_node(codegen, binop->right);
    
    // Emit operation
    if (strcmp(binop->op, "+") == 0) {
        codegen_emit_byte(codegen, OPCODE_ADD);
    } else if (strcmp(binop->op, "-") == 0) {
        codegen_emit_byte(codegen, OPCODE_SUBTRACT);
    } else if (strcmp(binop->op, "*") == 0) {
        codegen_emit_byte(codegen, OPCODE_MULTIPLY);
    } else if (strcmp(binop->op, "/") == 0) {
        codegen_emit_byte(codegen, OPCODE_DIVIDE);
    } else if (strcmp(binop->op, "%") == 0) {
        codegen_emit_byte(codegen, OPCODE_MODULO);
    } else if (strcmp(binop->op, "==") == 0) {
        codegen_emit_byte(codegen, OPCODE_EQUAL);
    } else if (strcmp(binop->op, "!=") == 0) {
        codegen_emit_byte(codegen, OPCODE_NOT_EQUAL);
    } else if (strcmp(binop->op, ">") == 0) {
        codegen_emit_byte(codegen, OPCODE_GREATER);
    } else if (strcmp(binop->op, ">=") == 0) {
        codegen_emit_byte(codegen, OPCODE_GREATER_EQUAL);
    } else if (strcmp(binop->op, "<") == 0) {
        codegen_emit_byte(codegen, OPCODE_LESS);
    } else if (strcmp(binop->op, "<=") == 0) {
        codegen_emit_byte(codegen, OPCODE_LESS_EQUAL);
    } else {
        codegen_error(codegen, "Unknown binary operator");
    }
}

// Generate code for an if statement
void codegen_if_statement(CodeGenerator* codegen, AstNode* node) {
    // This is a placeholder - full implementation would handle AST_NODE_IF_STMT
    // For now, demonstrate the pattern
    
    // Generate condition
    // codegen_node(codegen, if_stmt->condition);
    
    // Jump to else branch if condition is false
    int else_jump = codegen_emit_jump(codegen, OPCODE_JUMP_IF_FALSE);
    codegen_emit_byte(codegen, OPCODE_POP);  // Pop condition
    
    // Generate then branch
    // codegen_node(codegen, if_stmt->then_branch);
    
    // Jump over else branch
    int end_jump = codegen_emit_jump(codegen, OPCODE_JUMP);
    
    // Patch else jump
    codegen_patch_jump(codegen, else_jump);
    codegen_emit_byte(codegen, OPCODE_POP);  // Pop condition
    
    // Generate else branch (if exists)
    // if (if_stmt->else_branch) {
    //     codegen_node(codegen, if_stmt->else_branch);
    // }
    
    // Patch end jump
    codegen_patch_jump(codegen, end_jump);
}

// Generate code for a while statement
void codegen_while_statement(CodeGenerator* codegen, AstNode* node) {
    // Mark loop start
    int loop_start = codegen->current_function->chunk->count;
    
    // Generate condition
    // codegen_node(codegen, while_stmt->condition);
    
    // Jump out of loop if condition is false
    int exit_jump = codegen_emit_jump(codegen, OPCODE_JUMP_IF_FALSE);
    codegen_emit_byte(codegen, OPCODE_POP);  // Pop condition
    
    // Generate body
    // codegen_node(codegen, while_stmt->body);
    
    // Loop back to start
    codegen_emit_loop(codegen, loop_start);
    
    // Patch exit jump
    codegen_patch_jump(codegen, exit_jump);
    codegen_emit_byte(codegen, OPCODE_POP);  // Pop condition
}

// Generate code for a generic node
void codegen_node(CodeGenerator* codegen, AstNode* node) {
    if (!node) return;
    
    switch (node->kind) {
        case AST_NODE_LITERAL:
            codegen_literal(codegen, node);
            break;
        case AST_NODE_BINARY_OP:
            codegen_binary_op(codegen, node);
            break;
        case AST_NODE_BLOCK:
            codegen_block(codegen, node);
            break;
        // Add more cases as needed
        default:
            codegen_error(codegen, "Unsupported AST node type");
            break;
    }
}

// Generate code for a block
void codegen_block(CodeGenerator* codegen, AstNode* node) {
    assert(node->kind == AST_NODE_BLOCK);
    
    codegen_begin_scope(codegen);
    
    AstBlock* block = node->as.block;
    // BciVec uses 'len' not 'count'
    for (size_t i = 0; i < block->len; i++) {
        codegen_node(codegen, block->data[i]);
    }
    
    codegen_end_scope(codegen);
}

// Main code generation function
bool codegen_generate(CodeGenerator* codegen, AstNode* program, Chunk* chunk) {
    // Create main function context
    codegen->current_function = function_context_create(NULL, "<main>");
    codegen->current_function->chunk = chunk;
    
    // Generate code for program
    codegen_node(codegen, program);
    
    // Emit return
    codegen_emit_byte(codegen, OPCODE_RETURN);
    
    // Apply optimizations if enabled
    if (codegen->optimize) {
        codegen_optimize_chunk(chunk);
    }
    
    return !codegen->had_error;
}

// Optimization: constant folding
void codegen_constant_folding(Chunk* chunk) {
    // Simple peephole optimization for constant operations
    for (int i = 0; i < chunk->count - 2; i++) {
        if (chunk->code[i] == OPCODE_CONSTANT &&
            chunk->code[i + 2] == OPCODE_CONSTANT) {
            // Two consecutive constants
            uint8_t const1_idx = chunk->code[i + 1];
            uint8_t const2_idx = chunk->code[i + 3];
            
            if (i + 4 < chunk->count) {
                uint8_t op = chunk->code[i + 4];
                double val1 = chunk->constants.data[const1_idx];
                double val2 = chunk->constants.data[const2_idx];
                double result = 0;
                bool can_fold = true;
                
                switch (op) {
                    case OPCODE_ADD: result = val1 + val2; break;
                    case OPCODE_SUBTRACT: result = val1 - val2; break;
                    case OPCODE_MULTIPLY: result = val1 * val2; break;
                    case OPCODE_DIVIDE:
                        if (val2 != 0) result = val1 / val2;
                        else can_fold = false;
                        break;
                    default: can_fold = false; break;
                }
                
                if (can_fold) {
                    // Replace with single constant
                    int new_const = chunk_add_constant(chunk, result);
                    chunk->code[i + 1] = (uint8_t)new_const;
                    // Mark removed instructions as NOPs
                    chunk->code[i + 2] = OPCODE_NOP;
                    chunk->code[i + 3] = OPCODE_NOP;
                    chunk->code[i + 4] = OPCODE_NOP;
                }
            }
        }
    }
}

// Optimization: dead code elimination
void codegen_dead_code_elimination(Chunk* chunk) {
    // Remove unreachable code after RETURN
    for (int i = 0; i < chunk->count; i++) {
        if (chunk->code[i] == OPCODE_RETURN) {
            // Mark everything after as NOP until next label/jump target
            for (int j = i + 1; j < chunk->count; j++) {
                uint8_t op = chunk->code[j];
                if (op == OPCODE_JUMP || op == OPCODE_JUMP_IF_FALSE || 
                    op == OPCODE_JUMP_IF_TRUE || op == OPCODE_LOOP) {
                    break;  // Stop at jump targets
                }
                chunk->code[j] = OPCODE_NOP;
            }
        }
    }
}

// Optimization: peephole optimization
void codegen_peephole_optimization(Chunk* chunk) {
    // Need at least 2 bytes for DUP+POP pattern, 3 bytes for CONSTANT+arg+POP pattern
    for (int i = 0; i < chunk->count - 1; i++) {
        // CONSTANT followed by POP -> remove both
        // Need to check i+2 is valid before accessing it
        if (i + 2 < chunk->count && 
            chunk->code[i] == OPCODE_CONSTANT && 
            chunk->code[i + 2] == OPCODE_POP) {
            chunk->code[i] = OPCODE_NOP;
            chunk->code[i + 1] = OPCODE_NOP;
            chunk->code[i + 2] = OPCODE_NOP;
        }
        
        // DUP followed by POP -> NOP
        if (chunk->code[i] == OPCODE_DUP && chunk->code[i + 1] == OPCODE_POP) {
            chunk->code[i] = OPCODE_NOP;
            chunk->code[i + 1] = OPCODE_NOP;
        }
    }
}

// Apply all optimizations
void codegen_optimize_chunk(Chunk* chunk) {
    codegen_constant_folding(chunk);
    codegen_dead_code_elimination(chunk);
    codegen_peephole_optimization(chunk);
}

// Disassemble a chunk
void codegen_disassemble_chunk(const Chunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    
    for (int offset = 0; offset < chunk->count;) {
        offset = codegen_disassemble_instruction(chunk, offset);
    }
}

// Disassemble a single instruction
int codegen_disassemble_instruction(const Chunk* chunk, int offset) {
    printf("%04d ", offset);
    
    uint8_t instruction = chunk->code[offset];
    
    if (instruction >= OPCODE_COUNT) {
        printf("Unknown opcode %d\n", instruction);
        return offset + 1;
    }
    
    const char* name = opcode_names[instruction];
    
    switch (instruction) {
        case OPCODE_CONSTANT: {
            uint8_t constant = chunk->code[offset + 1];
            printf("%-16s %4d '", name, constant);
            printf("%g", chunk->constants.data[constant]);
            printf("'\n");
            return offset + 2;
        }
        
        case OPCODE_GET_LOCAL:
        case OPCODE_SET_LOCAL:
        case OPCODE_GET_GLOBAL:
        case OPCODE_SET_GLOBAL:
        case OPCODE_DEFINE_GLOBAL:
        case OPCODE_CALL: {
            uint8_t slot = chunk->code[offset + 1];
            printf("%-16s %4d\n", name, slot);
            return offset + 2;
        }
        
        case OPCODE_JUMP:
        case OPCODE_JUMP_IF_FALSE:
        case OPCODE_JUMP_IF_TRUE:
        case OPCODE_LOOP: {
            uint16_t jump = (uint16_t)(chunk->code[offset + 1] << 8);
            jump |= chunk->code[offset + 2];
            printf("%-16s %4d -> %d\n", name, offset, 
                   instruction == OPCODE_LOOP ? offset - jump : offset + 3 + jump);
            return offset + 3;
        }
        
        default:
            printf("%s\n", name);
            return offset + 1;
    }
}

// Placeholder implementations for unimplemented functions
void codegen_unary_op(CodeGenerator* codegen, AstNode* node) {
    (void)codegen; (void)node;
    // TODO: Implement
}

void codegen_variable(CodeGenerator* codegen, AstNode* node) {
    (void)codegen; (void)node;
    // TODO: Implement
}

void codegen_assignment(CodeGenerator* codegen, AstNode* node) {
    (void)codegen; (void)node;
    // TODO: Implement
}

void codegen_var_decl(CodeGenerator* codegen, AstNode* node) {
    (void)codegen; (void)node;
    // TODO: Implement
}

void codegen_for_statement(CodeGenerator* codegen, AstNode* node) {
    (void)codegen; (void)node;
    // TODO: Implement
}

void codegen_function_decl(CodeGenerator* codegen, AstNode* node) {
    (void)codegen; (void)node;
    // TODO: Implement
}

void codegen_function_call(CodeGenerator* codegen, AstNode* node) {
    (void)codegen; (void)node;
    // TODO: Implement
}

void codegen_return_statement(CodeGenerator* codegen, AstNode* node) {
    (void)codegen; (void)node;
    // TODO: Implement
}
