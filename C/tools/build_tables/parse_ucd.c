
#include "unicode_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Parse hex value
static uint32_t parse_hex(const char *str) {
    return (uint32_t)strtoul(str, NULL, 16);
}

// Parse general category
static uint8_t parse_general_category(const char *str) {
    if (strcmp(str, "Lu") == 0) return GC_Lu;
    if (strcmp(str, "Ll") == 0) return GC_Ll;
    if (strcmp(str, "Lt") == 0) return GC_Lt;
    if (strcmp(str, "Lm") == 0) return GC_Lm;
    if (strcmp(str, "Lo") == 0) return GC_Lo;
    if (strcmp(str, "Mn") == 0) return GC_Mn;
    if (strcmp(str, "Mc") == 0) return GC_Mc;
    if (strcmp(str, "Me") == 0) return GC_Me;
    if (strcmp(str, "Nd") == 0) return GC_Nd;
    if (strcmp(str, "Nl") == 0) return GC_Nl;
    if (strcmp(str, "No") == 0) return GC_No;
    if (strcmp(str, "Pc") == 0) return GC_Pc;
    if (strcmp(str, "Pd") == 0) return GC_Pd;
    if (strcmp(str, "Ps") == 0) return GC_Ps;
    if (strcmp(str, "Pe") == 0) return GC_Pe;
    if (strcmp(str, "Pi") == 0) return GC_Pi;
    if (strcmp(str, "Pf") == 0) return GC_Pf;
    if (strcmp(str, "Po") == 0) return GC_Po;
    if (strcmp(str, "Sm") == 0) return GC_Sm;
    if (strcmp(str, "Sc") == 0) return GC_Sc;
    if (strcmp(str, "Sk") == 0) return GC_Sk;
    if (strcmp(str, "So") == 0) return GC_So;
    if (strcmp(str, "Zs") == 0) return GC_Zs;
    if (strcmp(str, "Zl") == 0) return GC_Zl;
    if (strcmp(str, "Zp") == 0) return GC_Zp;
    if (strcmp(str, "Cc") == 0) return GC_Cc;
    if (strcmp(str, "Cf") == 0) return GC_Cf;
    if (strcmp(str, "Cs") == 0) return GC_Cs;
    if (strcmp(str, "Co") == 0) return GC_Co;
    if (strcmp(str, "Cn") == 0) return GC_Cn;
    return GC_Cn; // Default to unassigned
}

// Parse UnicodeData.txt line
static bool parse_unicode_data_line(const char *line, unicode_char_props_t *props) {
    char fields[15][256];
    int field_count = 0;
    const char *p = line;
    
    // Split line by semicolons
    while (*p && field_count < 15) {
        char *field = fields[field_count];
        int len = 0;
        while (*p && *p != ';' && len < 255) {
            field[len++] = *p++;
        }
        field[len] = '\0';
        field_count++;
        if (*p == ';') p++;
    }
    
    // UnicodeData.txt has 15 fields, but the last field (field 15) is often empty
    // So we accept lines with 14 or 15 fields
    if (field_count < 14) return false;
    
    // Parse fields
    props->codepoint = parse_hex(fields[0]);
    props->general_category = parse_general_category(fields[2]);
    props->combining_class = (uint8_t)atoi(fields[3]);
    
    // Parse case mappings
    props->uppercase = fields[12][0] ? parse_hex(fields[12]) : props->codepoint;
    props->lowercase = fields[13][0] ? parse_hex(fields[13]) : props->codepoint;
    props->titlecase = fields[14][0] ? parse_hex(fields[14]) : props->codepoint;
    
    // Parse decomposition
    props->decomp_len = 0;
    if (fields[5][0]) {
        const char *decomp = fields[5];
        // Skip decomposition type tag if present
        if (decomp[0] == '<') {
            while (*decomp && *decomp != '>') decomp++;
            if (*decomp == '>') decomp++;
            while (*decomp == ' ') decomp++;
        }
        
        // Parse decomposition mapping
        while (*decomp && props->decomp_len < 18) {
            props->decomp_mapping[props->decomp_len++] = parse_hex(decomp);
            while (*decomp && *decomp != ' ') decomp++;
            while (*decomp == ' ') decomp++;
        }
    }
    
    return true;
}

// Parse UnicodeData.txt
bool parse_unicode_data(const char *filename, unicode_char_props_t **props_out, size_t *count_out) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return false;
    }
    
    size_t capacity = 150000; // Unicode 17.0 has ~149,813 assigned characters
    unicode_char_props_t *props = calloc(capacity, sizeof(unicode_char_props_t));
    if (!props) {
        fclose(file);
        return false;
    }
    
    size_t count = 0;
    char line[4096];
    unicode_char_props_t range_start;
    bool in_range = false;
    
    while (fgets(line, sizeof(line), file)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        
        // Skip empty lines and comments
        if (line[0] == '\0' || line[0] == '#') continue;
        
        // Check capacity
        if (count >= capacity) {
            capacity *= 2;
            unicode_char_props_t *new_props = realloc(props, capacity * sizeof(unicode_char_props_t));
            if (!new_props) {
                free(props);
                fclose(file);
                return false;
            }
            props = new_props;
        }
        
        // Parse the line
        unicode_char_props_t current;
        if (!parse_unicode_data_line(line, &current)) {
            continue;
        }
        
        // Extract name field (field 1) to check for range markers
        char name[256];
        const char *p = line;
        // Skip codepoint field (field 0)
        while (*p && *p != ';') p++;
        if (*p == ';') p++;
        // Extract name field (field 1)
        int name_len = 0;
        while (*p && *p != ';' && name_len < 255) {
            name[name_len++] = *p++;
        }
        name[name_len] = '\0';
        
        // Check for range markers
        if (strstr(name, "First>")) {
            // Start of a range
            range_start = current;
            in_range = true;
            props[count++] = current;
        } else if (strstr(name, "Last>") && in_range) {
            // End of a range - expand all code points between First and Last
            uint32_t start_cp = range_start.codepoint;
            uint32_t end_cp = current.codepoint;
            
            // Generate entries for all code points in the range (start+1 to end inclusive)
            for (uint32_t cp = start_cp + 1; cp <= end_cp; cp++) {
                // Check capacity for large ranges
                if (count >= capacity) {
                    capacity *= 2;
                    unicode_char_props_t *new_props = realloc(props, capacity * sizeof(unicode_char_props_t));
                    if (!new_props) {
                        free(props);
                        fclose(file);
                        return false;
                    }
                    props = new_props;
                }
                
                // Copy properties from range_start and update codepoint
                props[count] = range_start;
                props[count].codepoint = cp;
                count++;
            }
            
            in_range = false;
            printf("Expanded range U+%04X..U+%04X (%u code points)\n", 
                   start_cp, end_cp, end_cp - start_cp + 1);
        } else {
            // Regular entry (not part of a range)
            props[count++] = current;
        }
    }
    
    fclose(file);
    
    *props_out = props;
    *count_out = count;
    
    printf("Parsed %zu characters from %s\n", count, filename);
    return true;
}

