// ===================================================================
// DESC: The main driver for the BCI compiler and virtual machine.
//       This file integrates all components to compile and run source
//       code.
// ===================================================================

#include <stdio.h>
#include <stdlib.h>
#include "kernel/c23_compat.h"  // C23 compatibility (nullptr, etc.)

#include "bci_lexer.h"
#include "bci_parser.h"
#include "bci_analyzer.h"
#include "bci_chunk.h"
#include "bci_codegen.h"
#include "bci_vm.h"

// This function encapsulates the entire compiler and VM pipeline.
static InterpretResult compile_and_run(const char* source) {
    // 1. Lexer
    Lexer lexer;
    lexer_init(&lexer, source);

    // 2. Parser
    Parser parser;
    parser_init(&parser, &lexer);
    AstNode* program = parser_parse(&parser);
    if (program == nullptr || parser.had_error) {
        fprintf(stderr, "Compilation failed: Syntax Error.\n");
        if (program) ast_free_node(program);
        return INTERPRET_COMPILE_ERROR;
    }

    // 3. Semantic Analyzer
    Analyzer analyzer;
    analyzer_init(&analyzer);
    bool is_semantically_valid = analyzer_analyze(&analyzer, program);
    if (!is_semantically_valid) {
        fprintf(stderr, "Compilation failed: Semantic Error.\n");
        analyzer_free(&analyzer);
        ast_free_node(program);
        return INTERPRET_COMPILE_ERROR;
    }
    
    // 4. Code Generator
    Chunk chunk;
    chunk_init(&chunk);
    CodeGenerator codegen;
    codegen_init(&codegen, &chunk);
    bool compilation_success = codegen_generate(&codegen, program);
    if (!compilation_success) {
        fprintf(stderr, "Compilation failed: Code Generation Error.\n");
        codegen_free(&codegen);
        chunk_free(&chunk);
        analyzer_free(&analyzer);
        ast_free_node(program);
        return INTERPRET_COMPILE_ERROR;
    }

    // 5. Virtual Machine
    VM vm;
    vm_init(&vm);
    InterpretResult result = vm_interpret(&vm, &chunk);

    // 6. Cleanup
    vm_free(&vm);
    codegen_free(&codegen);
    chunk_free(&chunk);
    analyzer_free(&analyzer);
    ast_free_node(program);

    return result;
}

int main(int argc, const char* argv[]) {
    // Example program to compile and run.
    const char* source_code = "10 * (2 + 3);";

    printf("Compiling and running source:\n%s\n\n", source_code);

    InterpretResult result = compile_and_run(source_code);
    
    printf("\nInterpretation finished with status: %d\n", result);

    return result == INTERPRET_OK ? 0 : 1;
}
