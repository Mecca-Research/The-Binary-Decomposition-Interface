// Simplified DSL test - tests lexer, parser, and AST only
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../DSL/ml_dsl_lexer.h"
#include "../DSL/ml_dsl_parser.h"
#include "../DSL/ml_dsl_ast.h"

// Test lexer tokenization
static void test_lexer(void) {
    printf("Testing DSL lexer...\n");
    
    const char* source = "ml model test { type: linear_regression; }";
    Lexer* lexer = lexer_create(source);
    assert(lexer != NULL);
    
    Token* tokens = NULL;
    size_t token_count = 0;
    
    Token token;
    do {
        token = lexer_next_token(lexer);
        tokens = realloc(tokens, sizeof(Token) * (token_count + 1));
        tokens[token_count++] = token;
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);
    
    assert(token_count > 0);
    assert(tokens[0].type == TOKEN_ML); // "ml"
    assert(tokens[1].type == TOKEN_MODEL); // "model"
    
    free(tokens);
    lexer_destroy(lexer);
    printf("✓ DSL lexer test passed\n");
}

// Test parser with token array
static void test_parser(void) {
    printf("Testing DSL parser...\n");
    
    const char* source = "ml model test { type: linear_regression; }";
    Lexer* lexer = lexer_create(source);
    
    // Collect all tokens
    Token* tokens = NULL;
    size_t token_count = 0;
    Token token;
    do {
        token = lexer_next_token(lexer);
        tokens = realloc(tokens, sizeof(Token) * (token_count + 1));
        tokens[token_count++] = token;
    } while (token.type != TOKEN_EOF && token.type != TOKEN_ERROR);
    
    // Create parser with tokens
    Parser* parser = parser_create(tokens, token_count);
    assert(parser != NULL);
    
    ASTNode* ast = parser_parse(parser);
    assert(ast != NULL);
    assert(ast->type == AST_PROGRAM);
    
    ast_node_destroy(ast);
    parser_destroy(parser);
    free(tokens);
    lexer_destroy(lexer);
    printf("✓ DSL parser test passed\n");
}

// Test AST parameter creation
static void test_ast_parameters(void) {
    printf("Testing DSL AST parameters...\n");
    
    // Create parameter list
    ASTParameterList* params = ast_parameter_list_create();
    assert(params != NULL);
    
    // Add a numeric parameter
    ASTParameter* param1 = ast_parameter_create("learning_rate");
    assert(param1 != NULL);
    param1->value_type = PARAM_NUMBER;
    param1->value.number = 0.01;
    ast_parameter_list_add(params, param1);
    
    // Add a string parameter
    ASTParameter* param2 = ast_parameter_create("kernel");
    assert(param2 != NULL);
    param2->value_type = PARAM_STRING;
    param2->value.string = strdup("rbf");
    ast_parameter_list_add(params, param2);
    
    assert(params->count == 2);
    
    // Retrieve parameters
    ASTParameter* retrieved = ast_parameter_list_get(params, "learning_rate");
    assert(retrieved != NULL);
    assert(retrieved->value.number == 0.01);
    
    ast_parameter_list_destroy(params);
    printf("✓ DSL AST parameters test passed\n");
}

int main(void) {
    printf("Running DSL Component tests...\n\n");
    
    test_lexer();
    test_parser();
    test_ast_parameters();
    
    printf("\n✓ All DSL Component tests passed!\n");
    printf("Note: Full DSL compiler integration requires VM/CodeGen infrastructure\n");
    
    return 0;
}
