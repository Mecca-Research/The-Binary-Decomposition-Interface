
#include "../include/training_tables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// Logic values
typedef enum {
    LOGIC_FALSE = 0,
    LOGIC_TRUE = 1,
    LOGIC_UNKNOWN = 2
} logic_value_t;

// Boolean operations
static uint8_t bool_and(uint8_t a, uint8_t b) { return a && b; }
static uint8_t bool_or(uint8_t a, uint8_t b) { return a || b; }
static uint8_t bool_not(uint8_t a) { return !a; }
static uint8_t bool_xor(uint8_t a, uint8_t b) { return a ^ b; }
static uint8_t bool_nand(uint8_t a, uint8_t b) { return !(a && b); }
static uint8_t bool_nor(uint8_t a, uint8_t b) { return !(a || b); }
static uint8_t bool_xnor(uint8_t a, uint8_t b) { return !(a ^ b); }
static uint8_t bool_implies(uint8_t a, uint8_t b) { return !a || b; }
static uint8_t bool_iff(uint8_t a, uint8_t b) { return a == b; }

// Three-valued logic operations (Kleene)
static uint8_t three_and(uint8_t a, uint8_t b) {
    if (a == LOGIC_FALSE || b == LOGIC_FALSE) return LOGIC_FALSE;
    if (a == LOGIC_UNKNOWN || b == LOGIC_UNKNOWN) return LOGIC_UNKNOWN;
    return LOGIC_TRUE;
}

static uint8_t three_or(uint8_t a, uint8_t b) {
    if (a == LOGIC_TRUE || b == LOGIC_TRUE) return LOGIC_TRUE;
    if (a == LOGIC_UNKNOWN || b == LOGIC_UNKNOWN) return LOGIC_UNKNOWN;
    return LOGIC_FALSE;
}

static uint8_t three_not(uint8_t a) {
    if (a == LOGIC_TRUE) return LOGIC_FALSE;
    if (a == LOGIC_FALSE) return LOGIC_TRUE;
    return LOGIC_UNKNOWN;
}

static uint8_t three_implies(uint8_t a, uint8_t b) {
    if (a == LOGIC_FALSE) return LOGIC_TRUE;
    if (b == LOGIC_TRUE) return LOGIC_TRUE;
    if (a == LOGIC_TRUE && b == LOGIC_FALSE) return LOGIC_FALSE;
    return LOGIC_UNKNOWN;
}

// Helper to create entry
static training_entry_t *create_logic_entry(uint32_t type, uint32_t difficulty,
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

int generate_logic_boolean_table(const char *filename) {
    printf("Generating boolean logic table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_LOGIC_BOOLEAN,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // Single variable operations (NOT)
    for (uint8_t a = 0; a <= 1; a++) {
        uint8_t input[2] = {0, a}; // op=0 for NOT
        uint8_t output = bool_not(a);
        
        training_entry_t *entry = create_logic_entry(
            ENTRY_LOGIC_BOOLEAN, 0,
            input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Two variable operations
    typedef uint8_t (*bool_op_t)(uint8_t, uint8_t);
    struct {
        uint8_t op_code;
        bool_op_t func;
        uint32_t difficulty;
    } ops[] = {
        {1, bool_and, 0},
        {2, bool_or, 0},
        {3, bool_xor, 1},
        {4, bool_nand, 1},
        {5, bool_nor, 1},
        {6, bool_xnor, 2},
        {7, bool_implies, 2},
        {8, bool_iff, 2}
    };
    
    for (size_t op_idx = 0; op_idx < sizeof(ops)/sizeof(ops[0]); op_idx++) {
        for (uint8_t a = 0; a <= 1; a++) {
            for (uint8_t b = 0; b <= 1; b++) {
                uint8_t input[3] = {ops[op_idx].op_code, a, b};
                uint8_t output = ops[op_idx].func(a, b);
                
                training_entry_t *entry = create_logic_entry(
                    ENTRY_LOGIC_BOOLEAN, ops[op_idx].difficulty,
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
    
    // Three variable compound expressions
    for (uint8_t a = 0; a <= 1; a++) {
        for (uint8_t b = 0; b <= 1; b++) {
            for (uint8_t c = 0; c <= 1; c++) {
                // (a AND b) OR c
                uint8_t input1[4] = {10, a, b, c};
                uint8_t output1 = bool_or(bool_and(a, b), c);
                training_entry_t *entry1 = create_logic_entry(
                    ENTRY_LOGIC_BOOLEAN, 3,
                    input1, sizeof(input1), &output1, sizeof(output1)
                );
                if (entry1) {
                    table_write_entry(fd, entry1);
                    free(entry1);
                    count++;
                }
                
                // (a OR b) AND c
                uint8_t input2[4] = {11, a, b, c};
                uint8_t output2 = bool_and(bool_or(a, b), c);
                training_entry_t *entry2 = create_logic_entry(
                    ENTRY_LOGIC_BOOLEAN, 3,
                    input2, sizeof(input2), &output2, sizeof(output2)
                );
                if (entry2) {
                    table_write_entry(fd, entry2);
                    free(entry2);
                    count++;
                }
                
                // a XOR b XOR c
                uint8_t input3[4] = {12, a, b, c};
                uint8_t output3 = bool_xor(bool_xor(a, b), c);
                training_entry_t *entry3 = create_logic_entry(
                    ENTRY_LOGIC_BOOLEAN, 4,
                    input3, sizeof(input3), &output3, sizeof(output3)
                );
                if (entry3) {
                    table_write_entry(fd, entry3);
                    free(entry3);
                    count++;
                }
            }
        }
    }
    
    // Four variable compound expressions
    for (uint8_t a = 0; a <= 1; a++) {
        for (uint8_t b = 0; b <= 1; b++) {
            for (uint8_t c = 0; c <= 1; c++) {
                for (uint8_t d = 0; d <= 1; d++) {
                    // (a AND b) OR (c AND d)
                    uint8_t input1[5] = {20, a, b, c, d};
                    uint8_t output1 = bool_or(bool_and(a, b), bool_and(c, d));
                    training_entry_t *entry1 = create_logic_entry(
                        ENTRY_LOGIC_BOOLEAN, 5,
                        input1, sizeof(input1), &output1, sizeof(output1)
                    );
                    if (entry1) {
                        table_write_entry(fd, entry1);
                        free(entry1);
                        count++;
                    }
                    
                    // (a OR b) AND (c OR d)
                    uint8_t input2[5] = {21, a, b, c, d};
                    uint8_t output2 = bool_and(bool_or(a, b), bool_or(c, d));
                    training_entry_t *entry2 = create_logic_entry(
                        ENTRY_LOGIC_BOOLEAN, 5,
                        input2, sizeof(input2), &output2, sizeof(output2)
                    );
                    if (entry2) {
                        table_write_entry(fd, entry2);
                        free(entry2);
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
    printf("Generated %u boolean logic entries\n", count);
    return 0;
}

int generate_logic_three_valued_table(const char *filename) {
    printf("Generating three-valued logic table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_LOGIC_THREE_VALUED,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // NOT operation
    for (uint8_t a = 0; a <= 2; a++) {
        uint8_t input[2] = {0, a};
        uint8_t output = three_not(a);
        
        training_entry_t *entry = create_logic_entry(
            ENTRY_LOGIC_THREE_VALUED, 2,
            input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // AND operation
    for (uint8_t a = 0; a <= 2; a++) {
        for (uint8_t b = 0; b <= 2; b++) {
            uint8_t input[3] = {1, a, b};
            uint8_t output = three_and(a, b);
            
            training_entry_t *entry = create_logic_entry(
                ENTRY_LOGIC_THREE_VALUED, 2,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // OR operation
    for (uint8_t a = 0; a <= 2; a++) {
        for (uint8_t b = 0; b <= 2; b++) {
            uint8_t input[3] = {2, a, b};
            uint8_t output = three_or(a, b);
            
            training_entry_t *entry = create_logic_entry(
                ENTRY_LOGIC_THREE_VALUED, 2,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // IMPLIES operation
    for (uint8_t a = 0; a <= 2; a++) {
        for (uint8_t b = 0; b <= 2; b++) {
            uint8_t input[3] = {3, a, b};
            uint8_t output = three_implies(a, b);
            
            training_entry_t *entry = create_logic_entry(
                ENTRY_LOGIC_THREE_VALUED, 3,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Three variable compound expressions
    for (uint8_t a = 0; a <= 2; a++) {
        for (uint8_t b = 0; b <= 2; b++) {
            for (uint8_t c = 0; c <= 2; c++) {
                // (a AND b) OR c
                uint8_t input1[4] = {10, a, b, c};
                uint8_t output1 = three_or(three_and(a, b), c);
                training_entry_t *entry1 = create_logic_entry(
                    ENTRY_LOGIC_THREE_VALUED, 4,
                    input1, sizeof(input1), &output1, sizeof(output1)
                );
                if (entry1) {
                    table_write_entry(fd, entry1);
                    free(entry1);
                    count++;
                }
                
                // (a OR b) AND c
                uint8_t input2[4] = {11, a, b, c};
                uint8_t output2 = three_and(three_or(a, b), c);
                training_entry_t *entry2 = create_logic_entry(
                    ENTRY_LOGIC_THREE_VALUED, 4,
                    input2, sizeof(input2), &output2, sizeof(output2)
                );
                if (entry2) {
                    table_write_entry(fd, entry2);
                    free(entry2);
                    count++;
                }
            }
        }
    }
    
    header.num_entries = count;
    header.total_size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    table_write_header(fd, &header);
    
    close(fd);
    printf("Generated %u three-valued logic entries\n", count);
    return 0;
}

int generate_logic_propositional_table(const char *filename) {
    printf("Generating propositional logic table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_LOGIC_PROPOSITIONAL,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // Tautologies (always true)
    for (uint8_t a = 0; a <= 1; a++) {
        for (uint8_t b = 0; b <= 1; b++) {
            // a OR NOT a (law of excluded middle)
            uint8_t input1[3] = {1, a, b};
            uint8_t output1 = 1; // always true
            training_entry_t *entry1 = create_logic_entry(
                ENTRY_LOGIC_PROPOSITIONAL, 3,
                input1, sizeof(input1), &output1, sizeof(output1)
            );
            if (entry1) {
                table_write_entry(fd, entry1);
                free(entry1);
                count++;
            }
            
            // (a IMPLIES b) OR (b IMPLIES a)
            uint8_t input2[3] = {2, a, b};
            uint8_t output2 = 1; // always true
            training_entry_t *entry2 = create_logic_entry(
                ENTRY_LOGIC_PROPOSITIONAL, 4,
                input2, sizeof(input2), &output2, sizeof(output2)
            );
            if (entry2) {
                table_write_entry(fd, entry2);
                free(entry2);
                count++;
            }
        }
    }
    
    // Contradictions (always false)
    for (uint8_t a = 0; a <= 1; a++) {
        // a AND NOT a
        uint8_t input[2] = {10, a};
        uint8_t output = 0; // always false
        training_entry_t *entry = create_logic_entry(
            ENTRY_LOGIC_PROPOSITIONAL, 3,
            input, sizeof(input), &output, sizeof(output)
        );
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Logical equivalences
    for (uint8_t a = 0; a <= 1; a++) {
        for (uint8_t b = 0; b <= 1; b++) {
            // De Morgan's law: NOT(a AND b) = (NOT a) OR (NOT b)
            uint8_t input1[3] = {20, a, b};
            uint8_t lhs1 = bool_not(bool_and(a, b));
            uint8_t rhs1 = bool_or(bool_not(a), bool_not(b));
            uint8_t output1 = (lhs1 == rhs1) ? 1 : 0;
            training_entry_t *entry1 = create_logic_entry(
                ENTRY_LOGIC_PROPOSITIONAL, 5,
                input1, sizeof(input1), &output1, sizeof(output1)
            );
            if (entry1) {
                table_write_entry(fd, entry1);
                free(entry1);
                count++;
            }
            
            // De Morgan's law: NOT(a OR b) = (NOT a) AND (NOT b)
            uint8_t input2[3] = {21, a, b};
            uint8_t lhs2 = bool_not(bool_or(a, b));
            uint8_t rhs2 = bool_and(bool_not(a), bool_not(b));
            uint8_t output2 = (lhs2 == rhs2) ? 1 : 0;
            training_entry_t *entry2 = create_logic_entry(
                ENTRY_LOGIC_PROPOSITIONAL, 5,
                input2, sizeof(input2), &output2, sizeof(output2)
            );
            if (entry2) {
                table_write_entry(fd, entry2);
                free(entry2);
                count++;
            }
            
            // Implication equivalence: (a IMPLIES b) = (NOT a OR b)
            uint8_t input3[3] = {22, a, b};
            uint8_t lhs3 = bool_implies(a, b);
            uint8_t rhs3 = bool_or(bool_not(a), b);
            uint8_t output3 = (lhs3 == rhs3) ? 1 : 0;
            training_entry_t *entry3 = create_logic_entry(
                ENTRY_LOGIC_PROPOSITIONAL, 4,
                input3, sizeof(input3), &output3, sizeof(output3)
            );
            if (entry3) {
                table_write_entry(fd, entry3);
                free(entry3);
                count++;
            }
        }
    }
    
    // Contingencies (sometimes true, sometimes false)
    for (uint8_t a = 0; a <= 1; a++) {
        for (uint8_t b = 0; b <= 1; b++) {
            for (uint8_t c = 0; c <= 1; c++) {
                // Complex contingent expression
                uint8_t input[4] = {30, a, b, c};
                uint8_t output = bool_and(bool_or(a, b), bool_implies(b, c));
                training_entry_t *entry = create_logic_entry(
                    ENTRY_LOGIC_PROPOSITIONAL, 6,
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
    
    header.num_entries = count;
    header.total_size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    table_write_header(fd, &header);
    
    close(fd);
    printf("Generated %u propositional logic entries\n", count);
    return 0;
}
