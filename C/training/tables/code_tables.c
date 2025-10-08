
#include "../include/training_tables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

// AST node types
typedef enum {
    AST_LITERAL = 1,
    AST_VARIABLE = 2,
    AST_BINARY_OP = 3,
    AST_UNARY_OP = 4,
    AST_FUNCTION_CALL = 5,
    AST_IF_STMT = 6,
    AST_WHILE_LOOP = 7,
    AST_FOR_LOOP = 8,
    AST_ASSIGNMENT = 9,
    AST_RETURN = 10
} ast_node_type_t;

// Helper to create entry
static training_entry_t *create_code_entry(uint32_t type, uint32_t difficulty,
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

int generate_code_ast_table(const char *filename) {
    printf("Generating AST patterns table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_CODE_AST,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // Simple expression patterns
    // Binary operations: a + b, a - b, a * b, a / b
    uint8_t binary_ops[] = {'+', '-', '*', '/', '%', '&', '|', '^', '<', '>', '='};
    
    for (size_t op_idx = 0; op_idx < sizeof(binary_ops); op_idx++) {
        for (uint32_t i = 0; i < 50; i++) {
            uint8_t input[3] = {AST_BINARY_OP, binary_ops[op_idx], (uint8_t)i};
            uint8_t output = 1; // Valid pattern
            
            training_entry_t *entry = create_code_entry(
                ENTRY_CODE_AST, op_idx / 3,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Unary operations: -a, !a, ~a, ++a, --a
    uint8_t unary_ops[] = {'-', '!', '~', '+', '-'};
    
    for (size_t op_idx = 0; op_idx < sizeof(unary_ops); op_idx++) {
        for (uint32_t i = 0; i < 50; i++) {
            uint8_t input[2] = {AST_UNARY_OP, unary_ops[op_idx]};
            uint8_t output = 1; // Valid pattern
            
            training_entry_t *entry = create_code_entry(
                ENTRY_CODE_AST, 1,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Function call patterns
    for (uint32_t num_args = 0; num_args <= 5; num_args++) {
        for (uint32_t i = 0; i < 100; i++) {
            uint8_t input[2] = {AST_FUNCTION_CALL, num_args};
            uint8_t output = 1; // Valid pattern
            
            training_entry_t *entry = create_code_entry(
                ENTRY_CODE_AST, num_args,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Control flow patterns
    // If statement
    for (uint32_t has_else = 0; has_else <= 1; has_else++) {
        for (uint32_t i = 0; i < 100; i++) {
            uint8_t input[2] = {AST_IF_STMT, has_else};
            uint8_t output = 1; // Valid pattern
            
            training_entry_t *entry = create_code_entry(
                ENTRY_CODE_AST, 2 + has_else,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // While loop
    for (uint32_t i = 0; i < 100; i++) {
        uint8_t input = AST_WHILE_LOOP;
        uint8_t output = 1; // Valid pattern
        
        training_entry_t *entry = create_code_entry(
            ENTRY_CODE_AST, 3,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // For loop
    for (uint32_t i = 0; i < 100; i++) {
        uint8_t input = AST_FOR_LOOP;
        uint8_t output = 1; // Valid pattern
        
        training_entry_t *entry = create_code_entry(
            ENTRY_CODE_AST, 4,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Assignment patterns
    for (uint32_t i = 0; i < 100; i++) {
        uint8_t input = AST_ASSIGNMENT;
        uint8_t output = 1; // Valid pattern
        
        training_entry_t *entry = create_code_entry(
            ENTRY_CODE_AST, 1,
            &input, sizeof(input), &output, sizeof(output)
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
    printf("Generated %u AST pattern entries\n", count);
    return 0;
}

int generate_code_idioms_table(const char *filename) {
    printf("Generating code idioms table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_CODE_IDIOMS,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // Loop idioms
    // Pattern 1: for (i = 0; i < n; i++)
    for (uint32_t n = 1; n <= 100; n++) {
        uint8_t input[2] = {1, (uint8_t)n}; // Idiom type 1, bound n
        uint8_t output = 1; // Valid idiom
        
        training_entry_t *entry = create_code_entry(
            ENTRY_CODE_IDIOMS, 2,
            input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Pattern 2: while (condition)
    for (uint32_t i = 0; i < 100; i++) {
        uint8_t input = 2; // Idiom type 2
        uint8_t output = 1; // Valid idiom
        
        training_entry_t *entry = create_code_entry(
            ENTRY_CODE_IDIOMS, 2,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Pattern 3: do-while loop
    for (uint32_t i = 0; i < 100; i++) {
        uint8_t input = 3; // Idiom type 3
        uint8_t output = 1; // Valid idiom
        
        training_entry_t *entry = create_code_entry(
            ENTRY_CODE_IDIOMS, 3,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Conditional idioms
    // Pattern 4: if-else chain
    for (uint32_t num_branches = 2; num_branches <= 5; num_branches++) {
        for (uint32_t i = 0; i < 50; i++) {
            uint8_t input[2] = {4, num_branches};
            uint8_t output = 1; // Valid idiom
            
            training_entry_t *entry = create_code_entry(
                ENTRY_CODE_IDIOMS, 3 + num_branches / 2,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Pattern 5: switch statement
    for (uint32_t num_cases = 2; num_cases <= 10; num_cases++) {
        for (uint32_t i = 0; i < 50; i++) {
            uint8_t input[2] = {5, num_cases};
            uint8_t output = 1; // Valid idiom
            
            training_entry_t *entry = create_code_entry(
                ENTRY_CODE_IDIOMS, 4,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Error handling idioms
    // Pattern 6: try-catch
    for (uint32_t i = 0; i < 100; i++) {
        uint8_t input = 6;
        uint8_t output = 1; // Valid idiom
        
        training_entry_t *entry = create_code_entry(
            ENTRY_CODE_IDIOMS, 5,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Pattern 7: error return codes
    for (uint32_t i = 0; i < 100; i++) {
        uint8_t input = 7;
        uint8_t output = 1; // Valid idiom
        
        training_entry_t *entry = create_code_entry(
            ENTRY_CODE_IDIOMS, 4,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Resource management idioms
    // Pattern 8: RAII (Resource Acquisition Is Initialization)
    for (uint32_t i = 0; i < 100; i++) {
        uint8_t input = 8;
        uint8_t output = 1; // Valid idiom
        
        training_entry_t *entry = create_code_entry(
            ENTRY_CODE_IDIOMS, 6,
            &input, sizeof(input), &output, sizeof(output)
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
    printf("Generated %u code idiom entries\n", count);
    return 0;
}

int generate_code_bugs_table(const char *filename) {
    printf("Generating bug patterns table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_CODE_BUGS,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // Off-by-one errors
    for (uint32_t bound = 1; bound <= 100; bound++) {
        // Correct: i < n
        uint8_t input1[3] = {1, (uint8_t)bound, 0};
        uint8_t output1 = 0; // No bug
        training_entry_t *entry1 = create_code_entry(
            ENTRY_CODE_BUGS, 3,
            input1, sizeof(input1), &output1, sizeof(output1)
        );
        if (entry1) {
            table_write_entry(fd, entry1);
            free(entry1);
            count++;
        }
        
        // Bug: i <= n (off-by-one)
        uint8_t input2[3] = {1, (uint8_t)bound, 1};
        uint8_t output2 = 1; // Bug present
        training_entry_t *entry2 = create_code_entry(
            ENTRY_CODE_BUGS, 3,
            input2, sizeof(input2), &output2, sizeof(output2)
        );
        if (entry2) {
            table_write_entry(fd, entry2);
            free(entry2);
            count++;
        }
    }
    
    // Null pointer dereference
    for (uint32_t i = 0; i < 100; i++) {
        // With null check
        uint8_t input1[2] = {2, 0};
        uint8_t output1 = 0; // No bug
        training_entry_t *entry1 = create_code_entry(
            ENTRY_CODE_BUGS, 4,
            input1, sizeof(input1), &output1, sizeof(output1)
        );
        if (entry1) {
            table_write_entry(fd, entry1);
            free(entry1);
            count++;
        }
        
        // Without null check
        uint8_t input2[2] = {2, 1};
        uint8_t output2 = 1; // Bug present
        training_entry_t *entry2 = create_code_entry(
            ENTRY_CODE_BUGS, 4,
            input2, sizeof(input2), &output2, sizeof(output2)
        );
        if (entry2) {
            table_write_entry(fd, entry2);
            free(entry2);
            count++;
        }
    }
    
    // Buffer overflow
    for (uint32_t size = 1; size <= 100; size++) {
        // Safe access
        uint8_t input1[3] = {3, (uint8_t)size, (uint8_t)(size - 1)};
        uint8_t output1 = 0; // No bug
        training_entry_t *entry1 = create_code_entry(
            ENTRY_CODE_BUGS, 5,
            input1, sizeof(input1), &output1, sizeof(output1)
        );
        if (entry1) {
            table_write_entry(fd, entry1);
            free(entry1);
            count++;
        }
        
        // Overflow
        uint8_t input2[3] = {3, (uint8_t)size, (uint8_t)size};
        uint8_t output2 = 1; // Bug present
        training_entry_t *entry2 = create_code_entry(
            ENTRY_CODE_BUGS, 5,
            input2, sizeof(input2), &output2, sizeof(output2)
        );
        if (entry2) {
            table_write_entry(fd, entry2);
            free(entry2);
            count++;
        }
    }
    
    // Memory leak
    for (uint32_t i = 0; i < 100; i++) {
        // With free
        uint8_t input1[2] = {4, 0};
        uint8_t output1 = 0; // No bug
        training_entry_t *entry1 = create_code_entry(
            ENTRY_CODE_BUGS, 5,
            input1, sizeof(input1), &output1, sizeof(output1)
        );
        if (entry1) {
            table_write_entry(fd, entry1);
            free(entry1);
            count++;
        }
        
        // Without free
        uint8_t input2[2] = {4, 1};
        uint8_t output2 = 1; // Bug present
        training_entry_t *entry2 = create_code_entry(
            ENTRY_CODE_BUGS, 5,
            input2, sizeof(input2), &output2, sizeof(output2)
        );
        if (entry2) {
            table_write_entry(fd, entry2);
            free(entry2);
            count++;
        }
    }
    
    // Type mismatch
    for (uint32_t i = 0; i < 100; i++) {
        // Correct types
        uint8_t input1[3] = {5, 1, 1}; // int, int
        uint8_t output1 = 0; // No bug
        training_entry_t *entry1 = create_code_entry(
            ENTRY_CODE_BUGS, 4,
            input1, sizeof(input1), &output1, sizeof(output1)
        );
        if (entry1) {
            table_write_entry(fd, entry1);
            free(entry1);
            count++;
        }
        
        // Type mismatch
        uint8_t input2[3] = {5, 1, 2}; // int, float
        uint8_t output2 = 1; // Bug present
        training_entry_t *entry2 = create_code_entry(
            ENTRY_CODE_BUGS, 4,
            input2, sizeof(input2), &output2, sizeof(output2)
        );
        if (entry2) {
            table_write_entry(fd, entry2);
            free(entry2);
            count++;
        }
    }
    
    // Logic errors (incorrect condition)
    for (uint32_t i = 0; i < 100; i++) {
        // Correct condition
        uint8_t input1[2] = {6, 0};
        uint8_t output1 = 0; // No bug
        training_entry_t *entry1 = create_code_entry(
            ENTRY_CODE_BUGS, 6,
            input1, sizeof(input1), &output1, sizeof(output1)
        );
        if (entry1) {
            table_write_entry(fd, entry1);
            free(entry1);
            count++;
        }
        
        // Incorrect condition
        uint8_t input2[2] = {6, 1};
        uint8_t output2 = 1; // Bug present
        training_entry_t *entry2 = create_code_entry(
            ENTRY_CODE_BUGS, 6,
            input2, sizeof(input2), &output2, sizeof(output2)
        );
        if (entry2) {
            table_write_entry(fd, entry2);
            free(entry2);
            count++;
        }
    }
    
    header.num_entries = count;
    header.total_size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    table_write_header(fd, &header);
    
    close(fd);
    printf("Generated %u bug pattern entries\n", count);
    return 0;
}
