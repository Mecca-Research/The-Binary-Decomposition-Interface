// ===================================================================
// DESC: Implementation of core dynamic data structures.
// ===================================================================

#include "c23_compat.h"
#include "bci_types.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// --- Dynamic String Functions ---
[[nodiscard]] 
BciStr* bci_str_new(const char* init) {
    BciStr* str = malloc(sizeof(BciStr));
    if (!str) {
        perror("Failed to allocate BciStr");
        return nullptr;
    }
    size_t init_len = init ? strlen(init) : 0;
    str->len = init_len;
    str->capacity = init_len > 0 ? init_len : 8; // Start with a minimum capacity
    str->data = malloc(str->capacity + 1); // +1 for null terminator
    if (!str->data) {
        perror("Failed to allocate data for BciStr");
        free(str);
        return nullptr;
    }
    if (init) {
        memcpy(str->data, init, init_len);
    }
    str->data[str->len] = '\0';
    return str;
}

void bci_str_free(BciStr* str) {
    if (!str) return;
    free(str->data);
    free(str);
}

void bci_str_append(BciStr* str, const char* suffix) {
    if (!str || !suffix) return;
    size_t suffix_len = strlen(suffix);
    size_t new_len = str->len + suffix_len;
    if (new_len >= str->capacity) {
        str->capacity = new_len * 2; // Double the required capacity
        char* new_data = realloc(str->data, str->capacity + 1);
        if (!new_data) {
            perror("Failed to reallocate data for BciStr");
            return; // Or handle error more gracefully
        }
        str->data = new_data;
    }
    memcpy(str->data + str->len, suffix, suffix_len);
    str->len = new_len;
    str->data[str->len] = '\0';
}
