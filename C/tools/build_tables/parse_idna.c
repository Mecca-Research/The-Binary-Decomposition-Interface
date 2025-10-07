
#include "unicode_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parse IdnaMappingTable.txt
bool parse_idna_mappings(const char *filename, unicode_idna_mapping_t **mappings_out, size_t *count_out) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return false;
    }
    
    size_t capacity = 10000;
    unicode_idna_mapping_t *mappings = calloc(capacity, sizeof(unicode_idna_mapping_t));
    if (!mappings) {
        fclose(file);
        return false;
    }
    
    size_t count = 0;
    char line[4096];
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Parse line: codepoint ; status ; mapping
        char *fields[3];
        int field_count = 0;
        char *p = line;
        
        while (*p && field_count < 3) {
            while (*p == ' ' || *p == '\t') p++;
            fields[field_count++] = p;
            while (*p && *p != ';' && *p != '\n') p++;
            if (*p == ';') *p++ = '\0';
            else if (*p == '\n') *p = '\0';
        }
        
        if (field_count < 2) continue;
        
        if (count >= capacity) {
            capacity *= 2;
            unicode_idna_mapping_t *new_mappings = realloc(mappings, capacity * sizeof(unicode_idna_mapping_t));
            if (!new_mappings) {
                free(mappings);
                fclose(file);
                return false;
            }
            mappings = new_mappings;
        }
        
        // Parse codepoint or range
        char *range = strchr(fields[0], '.');
        if (range) {
            // Range: start..end
            uint32_t start = (uint32_t)strtoul(fields[0], NULL, 16);
            uint32_t end = (uint32_t)strtoul(range + 2, NULL, 16);
            
            // Add entry for each codepoint in range
            for (uint32_t cp = start; cp <= end && count < capacity; cp++) {
                mappings[count].codepoint = cp;
                mappings[count].status = 0; // TODO: Parse status
                mappings[count].mapping_len = 0;
                count++;
            }
        } else {
            mappings[count].codepoint = (uint32_t)strtoul(fields[0], NULL, 16);
            mappings[count].status = 0; // TODO: Parse status
            mappings[count].mapping_len = 0;
            count++;
        }
    }
    
    fclose(file);
    
    *mappings_out = mappings;
    *count_out = count;
    
    printf("Parsed %zu IDNA mappings from %s\n", count, filename);
    return true;
}

