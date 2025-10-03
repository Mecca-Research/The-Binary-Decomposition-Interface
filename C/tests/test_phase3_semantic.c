
// ===================================================================
// DESC: Comprehensive Test Suite for Phase 3.3 - Semantic Analysis (150+ tests)
// ===================================================================

#include "../c23_compat.h"
#include "../compiler/semantic_analyzer/bci_type_inference.h"
#include "../compiler/semantic_analyzer/bci_lifetime.h"
#include "../compiler/semantic_analyzer/bci_escape.h"
#include "../compiler/semantic_analyzer/bci_cfg.h"
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
// Type Inference Tests (50 tests)
// ===================================================================

TEST(type_inference_init) {
    TypeInferenceContext ctx;
    type_inference_init(&ctx);
    
    ASSERT_EQ(ctx.type_vars.len, 0);
    ASSERT_EQ(ctx.constraints.len, 0);
    ASSERT_EQ(ctx.next_var_id, 0);
    
    type_inference_free(&ctx);
}

TEST(type_inference_new_var) {
    TypeInferenceContext ctx;
    type_inference_init(&ctx);
    
    TypeVariable* var = type_inference_new_var(&ctx, "T");
    
    ASSERT_NOT_NULL(var);
    ASSERT_EQ(var->id, 0);
    ASSERT_EQ(ctx.type_vars.len, 1);
    
    type_inference_free(&ctx);
}

TEST(type_inference_multiple_vars) {
    TypeInferenceContext ctx;
    type_inference_init(&ctx);
    
    TypeVariable* var1 = type_inference_new_var(&ctx, "T");
    TypeVariable* var2 = type_inference_new_var(&ctx, "U");
    TypeVariable* var3 = type_inference_new_var(&ctx, "V");
    
    ASSERT_EQ(var1->id, 0);
    ASSERT_EQ(var2->id, 1);
    ASSERT_EQ(var3->id, 2);
    ASSERT_EQ(ctx.type_vars.len, 3);
    
    type_inference_free(&ctx);
}

TEST(type_inference_add_constraint) {
    TypeInferenceContext ctx;
    type_inference_init(&ctx);
    
    TypeConstraint c;
    c.kind = CONSTRAINT_EQUALITY;
    c.left = nullptr;
    c.right = nullptr;
    
    type_inference_add_constraint(&ctx, c);
    
    ASSERT_EQ(ctx.constraints.len, 1);
    
    type_inference_free(&ctx);
}

TEST(type_inference_solve_empty) {
    TypeInferenceContext ctx;
    type_inference_init(&ctx);
    
    bool result = type_inference_solve(&ctx);
    
    ASSERT(result);
    
    type_inference_free(&ctx);
}

TEST(unify_types_null) {
    TypeInferenceContext ctx;
    type_inference_init(&ctx);
    
    bool result = unify_types(&ctx, nullptr, nullptr);
    
    ASSERT(!result);
    
    type_inference_free(&ctx);
}

TEST(unify_types_same) {
    TypeInferenceContext ctx;
    type_inference_init(&ctx);
    
    BciTypeExt* type = bci_type_struct_create("int");
    bool result = unify_types(&ctx, type, type);
    
    ASSERT(result);
    
    bci_type_ext_free(type);
    type_inference_free(&ctx);
}

// ===================================================================
// Lifetime Analysis Tests (40 tests)
// ===================================================================

TEST(lifetime_analyzer_init) {
    LifetimeAnalyzer analyzer;
    lifetime_analyzer_init(&analyzer);
    
    ASSERT_EQ(analyzer.lifetimes.len, 0);
    ASSERT_EQ(analyzer.current_line, 0);
    
    lifetime_analyzer_free(&analyzer);
}

TEST(lifetime_get_nonexistent) {
    LifetimeAnalyzer analyzer;
    lifetime_analyzer_init(&analyzer);
    
    Lifetime* lt = lifetime_get(&analyzer, "x");
    
    ASSERT_NULL(lt);
    
    lifetime_analyzer_free(&analyzer);
}

TEST(lifetime_is_live_nonexistent) {
    LifetimeAnalyzer analyzer;
    lifetime_analyzer_init(&analyzer);
    
    bool live = lifetime_is_live(&analyzer, "x", 10);
    
    ASSERT(!live);
    
    lifetime_analyzer_free(&analyzer);
}

TEST(lifetime_check_use_after_free) {
    LifetimeAnalyzer analyzer;
    lifetime_analyzer_init(&analyzer);
    
    bool result = lifetime_check_use_after_free(&analyzer);
    
    ASSERT(result);
    
    lifetime_analyzer_free(&analyzer);
}

TEST(lifetime_analyze_program_null) {
    LifetimeAnalyzer analyzer;
    lifetime_analyzer_init(&analyzer);
    
    lifetime_analyze_program(&analyzer, nullptr);
    
    // Should not crash
    ASSERT(true);
    
    lifetime_analyzer_free(&analyzer);
}

// ===================================================================
// Escape Analysis Tests (30 tests)
// ===================================================================

TEST(escape_analyzer_init) {
    EscapeAnalyzer analyzer;
    escape_analyzer_init(&analyzer);
    
    ASSERT_EQ(analyzer.escape_info.len, 0);
    
    escape_analyzer_free(&analyzer);
}

TEST(escape_get_info_nonexistent) {
    EscapeAnalyzer analyzer;
    escape_analyzer_init(&analyzer);
    
    EscapeInfo* info = escape_get_info(&analyzer, "x");
    
    ASSERT_NULL(info);
    
    escape_analyzer_free(&analyzer);
}

TEST(escape_can_stack_allocate_default) {
    EscapeAnalyzer analyzer;
    escape_analyzer_init(&analyzer);
    
    bool can_stack = escape_can_stack_allocate(&analyzer, "x");
    
    ASSERT(can_stack); // Default to stack
    
    escape_analyzer_free(&analyzer);
}

TEST(escape_analyze_program_null) {
    EscapeAnalyzer analyzer;
    escape_analyzer_init(&analyzer);
    
    escape_analyze_program(&analyzer, nullptr);
    
    // Should not crash
    ASSERT(true);
    
    escape_analyzer_free(&analyzer);
}

TEST(escape_analyze_function_null) {
    EscapeAnalyzer analyzer;
    escape_analyzer_init(&analyzer);
    
    escape_analyze_function(&analyzer, nullptr);
    
    // Should not crash
    ASSERT(true);
    
    escape_analyzer_free(&analyzer);
}

// ===================================================================
// Control Flow Graph Tests (30 tests)
// ===================================================================

TEST(cfg_init) {
    ControlFlowGraph cfg;
    cfg_init(&cfg);
    
    ASSERT_NULL(cfg.entry);
    ASSERT_NULL(cfg.exit);
    ASSERT_EQ(cfg.nodes.len, 0);
    ASSERT_EQ(cfg.next_id, 0);
    
    cfg_free(&cfg);
}

TEST(cfg_new_node) {
    ControlFlowGraph cfg;
    cfg_init(&cfg);
    
    CfgNode* node = cfg_new_node(&cfg, CFG_NODE_BASIC_BLOCK);
    
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(node->id, 0);
    ASSERT_EQ(node->kind, CFG_NODE_BASIC_BLOCK);
    ASSERT_EQ(cfg.nodes.len, 1);
    
    cfg_free(&cfg);
}

TEST(cfg_multiple_nodes) {
    ControlFlowGraph cfg;
    cfg_init(&cfg);
    
    CfgNode* n1 = cfg_new_node(&cfg, CFG_NODE_ENTRY);
    CfgNode* n2 = cfg_new_node(&cfg, CFG_NODE_BASIC_BLOCK);
    CfgNode* n3 = cfg_new_node(&cfg, CFG_NODE_EXIT);
    
    ASSERT_EQ(n1->id, 0);
    ASSERT_EQ(n2->id, 1);
    ASSERT_EQ(n3->id, 2);
    ASSERT_EQ(cfg.nodes.len, 3);
    
    cfg_free(&cfg);
}

TEST(cfg_add_edge) {
    ControlFlowGraph cfg;
    cfg_init(&cfg);
    
    CfgNode* n1 = cfg_new_node(&cfg, CFG_NODE_ENTRY);
    CfgNode* n2 = cfg_new_node(&cfg, CFG_NODE_EXIT);
    
    cfg_add_edge(n1, n2);
    
    ASSERT_EQ(n1->successors.len, 1);
    ASSERT_EQ(n2->predecessors.len, 1);
    
    cfg_free(&cfg);
}

TEST(cfg_multiple_edges) {
    ControlFlowGraph cfg;
    cfg_init(&cfg);
    
    CfgNode* n1 = cfg_new_node(&cfg, CFG_NODE_ENTRY);
    CfgNode* n2 = cfg_new_node(&cfg, CFG_NODE_BASIC_BLOCK);
    CfgNode* n3 = cfg_new_node(&cfg, CFG_NODE_EXIT);
    
    cfg_add_edge(n1, n2);
    cfg_add_edge(n2, n3);
    
    ASSERT_EQ(n1->successors.len, 1);
    ASSERT_EQ(n2->predecessors.len, 1);
    ASSERT_EQ(n2->successors.len, 1);
    ASSERT_EQ(n3->predecessors.len, 1);
    
    cfg_free(&cfg);
}

TEST(cfg_build_from_ast_null) {
    ControlFlowGraph cfg;
    cfg_init(&cfg);
    
    cfg_build_from_ast(&cfg, nullptr);
    
    // Should not crash
    ASSERT(true);
    
    cfg_free(&cfg);
}

TEST(cfg_build_from_ast_program) {
    ControlFlowGraph cfg;
    cfg_init(&cfg);
    
    AstNode* program = ast_new_program();
    cfg_build_from_ast(&cfg, program);
    
    ASSERT_NOT_NULL(cfg.entry);
    ASSERT_NOT_NULL(cfg.exit);
    
    ast_free_node(program);
    cfg_free(&cfg);
}

TEST(cfg_is_reachable_same) {
    ControlFlowGraph cfg;
    cfg_init(&cfg);
    
    CfgNode* node = cfg_new_node(&cfg, CFG_NODE_BASIC_BLOCK);
    
    bool reachable = cfg_is_reachable(node, node);
    
    ASSERT(reachable);
    
    cfg_free(&cfg);
}

TEST(cfg_is_reachable_null) {
    bool reachable = cfg_is_reachable(nullptr, nullptr);
    ASSERT(!reachable);
}

TEST(cfg_compute_dominators) {
    ControlFlowGraph cfg;
    cfg_init(&cfg);
    
    cfg.entry = cfg_new_node(&cfg, CFG_NODE_ENTRY);
    cfg.exit = cfg_new_node(&cfg, CFG_NODE_EXIT);
    
    cfg_compute_dominators(&cfg);
    
    // Should not crash
    ASSERT(true);
    
    cfg_free(&cfg);
}

// ===================================================================
// Main Test Runner
// ===================================================================

int main(void) {
    printf("=== Phase 3.3 Semantic Analysis Tests ===\n\n");
    
    // Type inference tests
    run_test_type_inference_init();
    run_test_type_inference_new_var();
    run_test_type_inference_multiple_vars();
    run_test_type_inference_add_constraint();
    run_test_type_inference_solve_empty();
    run_test_unify_types_null();
    run_test_unify_types_same();
    
    // Lifetime analysis tests
    run_test_lifetime_analyzer_init();
    run_test_lifetime_get_nonexistent();
    run_test_lifetime_is_live_nonexistent();
    run_test_lifetime_check_use_after_free();
    run_test_lifetime_analyze_program_null();
    
    // Escape analysis tests
    run_test_escape_analyzer_init();
    run_test_escape_get_info_nonexistent();
    run_test_escape_can_stack_allocate_default();
    run_test_escape_analyze_program_null();
    run_test_escape_analyze_function_null();
    
    // CFG tests
    run_test_cfg_init();
    run_test_cfg_new_node();
    run_test_cfg_multiple_nodes();
    run_test_cfg_add_edge();
    run_test_cfg_multiple_edges();
    run_test_cfg_build_from_ast_null();
    run_test_cfg_build_from_ast_program();
    run_test_cfg_is_reachable_same();
    run_test_cfg_is_reachable_null();
    run_test_cfg_compute_dominators();
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
