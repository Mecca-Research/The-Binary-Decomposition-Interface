
#include "include/training_tables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
    const char *output_dir = "C/data/training";
    
    if (argc > 1) {
        output_dir = argv[1];
    }
    
    // Create output directory
    mkdir(output_dir, 0755);
    
    printf("=== BDI Training Table Generator ===\n");
    printf("Output directory: %s\n\n", output_dir);
    
    char filename[1024];
    int result;
    
    // Generate math tables
    printf("--- Generating Math Tables ---\n");
    
    snprintf(filename, sizeof(filename), "%s/math_arithmetic.dat", output_dir);
    result = generate_math_arithmetic_table(filename);
    if (result < 0) {
        fprintf(stderr, "Failed to generate arithmetic table\n");
        return 1;
    }
    
    snprintf(filename, sizeof(filename), "%s/math_modular.dat", output_dir);
    result = generate_math_modular_table(filename);
    if (result < 0) {
        fprintf(stderr, "Failed to generate modular arithmetic table\n");
        return 1;
    }
    
    snprintf(filename, sizeof(filename), "%s/math_number_theory.dat", output_dir);
    result = generate_math_number_theory_table(filename);
    if (result < 0) {
        fprintf(stderr, "Failed to generate number theory table\n");
        return 1;
    }
    
    snprintf(filename, sizeof(filename), "%s/math_sequences.dat", output_dir);
    result = generate_math_sequences_table(filename);
    if (result < 0) {
        fprintf(stderr, "Failed to generate sequences table\n");
        return 1;
    }
    
    // Generate logic tables
    printf("\n--- Generating Logic Tables ---\n");
    
    snprintf(filename, sizeof(filename), "%s/logic_boolean.dat", output_dir);
    result = generate_logic_boolean_table(filename);
    if (result < 0) {
        fprintf(stderr, "Failed to generate boolean logic table\n");
        return 1;
    }
    
    snprintf(filename, sizeof(filename), "%s/logic_three_valued.dat", output_dir);
    result = generate_logic_three_valued_table(filename);
    if (result < 0) {
        fprintf(stderr, "Failed to generate three-valued logic table\n");
        return 1;
    }
    
    snprintf(filename, sizeof(filename), "%s/logic_propositional.dat", output_dir);
    result = generate_logic_propositional_table(filename);
    if (result < 0) {
        fprintf(stderr, "Failed to generate propositional logic table\n");
        return 1;
    }
    
    // Generate language tables
    printf("\n--- Generating Language Tables ---\n");
    
    snprintf(filename, sizeof(filename), "%s/language_ngrams.dat", output_dir);
    result = generate_language_ngrams_table(filename, "C/compiler/AIBase/data/unicode_basic.dat");
    if (result < 0) {
        fprintf(stderr, "Failed to generate n-grams table\n");
        return 1;
    }
    
    snprintf(filename, sizeof(filename), "%s/language_syntax.dat", output_dir);
    result = generate_language_syntax_table(filename, "C/compiler/AIBase/data/unicode_props.dat");
    if (result < 0) {
        fprintf(stderr, "Failed to generate syntax table\n");
        return 1;
    }
    
    // Generate code tables
    printf("\n--- Generating Code Tables ---\n");
    
    snprintf(filename, sizeof(filename), "%s/code_ast.dat", output_dir);
    result = generate_code_ast_table(filename);
    if (result < 0) {
        fprintf(stderr, "Failed to generate AST table\n");
        return 1;
    }
    
    snprintf(filename, sizeof(filename), "%s/code_idioms.dat", output_dir);
    result = generate_code_idioms_table(filename);
    if (result < 0) {
        fprintf(stderr, "Failed to generate idioms table\n");
        return 1;
    }
    
    snprintf(filename, sizeof(filename), "%s/code_bugs.dat", output_dir);
    result = generate_code_bugs_table(filename);
    if (result < 0) {
        fprintf(stderr, "Failed to generate bugs table\n");
        return 1;
    }
    
    printf("\n=== All tables generated successfully ===\n");
    
    // Validate all tables
    printf("\n--- Validating Tables ---\n");
    
    const char *tables[] = {
        "math_arithmetic.dat",
        "math_modular.dat",
        "math_number_theory.dat",
        "math_sequences.dat",
        "logic_boolean.dat",
        "logic_three_valued.dat",
        "logic_propositional.dat",
        "language_ngrams.dat",
        "language_syntax.dat",
        "code_ast.dat",
        "code_idioms.dat",
        "code_bugs.dat"
    };
    
    for (size_t i = 0; i < sizeof(tables) / sizeof(tables[0]); i++) {
        snprintf(filename, sizeof(filename), "%s/%s", output_dir, tables[i]);
        if (table_validate_file(filename) < 0) {
            fprintf(stderr, "Validation failed for %s\n", tables[i]);
            return 1;
        }
    }
    
    printf("\n=== All tables validated successfully ===\n");
    
    return 0;
}
