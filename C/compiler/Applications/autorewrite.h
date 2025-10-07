
#ifndef BDI_AUTOREWRITE_H
#define BDI_AUTOREWRITE_H

#include <stdbool.h>
#include <stddef.h>

// Rewrite suggestion
typedef struct {
    char original_code[512];
    char suggested_code[512];
    char reason[256];
    double confidence;
    bool is_safety_improvement;
} RewriteSuggestion;

// Rewrite result
typedef struct {
    RewriteSuggestion *suggestions;
    size_t suggestion_count;
    char *rewritten_code;
} RewriteResult;

// Initialize autorewrite
bool autorewrite_init(void);

// Cleanup autorewrite
void autorewrite_cleanup(void);

// Analyze code and suggest rewrites
RewriteResult* autorewrite_analyze(const char *code);

// Apply rewrites
char* autorewrite_apply(const char *code, const RewriteSuggestion *suggestions, size_t count);

// Free rewrite result
void autorewrite_free_result(RewriteResult *result);

// Learn from codebase
bool autorewrite_learn_from_codebase(const char *codebase_path);

#endif // BDI_AUTOREWRITE_H
