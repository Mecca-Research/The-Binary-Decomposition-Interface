
#include "../include/training_tables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_NGRAM_SIZE 5
#define MAX_CODEPOINTS 100000

// Helper to create entry
static training_entry_t *create_language_entry(uint32_t type, uint32_t difficulty,
                                               const void *input, size_t input_size,
                                               const void *output, size_t output_size) {
    training_entry_t *entry = malloc(sizeof(training_entry_t) + input_size + output_size);
    if (!entry) return NULL;
    
    entry->entry_type = type;
    entry->difficulty = difficulty;
    entry->input_size = input_size;
    entry->output_size = output_size;
    
    memcpy(entry->data, input, input_size);
    memcpy(entry->data + input_size, output, output_size);
    
    return entry;
}

int generate_language_ngrams_table(const char *filename, const char *unicode_data) {
    printf("Generating n-grams table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_LANGUAGE_NGRAMS,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // Generate unigrams (character frequencies)
    // Sample common Unicode ranges
    struct {
        uint32_t start;
        uint32_t end;
        uint32_t difficulty;
    } ranges[] = {
        {0x0020, 0x007E, 0},  // Basic Latin
        {0x00A0, 0x00FF, 1},  // Latin-1 Supplement
        {0x0100, 0x017F, 2},  // Latin Extended-A
        {0x0370, 0x03FF, 3},  // Greek
        {0x0400, 0x04FF, 3},  // Cyrillic
        {0x4E00, 0x4EFF, 4},  // CJK Unified Ideographs (sample)
        {0x3040, 0x309F, 4},  // Hiragana
        {0x30A0, 0x30FF, 4},  // Katakana
        {0x0600, 0x06FF, 5},  // Arabic
        {0x0900, 0x097F, 5},  // Devanagari
    };
    
    for (size_t r = 0; r < sizeof(ranges)/sizeof(ranges[0]); r++) {
        for (uint32_t cp = ranges[r].start; cp <= ranges[r].end && cp < ranges[r].start + 100; cp++) {
            uint32_t input = cp;
            uint32_t output = 1; // frequency placeholder
            
            training_entry_t *entry = create_language_entry(
                ENTRY_LANGUAGE_NGRAMS, ranges[r].difficulty,
                &input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Generate bigrams (2-character sequences)
    for (size_t r = 0; r < sizeof(ranges)/sizeof(ranges[0]); r++) {
        uint32_t range_size = ranges[r].end - ranges[r].start + 1;
        if (range_size > 50) range_size = 50;
        
        for (uint32_t i = 0; i < range_size; i++) {
            for (uint32_t j = 0; j < range_size; j++) {
                uint32_t input[2] = {ranges[r].start + i, ranges[r].start + j};
                uint32_t output = 1; // frequency placeholder
                
                training_entry_t *entry = create_language_entry(
                    ENTRY_LANGUAGE_NGRAMS, ranges[r].difficulty + 1,
                    input, sizeof(input), &output, sizeof(output)
                );
                
                if (entry) {
                    table_write_entry(fd, entry);
                    free(entry);
                    count++;
                }
            }
        }
    }
    
    // Generate trigrams (3-character sequences)
    for (size_t r = 0; r < 3; r++) { // Only first 3 ranges for trigrams
        uint32_t range_size = ranges[r].end - ranges[r].start + 1;
        if (range_size > 20) range_size = 20;
        
        for (uint32_t i = 0; i < range_size; i++) {
            for (uint32_t j = 0; j < range_size; j++) {
                for (uint32_t k = 0; k < range_size; k++) {
                    uint32_t input[3] = {
                        ranges[r].start + i,
                        ranges[r].start + j,
                        ranges[r].start + k
                    };
                    uint32_t output = 1; // frequency placeholder
                    
                    training_entry_t *entry = create_language_entry(
                        ENTRY_LANGUAGE_NGRAMS, ranges[r].difficulty + 2,
                        input, sizeof(input), &output, sizeof(output)
                    );
                    
                    if (entry) {
                        table_write_entry(fd, entry);
                        free(entry);
                        count++;
                    }
                }
            }
        }
    }
    
    // Generate 4-grams (limited set)
    for (uint32_t i = 0; i < 10; i++) {
        for (uint32_t j = 0; j < 10; j++) {
            for (uint32_t k = 0; k < 10; k++) {
                for (uint32_t l = 0; l < 10; l++) {
                    uint32_t input[4] = {
                        0x0061 + i, // 'a' + i
                        0x0061 + j,
                        0x0061 + k,
                        0x0061 + l
                    };
                    uint32_t output = 1; // frequency placeholder
                    
                    training_entry_t *entry = create_language_entry(
                        ENTRY_LANGUAGE_NGRAMS, 4,
                        input, sizeof(input), &output, sizeof(output)
                    );
                    
                    if (entry) {
                        table_write_entry(fd, entry);
                        free(entry);
                        count++;
                    }
                }
            }
        }
    }
    
    header.num_entries = count;
    header.total_size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    table_write_header(fd, &header);
    
    close(fd);
    printf("Generated %u n-gram entries\n", count);
    return 0;
}

int generate_language_syntax_table(const char *filename, const char *unicode_data) {
    printf("Generating syntax patterns table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_LANGUAGE_SYNTAX,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // Case patterns (uppercase, lowercase, titlecase)
    for (uint32_t base = 0x0041; base <= 0x005A; base++) { // A-Z
        uint32_t input[3] = {base, base + 32, base}; // Upper, lower, upper
        uint32_t output = 1; // Pattern type: case alternation
        
        training_entry_t *entry = create_language_entry(
            ENTRY_LANGUAGE_SYNTAX, 1,
            input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Script transitions
    struct {
        uint32_t script1_start;
        uint32_t script2_start;
        uint32_t difficulty;
    } transitions[] = {
        {0x0041, 0x4E00, 5}, // Latin to CJK
        {0x0041, 0x0400, 4}, // Latin to Cyrillic
        {0x0041, 0x0370, 3}, // Latin to Greek
        {0x0041, 0x0600, 5}, // Latin to Arabic
        {0x4E00, 0x3040, 6}, // CJK to Hiragana
    };
    
    for (size_t t = 0; t < sizeof(transitions)/sizeof(transitions[0]); t++) {
        for (uint32_t i = 0; i < 10; i++) {
            uint32_t input[2] = {
                transitions[t].script1_start + i,
                transitions[t].script2_start + i
            };
            uint32_t output = 2; // Pattern type: script transition
            
            training_entry_t *entry = create_language_entry(
                ENTRY_LANGUAGE_SYNTAX, transitions[t].difficulty,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Grapheme clusters (combining characters)
    for (uint32_t base = 0x0041; base <= 0x005A; base++) {
        for (uint32_t combining = 0x0300; combining <= 0x0310; combining++) {
            uint32_t input[2] = {base, combining};
            uint32_t output = 3; // Pattern type: grapheme cluster
            
            training_entry_t *entry = create_language_entry(
                ENTRY_LANGUAGE_SYNTAX, 3,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Common grammatical patterns (simplified)
    // Pattern: determiner + noun
    for (uint32_t i = 0; i < 100; i++) {
        uint32_t input[2] = {1, 2}; // POS tags: DET, NOUN
        uint32_t output = 4; // Pattern type: valid phrase
        
        training_entry_t *entry = create_language_entry(
            ENTRY_LANGUAGE_SYNTAX, 4,
            input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Pattern: adjective + noun
    for (uint32_t i = 0; i < 100; i++) {
        uint32_t input[2] = {3, 2}; // POS tags: ADJ, NOUN
        uint32_t output = 4; // Pattern type: valid phrase
        
        training_entry_t *entry = create_language_entry(
            ENTRY_LANGUAGE_SYNTAX, 4,
            input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Pattern: subject + verb + object
    for (uint32_t i = 0; i < 100; i++) {
        uint32_t input[3] = {2, 4, 2}; // POS tags: NOUN, VERB, NOUN
        uint32_t output = 5; // Pattern type: valid sentence
        
        training_entry_t *entry = create_language_entry(
            ENTRY_LANGUAGE_SYNTAX, 5,
            input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    header.num_entries = count;
    header.total_size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    table_write_header(fd, &header);
    
    close(fd);
    printf("Generated %u syntax pattern entries\n", count);
    return 0;
}
