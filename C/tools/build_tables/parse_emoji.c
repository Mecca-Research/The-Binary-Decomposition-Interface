
#include "unicode_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parse emoji-test.txt
bool parse_emoji_test(const char *filename, unicode_emoji_t **emoji_out, size_t *count_out) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Failed to open %s\n", filename);
        return false;
    }
    
    size_t capacity = 5000;
    unicode_emoji_t *emoji = calloc(capacity, sizeof(unicode_emoji_t));
    if (!emoji) {
        fclose(file);
        return false;
    }
    
    size_t count = 0;
    char line[4096];
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;
        
        // Parse emoji sequence
        char *semicolon = strchr(line, ';');
        if (!semicolon) continue;
        
        *semicolon = '\0';
        char *sequence_str = line;
        
        if (count >= capacity) {
            capacity *= 2;
            unicode_emoji_t *new_emoji = realloc(emoji, capacity * sizeof(unicode_emoji_t));
            if (!new_emoji) {
                free(emoji);
                fclose(file);
                return false;
            }
            emoji = new_emoji;
        }
        
        // Parse code points
        emoji[count].sequence_len = 0;
        char *token = strtok(sequence_str, " ");
        while (token && emoji[count].sequence_len < 10) {
            emoji[count].sequence[emoji[count].sequence_len++] = 
                (uint32_t)strtoul(token, NULL, 16);
            token = strtok(NULL, " ");
        }
        
        if (emoji[count].sequence_len > 0) {
            emoji[count].codepoint = emoji[count].sequence[0];
            emoji[count].is_zwj_sequence = (emoji[count].sequence_len > 1);
            count++;
        }
    }
    
    fclose(file);
    
    *emoji_out = emoji;
    *count_out = count;
    
    printf("Parsed %zu emoji from %s\n", count, filename);
    return true;
}

