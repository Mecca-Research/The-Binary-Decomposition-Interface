
#ifndef TRAINING_TABLES_H
#define TRAINING_TABLES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Training entry types
typedef enum {
    ENTRY_MATH_ARITHMETIC = 0,
    ENTRY_MATH_MODULAR = 1,
    ENTRY_MATH_NUMBER_THEORY = 2,
    ENTRY_MATH_SEQUENCES = 3,
    ENTRY_LOGIC_BOOLEAN = 4,
    ENTRY_LOGIC_THREE_VALUED = 5,
    ENTRY_LOGIC_PROPOSITIONAL = 6,
    ENTRY_LANGUAGE_NGRAMS = 7,
    ENTRY_LANGUAGE_SYNTAX = 8,
    ENTRY_CODE_AST = 9,
    ENTRY_CODE_IDIOMS = 10,
    ENTRY_CODE_BUGS = 11
} training_entry_type_t;

// Training entry structure
typedef struct {
    uint32_t entry_type;      // Type of training entry
    uint32_t difficulty;      // Difficulty level (0-7 for phases)
    uint32_t input_size;      // Size of input data
    uint32_t output_size;     // Size of expected output
    uint8_t data[];           // Variable-length input and output data
} training_entry_t;

// Table file header
typedef struct {
    uint32_t magic;           // Magic number: 0x42444954 ('BDIT')
    uint32_t version;         // Format version
    uint32_t entry_type;      // Type of entries in this file
    uint32_t num_entries;     // Number of entries
    uint64_t total_size;      // Total file size
    uint32_t checksum;        // CRC32 checksum
    uint32_t reserved[10];    // Reserved for future use
} table_header_t;

// Common table operations
int table_write_header(int fd, const table_header_t *header);
int table_read_header(int fd, table_header_t *header);
int table_write_entry(int fd, const training_entry_t *entry);
int table_read_entry(int fd, training_entry_t **entry);
uint32_t table_calculate_checksum(const void *data, size_t size);
int table_validate_file(const char *filename);

// Math table generators
int generate_math_arithmetic_table(const char *filename);
int generate_math_modular_table(const char *filename);
int generate_math_number_theory_table(const char *filename);
int generate_math_sequences_table(const char *filename);

// Logic table generators
int generate_logic_boolean_table(const char *filename);
int generate_logic_three_valued_table(const char *filename);
int generate_logic_propositional_table(const char *filename);

// Language table generators
int generate_language_ngrams_table(const char *filename, const char *unicode_data);
int generate_language_syntax_table(const char *filename, const char *unicode_data);

// Code table generators
int generate_code_ast_table(const char *filename);
int generate_code_idioms_table(const char *filename);
int generate_code_bugs_table(const char *filename);

#endif // TRAINING_TABLES_H
