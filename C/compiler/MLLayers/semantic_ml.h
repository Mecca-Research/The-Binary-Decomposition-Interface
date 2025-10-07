
#ifndef BDI_SEMANTIC_ML_H
#define BDI_SEMANTIC_ML_H

#include <stdbool.h>
#include <stddef.h>

// Variable role types
typedef enum {
    VAR_ROLE_COUNTER,
    VAR_ROLE_ACCUMULATOR,
    VAR_ROLE_POINTER,
    VAR_ROLE_BUFFER,
    VAR_ROLE_FLAG,
    VAR_ROLE_TEMPORARY,
    VAR_ROLE_UNKNOWN
} VariableRole;

// Memory operation types
typedef enum {
    MEM_OP_ALLOC,
    MEM_OP_FREE,
    MEM_OP_READ,
    MEM_OP_WRITE,
    MEM_OP_REALLOC
} MemoryOperationType;

// Memory issue
typedef struct {
    MemoryOperationType op_type;
    char variable_name[64];
    char issue_description[256];
    double confidence;
    bool is_critical;
} MemoryIssue;

// Initialize semantic ML
bool semantic_ml_init(void);

// Cleanup semantic ML
void semantic_ml_cleanup(void);

// Infer variable role
VariableRole semantic_ml_infer_role(const char *var_name, const char *context);

// Flag memory operations
bool semantic_ml_flag_memory_op(const char *code, MemoryIssue *issue);

// Suggest annotations
const char* semantic_ml_suggest_annotation(const char *var_name, VariableRole role);

// Detect potential memory leaks
bool semantic_ml_detect_leak(const char *code, char *leak_description, size_t desc_size);

// Detect use-after-free
bool semantic_ml_detect_use_after_free(const char *code, char *description, size_t desc_size);

#endif // BDI_SEMANTIC_ML_H
