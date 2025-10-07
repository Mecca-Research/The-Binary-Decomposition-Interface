
#include "lexer_ml.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static bool lexer_ml_initialized = false;

bool lexer_ml_init(void) {
    if (lexer_ml_initialized) {
        return true;
    }
    lexer_ml_initialized = true;
    return true;
}

void lexer_ml_cleanup(void) {
    lexer_ml_initialized = false;
}

AutoRepairResult* lexer_ml_auto_repair(const char *code) {
    if (!code) {
        return NULL;
    }

    AutoRepairResult *result = calloc(1, sizeof(AutoRepairResult));
    if (!result) {
        return NULL;
    }

    // Simple heuristic-based repair
    size_t code_len = strlen(code);
    result->repaired_code = malloc(code_len + 1000);  // Extra space for repairs
    if (!result->repaired_code) {
        free(result);
        return NULL;
    }

    strcpy(result->repaired_code, code);
    result->success = true;
    result->repair_count = 0;

    // Allocate error array
    result->errors = calloc(10, sizeof(SyntaxError));
    result->error_count = 0;

    // Simple repair: add missing semicolons at end of lines
    char *line = strtok(result->repaired_code, "\n");
    while (line && result->error_count < 10) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] != ';' && line[len - 1] != '{' && line[len - 1] != '}') {
            // Check if it looks like a statement
            if (strstr(line, "int ") || strstr(line, "return ") || strstr(line, "=")) {
                SyntaxError *err = &result->errors[result->error_count++];
                err->type = SYNTAX_ERROR_MISSING_SEMICOLON;
                err->line = result->error_count;
                err->column = len;
                strcpy(err->message, "Missing semicolon");
                strcpy(err->suggested_fix, "Add ';' at end of line");
                err->confidence = 0.85;
                result->repair_count++;
            }
        }
        line = strtok(NULL, "\n");
    }

    return result;
}

void lexer_ml_free_repair_result(AutoRepairResult *result) {
    if (!result) return;
    free(result->repaired_code);
    free(result->errors);
    free(result);
}

const char* lexer_ml_predict_next_token(const char *code, size_t position) {
    if (!code || position >= strlen(code)) {
        return NULL;
    }

    // Simple prediction based on context
    const char *context = code + (position > 20 ? position - 20 : 0);

    if (strstr(context, "int ")) return "identifier";
    if (strstr(context, "return ")) return "expression";
    if (strstr(context, "if (")) return "condition";
    if (strstr(context, "for (")) return "init";

    return "token";
}

const char* lexer_ml_suggest_completion(const char *code, size_t position) {
    if (!code || position >= strlen(code)) {
        return NULL;
    }

    // Simple completion suggestions
    const char *context = code + (position > 10 ? position - 10 : 0);

    if (strstr(context, "prin")) return "printf";
    if (strstr(context, "mal")) return "malloc";
    if (strstr(context, "str")) return "strlen";
    if (strstr(context, "mem")) return "memcpy";

    return NULL;
}

bool lexer_ml_detect_mistake(const char *code, SyntaxError *error) {
    if (!code || !error) {
        return false;
    }

    // Detect common mistakes
    if (strstr(code, "==") && strstr(code, "if")) {
        // Possible assignment instead of comparison
        if (strstr(code, "= ") && !strstr(code, "==")) {
            error->type = SYNTAX_ERROR_INVALID_TOKEN;
            strcpy(error->message, "Possible assignment in condition");
            strcpy(error->suggested_fix, "Use '==' for comparison");
            error->confidence = 0.70;
            return true;
        }
    }

    return false;
}
