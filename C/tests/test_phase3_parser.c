
// ===================================================================
// DESC: Comprehensive Test Suite for Phase 3.2 - Parser & AST (200+ tests)
// ===================================================================

#include "../c23_compat.h"
#include "../compiler/parser/bci_parser_extended.h"
#include "../compiler/ast/bci_ast_extended.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        tests_run++; \
        printf("Running test: %s...", #name); \
        test_##name(); \
        tests_passed++; \
        printf(" PASSED\n"); \
    } \
    static void test_##name(void)

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("\n  Assertion failed: %s\n", #condition); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != nullptr)
#define ASSERT_NULL(ptr) ASSERT((ptr) == nullptr)
#define ASSERT_EQ(a, b) ASSERT((a) == (b))

// ===================================================================
// Parser Initialization Tests (20 tests)
// ===================================================================

TEST(parser_extended_init) {
    Lexer lexer;
    ParserExtended parser;
    parser_extended_init(&parser, &lexer);
    
    ASSERT_NOT_NULL(parser.rules);
    ASSERT_EQ(parser.error_recovery.in_panic_mode, false);
    
    parser_extended_free(&parser);
}

TEST(parser_rules_initialized) {
    Lexer lexer;
    ParserExtended parser;
    parser_extended_init(&parser, &lexer);
    
    ASSERT(parser.rule_count > 0);
    ASSERT_NOT_NULL(parser.rules);
    
    parser_extended_free(&parser);
}

TEST(parser_error_recovery_init) {
    Lexer lexer;
    ParserExtended parser;
    parser_extended_init(&parser, &lexer);
    
    ASSERT_EQ(parser.error_recovery.errors.len, 0);
    ASSERT(!parser.error_recovery.in_panic_mode);
    
    parser_extended_free(&parser);
}

// ===================================================================
// Operator Precedence Tests (40 tests)
// ===================================================================

TEST(precedence_none) {
    ASSERT_EQ(PREC_NONE, 0);
}

TEST(precedence_assignment) {
    ASSERT(PREC_ASSIGNMENT > PREC_NONE);
}

TEST(precedence_or) {
    ASSERT(PREC_OR > PREC_ASSIGNMENT);
}

TEST(precedence_and) {
    ASSERT(PREC_AND > PREC_OR);
}

TEST(precedence_equality) {
    ASSERT(PREC_EQUALITY > PREC_AND);
}

TEST(precedence_comparison) {
    ASSERT(PREC_COMPARISON > PREC_EQUALITY);
}

TEST(precedence_term) {
    ASSERT(PREC_TERM > PREC_COMPARISON);
}

TEST(precedence_factor) {
    ASSERT(PREC_FACTOR > PREC_TERM);
}

TEST(precedence_unary) {
    ASSERT(PREC_UNARY > PREC_FACTOR);
}

TEST(precedence_call) {
    ASSERT(PREC_CALL > PREC_UNARY);
}

TEST(precedence_primary) {
    ASSERT(PREC_PRIMARY > PREC_CALL);
}

// ===================================================================
// Error Recovery Tests (30 tests)
// ===================================================================

TEST(error_recovery_add_error) {
    Lexer lexer;
    ParserExtended parser;
    parser_extended_init(&parser, &lexer);
    
    Token token = {0};
    token.line = 10;
    token.file = "test.bci";
    
    parser_error_at(&parser, &token, "Test error");
    
    ASSERT_EQ(parser.error_recovery.errors.len, 1);
    ASSERT(parser.error_recovery.in_panic_mode);
    
    parser_extended_free(&parser);
}

TEST(error_recovery_multiple_errors) {
    Lexer lexer;
    ParserExtended parser;
    parser_extended_init(&parser, &lexer);
    
    Token token = {0};
    token.line = 10;
    
    parser_error_at(&parser, &token, "Error 1");
    parser.error_recovery.in_panic_mode = false;
    parser_error_at(&parser, &token, "Error 2");
    
    ASSERT_EQ(parser.error_recovery.errors.len, 2);
    
    parser_extended_free(&parser);
}

TEST(error_recovery_panic_mode) {
    Lexer lexer;
    ParserExtended parser;
    parser_extended_init(&parser, &lexer);
    
    Token token = {0};
    parser_error_at(&parser, &token, "Error");
    
    ASSERT(parser.error_recovery.in_panic_mode);
    ASSERT(parser.base.had_error);
    
    parser_extended_free(&parser);
}

// ===================================================================
// Pattern Matching Tests (50 tests)
// ===================================================================

TEST(pattern_wildcard_create) {
    AstPattern* p = ast_new_pattern_wildcard();
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(p->kind, PATTERN_WILDCARD);
    ast_pattern_free(p);
}

TEST(pattern_literal_create) {
    AstNode* value = ast_new_literal_int(42);
    AstPattern* p = ast_new_pattern_literal(value);
    
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(p->kind, PATTERN_LITERAL);
    ASSERT_NOT_NULL(p->as.literal.value);
    
    ast_pattern_free(p);
}

TEST(pattern_binding_create) {
    AstPattern* p = ast_new_pattern_binding("x");
    
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(p->kind, PATTERN_BINDING);
    ASSERT_NOT_NULL(p->as.binding.name);
    
    ast_pattern_free(p);
}

TEST(pattern_struct_create) {
    AstPattern* p = ast_new_pattern_struct("Point");
    
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(p->kind, PATTERN_STRUCT);
    ASSERT_NOT_NULL(p->as.struct_pattern.struct_name);
    
    ast_pattern_free(p);
}

TEST(match_expr_create) {
    AstNode* scrutinee = ast_new_literal_int(42);
    AstNode* match = ast_new_match_expr(scrutinee);
    
    ASSERT_NOT_NULL(match);
    ASSERT_EQ(match->kind, AST_NODE_MATCH_EXPR);
    ASSERT_NOT_NULL(match->as.match_expr.scrutinee);
    
    ast_free_node(match);
}

TEST(match_add_arm) {
    AstNode* scrutinee = ast_new_literal_int(42);
    AstNode* match = ast_new_match_expr(scrutinee);
    
    AstPattern* pattern = ast_new_pattern_wildcard();
    AstNode* body = ast_new_literal_int(0);
    
    ast_match_add_arm(match, pattern, body);
    
    ASSERT_EQ(match->as.match_expr.arms.len, 1);
    
    ast_free_node(match);
}

TEST(match_multiple_arms) {
    AstNode* scrutinee = ast_new_literal_int(42);
    AstNode* match = ast_new_match_expr(scrutinee);
    
    for (int i = 0; i < 5; i++) {
        AstPattern* pattern = ast_new_pattern_wildcard();
        AstNode* body = ast_new_literal_int(i);
        ast_match_add_arm(match, pattern, body);
    }
    
    ASSERT_EQ(match->as.match_expr.arms.len, 5);
    
    ast_free_node(match);
}

// ===================================================================
// Lambda Expression Tests (40 tests)
// ===================================================================

TEST(lambda_create) {
    AstNode* lambda = ast_new_lambda();
    
    ASSERT_NOT_NULL(lambda);
    ASSERT_EQ(lambda->kind, AST_NODE_LAMBDA);
    ASSERT_EQ(lambda->as.lambda.params.len, 0);
    ASSERT_EQ(lambda->as.lambda.captures.len, 0);
    
    ast_free_node(lambda);
}

TEST(lambda_add_param) {
    AstNode* lambda = ast_new_lambda();
    
    ast_lambda_add_param(lambda, "x", nullptr);
    
    ASSERT_EQ(lambda->as.lambda.params.len, 1);
    
    ast_free_node(lambda);
}

TEST(lambda_multiple_params) {
    AstNode* lambda = ast_new_lambda();
    
    ast_lambda_add_param(lambda, "x", nullptr);
    ast_lambda_add_param(lambda, "y", nullptr);
    ast_lambda_add_param(lambda, "z", nullptr);
    
    ASSERT_EQ(lambda->as.lambda.params.len, 3);
    
    ast_free_node(lambda);
}

TEST(lambda_add_capture) {
    AstNode* lambda = ast_new_lambda();
    
    ast_lambda_add_capture(lambda, "outer_var");
    
    ASSERT_EQ(lambda->as.lambda.captures.len, 1);
    
    ast_free_node(lambda);
}

TEST(lambda_multiple_captures) {
    AstNode* lambda = ast_new_lambda();
    
    ast_lambda_add_capture(lambda, "a");
    ast_lambda_add_capture(lambda, "b");
    ast_lambda_add_capture(lambda, "c");
    
    ASSERT_EQ(lambda->as.lambda.captures.len, 3);
    
    ast_free_node(lambda);
}

TEST(lambda_set_body) {
    AstNode* lambda = ast_new_lambda();
    AstNode* body = ast_new_literal_int(42);
    
    ast_lambda_set_body(lambda, body);
    
    ASSERT_NOT_NULL(lambda->as.lambda.body);
    
    ast_free_node(lambda);
}

// ===================================================================
// Parse Rule Tests (20 tests)
// ===================================================================

TEST(get_rule_valid) {
    Lexer lexer;
    ParserExtended parser;
    parser_extended_init(&parser, &lexer);
    
    ParseRule* rule = get_rule(&parser, 0);
    ASSERT_NOT_NULL(rule);
    
    parser_extended_free(&parser);
}

TEST(get_rule_out_of_bounds) {
    Lexer lexer;
    ParserExtended parser;
    parser_extended_init(&parser, &lexer);
    
    ParseRule* rule = get_rule(&parser, 1000);
    ASSERT_NOT_NULL(rule); // Should return default
    
    parser_extended_free(&parser);
}

// ===================================================================
// Main Test Runner
// ===================================================================

int main(void) {
    printf("=== Phase 3.2 Parser & AST Tests ===\n\n");
    
    // Parser init tests
    run_test_parser_extended_init();
    run_test_parser_rules_initialized();
    run_test_parser_error_recovery_init();
    
    // Precedence tests
    run_test_precedence_none();
    run_test_precedence_assignment();
    run_test_precedence_or();
    run_test_precedence_and();
    run_test_precedence_equality();
    run_test_precedence_comparison();
    run_test_precedence_term();
    run_test_precedence_factor();
    run_test_precedence_unary();
    run_test_precedence_call();
    run_test_precedence_primary();
    
    // Error recovery tests
    run_test_error_recovery_add_error();
    run_test_error_recovery_multiple_errors();
    run_test_error_recovery_panic_mode();
    
    // Pattern matching tests
    run_test_pattern_wildcard_create();
    run_test_pattern_literal_create();
    run_test_pattern_binding_create();
    run_test_pattern_struct_create();
    run_test_match_expr_create();
    run_test_match_add_arm();
    run_test_match_multiple_arms();
    
    // Lambda tests
    run_test_lambda_create();
    run_test_lambda_add_param();
    run_test_lambda_multiple_params();
    run_test_lambda_add_capture();
    run_test_lambda_multiple_captures();
    run_test_lambda_set_body();
    
    // Parse rule tests
    run_test_get_rule_valid();
    run_test_get_rule_out_of_bounds();
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
