
#include "unicode_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parse Unihan_Readings.txt
bool parse_unihan_readings(const char *filename, unicode_unihan_t **unihan_out, size_t *count_out) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return false;
    }
    
    size_t capacity = 100000;
    unicode_unihan_t *unihan = calloc(capacity, sizeof(unicode_unihan_t));
    if (!unihan) {
        fclose(file);
        return false;
    }
    
    size_t count = 0;
    char line[4096];
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Parse line: U+XXXX kProperty value
        if (line[0] != 'U' || line[1] != '+') continue;
        
        char *tab1 = strchr(line, '\t');
        if (!tab1) continue;
        *tab1 = '\0';
        
        char *tab2 = strchr(tab1 + 1, '\t');
        if (!tab2) continue;
        *tab2 = '\0';
        
        uint32_t codepoint = (uint32_t)strtoul(line + 2, NULL, 16);
        const char *property = tab1 + 1;
        const char *value = tab2 + 1;
        
        // Remove newline from value
        char *newline = strchr(value, '\n');
        if (newline) *newline = '\0';
        
        // Find or create entry
        size_t idx = count;
        for (size_t i = 0; i < count; i++) {
            if (unihan[i].codepoint == codepoint) {
                idx = i;
                break;
            }
        }
        
        if (idx == count) {
            if (count >= capacity) {
                capacity *= 2;
                unicode_unihan_t *new_unihan = realloc(unihan, capacity * sizeof(unicode_unihan_t));
                if (!new_unihan) {
                    free(unihan);
                    fclose(file);
                    return false;
                }
                unihan = new_unihan;
            }
            unihan[count].codepoint = codepoint;
            count++;
        }
        
        // Store property value
        if (strcmp(property, "kMandarin") == 0) {
            strncpy(unihan[idx].mandarin, value, 15);
        } else if (strcmp(property, "kCantonese") == 0) {
            strncpy(unihan[idx].cantonese, value, 15);
        } else if (strcmp(property, "kJapaneseOn") == 0) {
            strncpy(unihan[idx].japanese_on, value, 15);
        } else if (strcmp(property, "kJapaneseKun") == 0) {
            strncpy(unihan[idx].japanese_kun, value, 15);
        } else if (strcmp(property, "kKorean") == 0) {
            strncpy(unihan[idx].korean, value, 15);
        } else if (strcmp(property, "kVietnamese") == 0) {
            strncpy(unihan[idx].vietnamese, value, 15);
        }
    }
    
    fclose(file);
    
    *unihan_out = unihan;
    *count_out = count;
    
    printf("Parsed %zu Unihan entries from %s\n", count, filename);
    return true;
}

