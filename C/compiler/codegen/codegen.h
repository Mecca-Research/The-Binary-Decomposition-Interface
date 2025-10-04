
// ===================================================================
// DESC: Bytecode Code Generator - Generates bytecode from AST
// ===================================================================
#ifndef BCI_CODEGEN_H
#define BCI_CODEGEN_H

#include "../../vm/bci_chunk.h"
#include "../ast/bci_ast.h"
#include "../semantic_analyzer/bci_symbol.h"
#include <stdbool.h>
#include <stdint.h>

// Extended opcodes for complete bytecode generation
typedef enum {
    // Existing opcodes (from bci_chunk.h)
    OPCODE_RETURN = OP_RETURN,
    OPCODE_CONSTANT = OP_CONSTANT,
    OPCODE_NEGATE = OP_NEGATE,
    OPCODE_ADD = OP_ADD,
    OPCODE_SUBTRACT = OP_SUBTRACT,
    OPCODE_MULTIPLY = OP_MULTIPLY,
    OPCODE_DIVIDE = OP_DIVIDE,
    
    // New opcodes for complete implementation
    OPCODE_MODULO,
    OPCODE_POWER,
    
    // Stack operations
    OPCODE_POP,
    OPCODE_DUP,
    OPCODE_SWAP,
    
    // Comparison operations
    OPCODE_EQUAL,
    OPCODE_NOT_EQUAL,
    OPCODE_GREATER,
    OPCODE_GREATER_EQUAL,
    OPCODE_LESS,
    OPCODE_LESS_EQUAL,
    
    // Logical operations
    OPCODE_NOT,
    OPCODE_AND,
    OPCODE_OR,
    
    // Control flow
    OPCODE_JUMP,
    OPCODE_JUMP_IF_FALSE,
    OPCODE_JUMP_IF_TRUE,
    OPCODE_LOOP,
    
    // Variable operations
    OPCODE_GET_LOCAL,
    OPCODE_SET_LOCAL,
    OPCODE_GET_GLOBAL,
    OPCODE_SET_GLOBAL,
    OPCODE_DEFINE_GLOBAL,
    
    // Function operations
    OPCODE_CALL,
    OPCODE_CALL_NATIVE,
    OPCODE_DEFINE_FUNCTION,
    OPCODE_CLOSURE,
    OPCODE_GET_UPVALUE,
    OPCODE_SET_UPVALUE,
    OPCODE_CLOSE_UPVALUE,
    
    // Array operations
    OPCODE_BUILD_ARRAY,
    OPCODE_GET_INDEX,
    OPCODE_SET_INDEX,
    
    // Object operations
    OPCODE_BUILD_OBJECT,
    OPCODE_GET_PROPERTY,
    OPCODE_SET_PROPERTY,
    
    // Type operations
    OPCODE_CAST,
    OPCODE_TYPEOF,
    
    // Special operations
    OPCODE_PRINT,
    OPCODE_ASSERT,
    OPCODE_NOP,
    OPCODE_HALT,
    
    OPCODE_COUNT
} ExtendedOpCode;

// Local variable information
typedef struct {
    const char* name;
    int depth;          // Scope depth
    bool is_captured;   // Is this variable captured by a closure?
} Local;

// Upvalue information (for closures)
typedef struct {
    uint8_t index;
    bool is_local;
} Upvalue;

// Jump patch information
typedef struct {
    size_t offset;      // Offset in bytecode to patch
    int scope_depth;    // Scope depth when jump was emitted
} JumpPatch;

// Function compilation context
typedef struct FunctionContext {
    struct FunctionContext* enclosing;  // Enclosing function (for nested functions)
    
    const char* name;
    int arity;          // Number of parameters
    
    // Local variables
    Local locals[256];
    int local_count;
    int scope_depth;
    
    // Upvalues (captured variables)
    Upvalue upvalues[256];
    int upvalue_count;
    
    // Chunk being compiled
    Chunk* chunk;
} FunctionContext;

// Code generator state
typedef struct {
    // Current function being compiled
    FunctionContext* current_function;
    
    // Symbol table (from semantic analysis)
    SymbolTable* symbol_table;
    
    // Global variables
    const char* globals[256];
    int global_count;
    
    // Jump patches (for forward jumps)
    JumpPatch jump_patches[256];
    int jump_patch_count;
    
    // Error handling
    bool had_error;
    const char* error_message;
    int error_line;
    
    // Configuration
    bool optimize;
    bool debug_info;
} CodeGenerator;

// Code generator API
CodeGenerator* codegen_create(void);
void codegen_destroy(CodeGenerator* codegen);

// Function context management (exposed for testing)
FunctionContext* function_context_create(FunctionContext* enclosing, const char* name);
void function_context_destroy(FunctionContext* context);

// Main code generation function
bool codegen_generate(CodeGenerator* codegen, AstNode* program, Chunk* chunk);

// Node-specific code generation
void codegen_node(CodeGenerator* codegen, AstNode* node);
void codegen_literal(CodeGenerator* codegen, AstNode* node);
void codegen_binary_op(CodeGenerator* codegen, AstNode* node);
void codegen_unary_op(CodeGenerator* codegen, AstNode* node);
void codegen_variable(CodeGenerator* codegen, AstNode* node);
void codegen_assignment(CodeGenerator* codegen, AstNode* node);
void codegen_var_decl(CodeGenerator* codegen, AstNode* node);
void codegen_if_statement(CodeGenerator* codegen, AstNode* node);
void codegen_while_statement(CodeGenerator* codegen, AstNode* node);
void codegen_for_statement(CodeGenerator* codegen, AstNode* node);
void codegen_function_decl(CodeGenerator* codegen, AstNode* node);
void codegen_function_call(CodeGenerator* codegen, AstNode* node);
void codegen_return_statement(CodeGenerator* codegen, AstNode* node);
void codegen_block(CodeGenerator* codegen, AstNode* node);

// Helper functions
void codegen_emit_byte(CodeGenerator* codegen, uint8_t byte);
void codegen_emit_bytes(CodeGenerator* codegen, uint8_t byte1, uint8_t byte2);
void codegen_emit_constant(CodeGenerator* codegen, double value);
int codegen_emit_jump(CodeGenerator* codegen, uint8_t opcode);
void codegen_patch_jump(CodeGenerator* codegen, int offset);
void codegen_emit_loop(CodeGenerator* codegen, int loop_start);

// Scope management
void codegen_begin_scope(CodeGenerator* codegen);
void codegen_end_scope(CodeGenerator* codegen);

// Variable management
int codegen_resolve_local(CodeGenerator* codegen, const char* name);
int codegen_resolve_upvalue(CodeGenerator* codegen, FunctionContext* function, const char* name);
int codegen_add_local(CodeGenerator* codegen, const char* name);
int codegen_add_global(CodeGenerator* codegen, const char* name);

// Error reporting
void codegen_error(CodeGenerator* codegen, const char* message);
void codegen_error_at(CodeGenerator* codegen, int line, const char* message);

// Optimization passes
void codegen_optimize_chunk(Chunk* chunk);
void codegen_constant_folding(Chunk* chunk);
void codegen_dead_code_elimination(Chunk* chunk);
void codegen_peephole_optimization(Chunk* chunk);

// Disassembly (for debugging)
void codegen_disassemble_chunk(const Chunk* chunk, const char* name);
int codegen_disassemble_instruction(const Chunk* chunk, int offset);

#endif // BCI_CODEGEN_H
