// BTL Peephole Optimizer Tests
#include "../btl/btl_peephole.h"
#include <stdio.h>
#include <assert.h>

static int tests_passed = 0;

#define TEST(name) printf("Testing %s... ", name); fflush(stdout);
#define PASS() printf("PASS\n"); tests_passed++;

void test_optimizer_creation(void) {
    TEST("optimizer creation");
    
    BTL_PeepholeOptimizer *opt = btl_peephole_create();
    assert(opt != NULL);
    assert(btl_peephole_get_rules_applied(opt) == 0);
    
    btl_peephole_destroy(opt);
    PASS();
}

void test_add_rules(void) {
    TEST("add optimization rules");
    
    BTL_PeepholeOptimizer *opt = btl_peephole_create();
    
    bool success = btl_peephole_add_rule(opt, &BTL_RULE_REDUNDANT_MOVE);
    assert(success);
    
    success = btl_peephole_add_rule(opt, &BTL_RULE_STRENGTH_REDUCTION);
    assert(success);
    
    btl_peephole_destroy(opt);
    PASS();
}

void test_pattern_matching(void) {
    TEST("pattern matching");
    
    BTL_PeepholeOptimizer *opt = btl_peephole_create();
    btl_peephole_add_rule(opt, &BTL_RULE_REDUNDANT_MOVE);
    
    BTL_InstructionPattern input[4] = {
        {0x89, 0, 0, false, false}, // MOV (redundant)
        {0x01, 1, 2, false, false}, // ADD
        {0x90, 0, 0, false, false}, // NOP
        {0xC3, 0, 0, false, false}  // RET
    };
    
    BTL_InstructionPattern output[4];
    size_t count = btl_peephole_optimize(opt, input, 4, output, 4);
    
    assert(count > 0);
    
    btl_peephole_destroy(opt);
    PASS();
}

void test_optimization_savings(void) {
    TEST("optimization savings tracking");
    
    BTL_PeepholeOptimizer *opt = btl_peephole_create();
    btl_peephole_add_rule(opt, &BTL_RULE_CONSTANT_FOLDING);
    
    BTL_InstructionPattern input[2] = {
        {0x01, 0, 0, true, false}, // ADD with 0
        {0xC3, 0, 0, false, false}  // RET
    };
    
    BTL_InstructionPattern output[2];
    btl_peephole_optimize(opt, input, 2, output, 2);
    
    // Check if savings were tracked
    uint32_t savings = btl_peephole_get_total_savings(opt);
    printf("Savings: %u cycles... ", savings);
    
    btl_peephole_destroy(opt);
    PASS();
}

int main(void) {
    printf("=== BTL Peephole Optimizer Tests ===\n\n");
    
    test_optimizer_creation();
    test_add_rules();
    test_pattern_matching();
    test_optimization_savings();
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    return 0;
}
