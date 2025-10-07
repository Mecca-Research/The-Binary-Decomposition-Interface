
#include "unicode_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parse allkeys.txt
bool parse_collation_keys(const char *filename, unicode_collation_key_t **keys_out, size_t *count_out) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return false;
    }
    
    size_t capacity = 150000;
    unicode_collation_key_t *keys = calloc(capacity, sizeof(unicode_collation_key_t));
    if (!keys) {
        fclose(file);
        return false;
    }
    
    size_t count = 0;
    char line[4096];
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '@') continue;
        
        // Parse line: codepoint ; [.primary.secondary.tertiary]
        char *semicolon = strchr(line, ';');
        if (!semicolon) continue;
        
        *semicolon = '\0';
        uint32_t codepoint = (uint32_t)strtoul(line, NULL, 16);
        
        if (count >= capacity) {
            capacity *= 2;
            unicode_collation_key_t *new_keys = realloc(keys, capacity * sizeof(unicode_collation_key_t));
            if (!new_keys) {
                free(keys);
                fclose(file);
                return false;
            }
            keys = new_keys;
        }
        
        keys[count].codepoint = codepoint;
        keys[count].primary = 0;
        keys[count].secondary = 0;
        keys[count].tertiary = 0;
        
        // Parse weights (simplified - just take first weight)
        char *bracket = strchr(semicolon + 1, '[');
        if (bracket) {
            sscanf(bracket + 1, ".%X.%X.%X", 
                   &keys[count].primary,
                   &keys[count].secondary,
                   &keys[count].tertiary);
        }
        
        count++;
    }
    
    fclose(file);
    
    *keys_out = keys;
    *count_out = count;
    
    printf("Parsed %zu collation keys from %s\n", count, filename);
    return true;
}

