// ===================================================================
// DESC: Core data types, enums, and dynamic data structures for the
//       BCI (Binary C Interface) compiler. Pure C (C99/C11).
// ===================================================================
#ifndef BCI_TYPES_H
#define BCI_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// --- Core Type Enumeration ---
// Defines all primitive types recognized by the compiler.
typedef enum {
    BCI_TYPE_UNKNOWN,
    BCI_TYPE_VOID,
    BCI_TYPE_BOOL,
    BCI_TYPE_I8, BCI_TYPE_U8,
    BCI_TYPE_I16, BCI_TYPE_U16,
    BCI_TYPE_I32, BCI_TYPE_U32,
    BCI_TYPE_I64, BCI_TYPE_U64,
    BCI_TYPE_F32, BCI_TYPE_F64,
    BCI_TYPE_PTR // Generic pointer type
} BciTypeKind;

// Represents a type in the compiler, can be extended for complex types.
typedef struct BciType {
    BciTypeKind kind;
    // For pointer types, points to the base type.
    // For structs/arrays, this could point to more detailed info.
    struct BciType* base;
} BciType;

// --- Dynamic String (similar to an sds string) ---
typedef struct {
    char* data;
    size_t len;
    size_t capacity;
} BciStr;

BciStr* bci_str_new(const char* init);
void bci_str_free(BciStr* str);
void bci_str_append(BciStr* str, const char* suffix);

// --- Generic Dynamic Array (Vector) ---
// A type-safe vector implementation using macros.
#define BciVec(T) struct { T* data; size_t len; size_t capacity; }

#define bci_vec_init(v)     do { (v)->data = NULL; (v)->len = 0; (v)->capacity = 0; } while (0)
#define bci_vec_free(v)     free((v)->data)
#define bci_vec_push(v, val) do { \
    if ((v)->len >= (v)->capacity) { \
        (v)->capacity = ((v)->capacity == 0) ? 8 : (v)->capacity * 2; \
        (v)->data = realloc((v)->data, (v)->capacity * sizeof(*(v)->data)); \
    } \
    (v)->data[(v)->len++] = (val); \
} while (0)

#endif // BCI_TYPES_H


// ===================================================================
// FILE: bci_ast.h
// DESC: Defines the structure of the Abstract Syntax Tree (AST).
//       The AST represents the parsed source code before semantic
//       analysis and code generation.
// ===================================================================
#ifndef BCI_AST_H
#define BCI_AST_H

#include "bci_types.h"

// --- AST Node Types ---
typedef enum {
    AST_NODE_LITERAL,
    AST_NODE_UNARY_OP,
    AST_NODE_BINARY_OP,
    AST_NODE_VARIABLE,
    AST_NODE_ASSIGNMENT,
    AST_NODE_VAR_DECL,
    AST_NODE_IF_STMT,
    AST_NODE_WHILE_STMT,
    AST_NODE_FUNC_CALL,
    AST_NODE_FUNC_DECL,
    AST_NODE_RETURN_STMT,
    AST_NODE_BLOCK,
    AST_NODE_PROGRAM
} AstNodeKind;

// Forward declaration for pointers within the struct.
typedef struct AstNode AstNode;

// --- AST Node Structures ---

// A block is a sequence of statements.
typedef BciVec(AstNode*) AstBlock;

// A literal value (e.g., 123, 3.14, "hello").
typedef struct {
    BciType* type;
    union {
        int64_t i64;
        double f64;
        const char* str;
    } value;
} AstLiteral;

// A binary operation (e.g., a + b).
typedef struct {
    const char* op; // e.g., "+", "==", "&&"
    AstNode* left;
    AstNode* right;
} AstBinaryOp;

// A variable declaration (e.g., int x = 10;).
typedef struct {
    const char* name;
    BciType* type;
    AstNode* initializer;
} AstVarDecl;

// A function declaration.
typedef struct {
    const char* name;
    BciVec(AstVarDecl*) params; // Vector of parameter declarations
    BciType* return_type;
    AstBlock* body;
} AstFuncDecl;

// The main AST node struct, using a tagged union for different node kinds.
struct AstNode {
    AstNodeKind kind;
    BciType* resolved_type; // To be filled in by semantic analysis
    // Location in source file (for error reporting)
    const char* file;
    int line;

    union {
        AstLiteral literal;
        AstBinaryOp binary_op;
        AstVarDecl var_decl;
        AstFuncDecl func_decl;
        AstBlock* block;
        // ... other node types (unary, assignment, if, while, etc.)
    } as;
};

// --- AST Management Functions ---
AstNode* ast_new_node(AstNodeKind kind);
void ast_free_node(AstNode* node);
// Helper functions to create specific nodes, e.g.:
AstNode* ast_new_literal_int(int64_t value);
AstNode* ast_new_binary_op(const char* op, AstNode* left, AstNode* right);
AstNode* ast_new_program();

#endif // BCI_AST_H
