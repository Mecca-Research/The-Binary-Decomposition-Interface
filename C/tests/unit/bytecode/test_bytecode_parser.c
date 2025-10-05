
#include "../../framework/test_framework.h"
#include "../../../bci/bci_parser.h"
#include "../../../bci/bci_lexer.h"

// Test bytecode parser initialization
static bool test_parser_initialization(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Parser* parser = parser_create();
    TEST_ASSERT_NOT_NULL(parser, "Parser creation should succeed");
    
    // Test initial state
    TEST_ASSERT(!parser_has_error(parser), "Parser should not have errors initially");
    TEST_ASSERT_EQ(0, parser_get_token_count(parser), "Parser should have no tokens initially");
    
    parser_destroy(parser);
    TEST_MEMORY_VERIFY("Parser initialization should not leak memory");
    
    return true;
}

// Test lexical analysis
static bool test_lexical_analysis(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Parser* parser = parser_create();
    TEST_ASSERT_NOT_NULL(parser, "Parser creation should succeed");
    
    const char* source = "42 + 3.14 * true";
    bool success = parser_tokenize(parser, source);
    TEST_ASSERT(success, "Tokenization should succeed");
    
    // Check token count
    size_t token_count = parser_get_token_count(parser);
    TEST_ASSERT_EQ(5, token_count, "Should have 5 tokens (number, +, number, *, boolean)");
    
    // Check token types
    Token* tokens = parser_get_tokens(parser);
    TEST_ASSERT_EQ(TOKEN_NUMBER, tokens[0].type, "First token should be NUMBER");
    TEST_ASSERT_EQ(TOKEN_PLUS, tokens[1].type, "Second token should be PLUS");
    TEST_ASSERT_EQ(TOKEN_NUMBER, tokens[2].type, "Third token should be NUMBER");
    TEST_ASSERT_EQ(TOKEN_STAR, tokens[3].type, "Fourth token should be STAR");
    TEST_ASSERT_EQ(TOKEN_TRUE, tokens[4].type, "Fifth token should be TRUE");
    
    // Check token values
    TEST_ASSERT_EQ(42.0, tokens[0].value.as_number, "First token value should be 42.0");
    TEST_ASSERT_EQ(3.14, tokens[2].value.as_number, "Third token value should be 3.14");
    TEST_ASSERT(tokens[4].value.as_boolean, "Fifth token value should be true");
    
    parser_destroy(parser);
    TEST_MEMORY_VERIFY("Lexical analysis should not leak memory");
    
    return true;
}

// Test expression parsing
static bool test_expression_parsing(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Parser* parser = parser_create();
    TEST_ASSERT_NOT_NULL(parser, "Parser creation should succeed");
    
    const char* source = "2 + 3 * 4";
    bool tokenized = parser_tokenize(parser, source);
    TEST_ASSERT(tokenized, "Tokenization should succeed");
    
    ASTNode* ast = parser_parse_expression(parser);
    TEST_ASSERT_NOT_NULL(ast, "Expression parsing should succeed");
    
    // Check AST structure (should respect operator precedence)
    TEST_ASSERT_EQ(AST_BINARY_OP, ast->type, "Root should be binary operation");
    TEST_ASSERT_EQ(TOKEN_PLUS, ast->data.binary_op.operator, "Root operator should be PLUS");
    
    // Left side should be number 2
    TEST_ASSERT_EQ(AST_NUMBER, ast->data.binary_op.left->type, "Left operand should be number");
    TEST_ASSERT_EQ(2.0, ast->data.binary_op.left->data.number, "Left operand should be 2.0");
    
    // Right side should be multiplication
    TEST_ASSERT_EQ(AST_BINARY_OP, ast->data.binary_op.right->type, "Right operand should be binary operation");
    TEST_ASSERT_EQ(TOKEN_STAR, ast->data.binary_op.right->data.binary_op.operator, "Right operator should be STAR");
    
    ast_destroy(ast);
    parser_destroy(parser);
    TEST_MEMORY_VERIFY("Expression parsing should not leak memory");
    
    return true;
}

// Test statement parsing
static bool test_statement_parsing(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Parser* parser = parser_create();
    TEST_ASSERT_NOT_NULL(parser, "Parser creation should succeed");
    
    const char* source = "var x = 42; print x;";
    bool tokenized = parser_tokenize(parser, source);
    TEST_ASSERT(tokenized, "Tokenization should succeed");
    
    ASTNode* program = parser_parse_program(parser);
    TEST_ASSERT_NOT_NULL(program, "Program parsing should succeed");
    
    // Check program structure
    TEST_ASSERT_EQ(AST_PROGRAM, program->type, "Root should be program");
    TEST_ASSERT_EQ(2, program->data.program.statement_count, "Program should have 2 statements");
    
    // Check first statement (variable declaration)
    ASTNode* var_decl = program->data.program.statements[0];
    TEST_ASSERT_EQ(AST_VAR_DECL, var_decl->type, "First statement should be variable declaration");
    TEST_ASSERT_STR_EQ("x", var_decl->data.var_decl.name, "Variable name should be 'x'");
    TEST_ASSERT_EQ(AST_NUMBER, var_decl->data.var_decl.initializer->type, "Initializer should be number");
    
    // Check second statement (print statement)
    ASTNode* print_stmt = program->data.program.statements[1];
    TEST_ASSERT_EQ(AST_PRINT_STMT, print_stmt->type, "Second statement should be print statement");
    TEST_ASSERT_EQ(AST_IDENTIFIER, print_stmt->data.print_stmt.expression->type, "Print expression should be identifier");
    
    ast_destroy(program);
    parser_destroy(parser);
    TEST_MEMORY_VERIFY("Statement parsing should not leak memory");
    
    return true;
}

// Test error handling in parsing
static bool test_parser_error_handling(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Parser* parser = parser_create();
    TEST_ASSERT_NOT_NULL(parser, "Parser creation should succeed");
    
    // Test syntax error
    const char* invalid_source = "2 + + 3";
    bool tokenized = parser_tokenize(parser, invalid_source);
    TEST_ASSERT(tokenized, "Tokenization should succeed even with syntax errors");
    
    ASTNode* ast = parser_parse_expression(parser);
    TEST_ASSERT_NULL(ast, "Parsing should fail with syntax error");
    TEST_ASSERT(parser_has_error(parser), "Parser should have error after failed parsing");
    
    const char* error_msg = parser_get_error(parser);
    TEST_ASSERT_NOT_NULL(error_msg, "Error message should be available");
    
    // Test error recovery
    parser_clear_error(parser);
    TEST_ASSERT(!parser_has_error(parser), "Error should be cleared");
    
    // Test valid parsing after error recovery
    const char* valid_source = "2 + 3";
    tokenized = parser_tokenize(parser, valid_source);
    TEST_ASSERT(tokenized, "Tokenization should succeed");
    
    ast = parser_parse_expression(parser);
    TEST_ASSERT_NOT_NULL(ast, "Parsing should succeed after error recovery");
    
    ast_destroy(ast);
    parser_destroy(parser);
    TEST_MEMORY_VERIFY("Parser error handling should not leak memory");
    
    return true;
}

// Test complex expression parsing
static bool test_complex_expression_parsing(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Parser* parser = parser_create();
    TEST_ASSERT_NOT_NULL(parser, "Parser creation should succeed");
    
    const char* source = "(2 + 3) * (4 - 1) / 2";
    bool tokenized = parser_tokenize(parser, source);
    TEST_ASSERT(tokenized, "Tokenization should succeed");
    
    ASTNode* ast = parser_parse_expression(parser);
    TEST_ASSERT_NOT_NULL(ast, "Complex expression parsing should succeed");
    
    // Verify the AST represents the correct structure
    TEST_ASSERT_EQ(AST_BINARY_OP, ast->type, "Root should be binary operation");
    TEST_ASSERT_EQ(TOKEN_SLASH, ast->data.binary_op.operator, "Root operator should be division");
    
    // Left side should be multiplication
    ASTNode* mult = ast->data.binary_op.left;
    TEST_ASSERT_EQ(AST_BINARY_OP, mult->type, "Left operand should be binary operation");
    TEST_ASSERT_EQ(TOKEN_STAR, mult->data.binary_op.operator, "Left operator should be multiplication");
    
    // Right side should be number 2
    TEST_ASSERT_EQ(AST_NUMBER, ast->data.binary_op.right->type, "Right operand should be number");
    TEST_ASSERT_EQ(2.0, ast->data.binary_op.right->data.number, "Right operand should be 2.0");
    
    ast_destroy(ast);
    parser_destroy(parser);
    TEST_MEMORY_VERIFY("Complex expression parsing should not leak memory");
    
    return true;
}

// Test function parsing
static bool test_function_parsing(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Parser* parser = parser_create();
    TEST_ASSERT_NOT_NULL(parser, "Parser creation should succeed");
    
    const char* source = "function add(a, b) { return a + b; }";
    bool tokenized = parser_tokenize(parser, source);
    TEST_ASSERT(tokenized, "Tokenization should succeed");
    
    ASTNode* func = parser_parse_function(parser);
    TEST_ASSERT_NOT_NULL(func, "Function parsing should succeed");
    
    // Check function structure
    TEST_ASSERT_EQ(AST_FUNCTION, func->type, "Node should be function");
    TEST_ASSERT_STR_EQ("add", func->data.function.name, "Function name should be 'add'");
    TEST_ASSERT_EQ(2, func->data.function.param_count, "Function should have 2 parameters");
    TEST_ASSERT_STR_EQ("a", func->data.function.params[0], "First parameter should be 'a'");
    TEST_ASSERT_STR_EQ("b", func->data.function.params[1], "Second parameter should be 'b'");
    
    // Check function body
    TEST_ASSERT_NOT_NULL(func->data.function.body, "Function should have body");
    TEST_ASSERT_EQ(AST_BLOCK, func->data.function.body->type, "Function body should be block");
    
    ast_destroy(func);
    parser_destroy(parser);
    TEST_MEMORY_VERIFY("Function parsing should not leak memory");
    
    return true;
}

// Test control flow parsing
static bool test_control_flow_parsing(void) {
    TEST_MEMORY_CHECKPOINT();
    
    Parser* parser = parser_create();
    TEST_ASSERT_NOT_NULL(parser, "Parser creation should succeed");
    
    const char* source = "if (x > 0) { print \"positive\"; } else { print \"non-positive\"; }";
    bool tokenized = parser_tokenize(parser, source);
    TEST_ASSERT(tokenized, "Tokenization should succeed");
    
    ASTNode* if_stmt = parser_parse_statement(parser);
    TEST_ASSERT_NOT_NULL(if_stmt, "If statement parsing should succeed");
    
    // Check if statement structure
    TEST_ASSERT_EQ(AST_IF_STMT, if_stmt->type, "Node should be if statement");
    TEST_ASSERT_NOT_NULL(if_stmt->data.if_stmt.condition, "If statement should have condition");
    TEST_ASSERT_NOT_NULL(if_stmt->data.if_stmt.then_branch, "If statement should have then branch");
    TEST_ASSERT_NOT_NULL(if_stmt->data.if_stmt.else_branch, "If statement should have else branch");
    
    // Check condition
    TEST_ASSERT_EQ(AST_BINARY_OP, if_stmt->data.if_stmt.condition->type, "Condition should be binary operation");
    TEST_ASSERT_EQ(TOKEN_GREATER, if_stmt->data.if_stmt.condition->data.binary_op.operator, "Condition operator should be GREATER");
    
    ast_destroy(if_stmt);
    parser_destroy(parser);
    TEST_MEMORY_VERIFY("Control flow parsing should not leak memory");
    
    return true;
}

// Test suite definition
static test_function_t bytecode_parser_tests[] = {
    test_parser_initialization,
    test_lexical_analysis,
    test_expression_parsing,
    test_statement_parsing,
    test_parser_error_handling,
    test_complex_expression_parsing,
    test_function_parsing,
    test_control_flow_parsing
};

test_suite_t bytecode_test_suite = {
    .name = "Bytecode Parser Tests",
    .tests = bytecode_parser_tests,
    .test_count = sizeof(bytecode_parser_tests) / sizeof(bytecode_parser_tests[0])
};
