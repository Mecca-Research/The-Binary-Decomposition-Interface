
/**
 * @file ml_dsl_test.c
 * @brief ML DSL Tests
 */

#include "../DSL/ml_dsl_lexer.h"
#include "../DSL/ml_dsl_parser.h"
#include "../DSL/ml_dsl_compiler.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

void test_lexer_basic(void) {
    printf("Testing DSL lexer basic tokenization...\n");
    
    const char* source = "ml model test { type: linear_regression; }";
    
    Lexer* lexer = lexer_create(source);
    assert(lexer != nullptr);
    
    Token token1 = lexer_next_token(lexer);
    assert(token1.type == TOKEN_ML);
    
    Token token2 = lexer_next_token(lexer);
    assert(token2.type == TOKEN_MODEL);
    
    Token token3 = lexer_next_token(lexer);
    assert(token3.type == TOKEN_IDENTIFIER);
    assert(strcmp(token3.lexeme, "test") == 0);
    
    lexer_destroy(lexer);
    
    printf("✓ DSL lexer basic test passed\n");
}

void test_lexer_numbers(void) {
    printf("Testing DSL lexer number tokenization...\n");
    
    const char* source = "learning_rate: 0.01; max_depth: 10;";
    
    Lexer* lexer = lexer_create(source);
    
    size_t token_count;
    Token* tokens = lexer_tokenize_all(lexer, &token_count);
    
    assert(tokens != nullptr);
    assert(token_count > 0);
    
    // Find number tokens
    bool found_float = false;
    bool found_int = false;
    
    for (size_t i = 0; i < token_count; i++) {
        if (tokens[i].type == TOKEN_NUMBER) {
            if (tokens[i].value.number == 0.01) found_float = true;
            if (tokens[i].value.number == 10.0) found_int = true;
        }
    }
    
    assert(found_float);
    assert(found_int);
    
    free(tokens);
    lexer_destroy(lexer);
    
    printf("✓ DSL lexer numbers test passed\n");
}

void test_parser_model_decl(void) {
    printf("Testing DSL parser model declaration...\n");
    
    const char* source = 
        "ml model linear_reg {\n"
        "    type: linear_regression;\n"
        "    input: vector[10];\n"
        "    learning_rate: 0.01;\n"
        "}";
    
    Lexer* lexer = lexer_create(source);
    size_t token_count;
    Token* tokens = lexer_tokenize_all(lexer, &token_count);
    lexer_destroy(lexer);
    
    Parser* parser = parser_create(tokens, token_count);
    ASTNode* node = parser_parse_model_declaration(parser);
    
    assert(node != nullptr);
    assert(node->type == AST_MODEL_DECL);
    assert(strcmp(node->data.model_decl.name, "linear_reg") == 0);
    assert(node->data.model_decl.parameters != nullptr);
    
    ast_node_destroy(node);
    parser_destroy(parser);
    free(tokens);
    
    printf("✓ DSL parser model declaration test passed\n");
}

void test_parser_train_stmt(void) {
    printf("Testing DSL parser train statement...\n");
    
    const char* source = 
        "train my_model with dataset \"data.csv\" {\n"
        "    epochs: 100;\n"
        "    batch_size: 32;\n"
        "};";
    
    Lexer* lexer = lexer_create(source);
    size_t token_count;
    Token* tokens = lexer_tokenize_all(lexer, &token_count);
    lexer_destroy(lexer);
    
    Parser* parser = parser_create(tokens, token_count);
    ASTNode* node = parser_parse_train_statement(parser);
    
    assert(node != nullptr);
    assert(node->type == AST_TRAIN_STMT);
    assert(strcmp(node->data.train_stmt.model_name, "my_model") == 0);
    assert(strcmp(node->data.train_stmt.dataset_path, "data.csv") == 0);
    
    ast_node_destroy(node);
    parser_destroy(parser);
    free(tokens);
    
    printf("✓ DSL parser train statement test passed\n");
}

void test_parser_predict_stmt(void) {
    printf("Testing DSL parser predict statement...\n");
    
    const char* source = "predict my_model on input [1.0, 2.0, 3.0];";
    
    Lexer* lexer = lexer_create(source);
    size_t token_count;
    Token* tokens = lexer_tokenize_all(lexer, &token_count);
    lexer_destroy(lexer);
    
    Parser* parser = parser_create(tokens, token_count);
    ASTNode* node = parser_parse_predict_statement(parser);
    
    assert(node != nullptr);
    assert(node->type == AST_PREDICT_STMT);
    assert(strcmp(node->data.predict_stmt.model_name, "my_model") == 0);
    assert(node->data.predict_stmt.input_count == 3);
    
    ast_node_destroy(node);
    parser_destroy(parser);
    free(tokens);
    
    printf("✓ DSL parser predict statement test passed\n");
}

void test_compiler_model_creation(void) {
    printf("Testing DSL compiler model creation...\n");
    
    const char* source = 
        "ml model test_lr {\n"
        "    type: linear_regression;\n"
        "    input: vector[5];\n"
        "    learning_rate: 0.05;\n"
        "}";
    
    MLVMContext* vm_context = nullptr;
    bool success = dsl_compile_source(source, &vm_context);
    
    assert(success);
    assert(vm_context != nullptr);
    
    ml_vm_context_destroy(vm_context);
    
    printf("✓ DSL compiler model creation test passed\n");
}

void test_compiler_full_program(void) {
    printf("Testing DSL compiler full program...\n");
    
    const char* source = 
        "ml model my_kmeans {\n"
        "    type: kmeans;\n"
        "    n_clusters: 3;\n"
        "    max_iterations: 100;\n"
        "}\n"
        "\n"
        "ml model my_qlearning {\n"
        "    type: qlearning;\n"
        "    state_space: discrete[10];\n"
        "    action_space: discrete[4];\n"
        "    learning_rate: 0.1;\n"
        "    discount_factor: 0.99;\n"
        "}";
    
    MLVMContext* vm_context = nullptr;
    bool success = dsl_compile_source(source, &vm_context);
    
    assert(success);
    assert(vm_context != nullptr);
    
    ml_vm_context_destroy(vm_context);
    
    printf("✓ DSL compiler full program test passed\n");
}

void test_ast_operations(void) {
    printf("Testing AST operations...\n");
    
    // Create parameter
    ASTParameter* param = ast_parameter_create("test_param");
    assert(param != nullptr);
    param->value_type = PARAM_NUMBER;
    param->value.number = 42.0;
    
    // Create parameter list
    ASTParameterList* list = ast_parameter_list_create();
    assert(list != nullptr);
    
    ast_parameter_list_add(list, param);
    assert(list->count == 1);
    
    ASTParameter* retrieved = ast_parameter_list_get(list, "test_param");
    assert(retrieved != nullptr);
    assert(retrieved->value.number == 42.0);
    
    ast_parameter_list_destroy(list);
    
    printf("✓ AST operations test passed\n");
}

int main(void) {
    printf("Running ML DSL tests...\n\n");
    
    test_lexer_basic();
    test_lexer_numbers();
    test_parser_model_decl();
    test_parser_train_stmt();
    test_parser_predict_stmt();
    test_compiler_model_creation();
    test_compiler_full_program();
    test_ast_operations();
    
    printf("\n✓ All ML DSL tests passed!\n");
    return 0;
}
