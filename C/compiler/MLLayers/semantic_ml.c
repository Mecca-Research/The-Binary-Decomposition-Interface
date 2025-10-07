
#include "semantic_ml.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static bool semantic_ml_initialized = false;

bool semantic_ml_init(void) {
    if (semantic_ml_initialized) {
        return true;
    }
    semantic_ml_initialized = true;
    return true;
}

void semantic_ml_cleanup(void) {
    semantic_ml_initialized = false;
}

VariableRole semantic_ml_infer_role(const char *var_name, const char *context) {
    if (!var_name) {
        return VAR_ROLE_UNKNOWN;
    }

    // Simple heuristic-based role inference
    if (strstr(var_name, "count") || strstr(var_name, "idx") || strstr(var_name, "i")) {
        return VAR_ROLE_COUNTER;
    }
    if (strstr(var_name, "sum") || strstr(var_name, "total") || strstr(var_name, "acc")) {
        return VAR_ROLE_ACCUMULATOR;
    }
    if (strstr(var_name, "ptr") || strstr(var_name, "p") || (context && strstr(context, "*"))) {
        return VAR_ROLE_POINTER;
    }
    if (strstr(var_name, "buf") || strstr(var_name, "buffer")) {
        return VAR_ROLE_BUFFER;
    }
    if (strstr(var_name, "flag") || strstr(var_name, "is_") || strstr(var_name, "has_")) {
        return VAR_ROLE_FLAG;
    }
    if (strstr(var_name, "tmp") || strstr(var_name, "temp")) {
        return VAR_ROLE_TEMPORARY;
    }

    return VAR_ROLE_UNKNOWN;
}

bool semantic_ml_flag_memory_op(const char *code, MemoryIssue *issue) {
    if (!code || !issue) {
        return false;
    }

    // Detect malloc without free
    if (strstr(code, "malloc(") && !strstr(code, "free(")) {
        issue->op_type = MEM_OP_ALLOC;
        strcpy(issue->issue_description, "malloc() without corresponding free()");
        issue->confidence = 0.75;
        issue->is_critical = true;
        return true;
    }

    // Detect potential buffer overflow
    if (strstr(code, "strcpy(") || strstr(code, "sprintf(")) {
        issue->op_type = MEM_OP_WRITE;
        strcpy(issue->issue_description, "Unsafe string operation - potential buffer overflow");
        issue->confidence = 0.80;
        issue->is_critical = true;
        return true;
    }

    return false;
}

const char* semantic_ml_suggest_annotation(const char *var_name, VariableRole role) {
    if (!var_name) {
        return NULL;
    }

    switch (role) {
        case VAR_ROLE_POINTER:
            return "/* @nullable */ or /* @nonnull */";
        case VAR_ROLE_BUFFER:
            return "/* @buffer_size(n) */";
        case VAR_ROLE_COUNTER:
            return "/* @range(0, max) */";
        default:
            return NULL;
    }
}

bool semantic_ml_detect_leak(const char *code, char *leak_description, size_t desc_size) {
    if (!code || !leak_description) {
        return false;
    }

    // Simple leak detection
    if (strstr(code, "malloc(") && !strstr(code, "free(")) {
        snprintf(leak_description, desc_size, 
                "Potential memory leak: malloc() without free()");
        return true;
    }

    return false;
}

bool semantic_ml_detect_use_after_free(const char *code, char *description, size_t desc_size) {
    if (!code || !description) {
        return false;
    }

    // Simple use-after-free detection
    // This is a placeholder - real implementation would need flow analysis
    if (strstr(code, "free(") && strstr(code, "->")) {
        snprintf(description, desc_size,
                "Potential use-after-free: pointer dereference after free()");
        return true;
    }

    return false;
}
