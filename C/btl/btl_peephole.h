
// BTL Peephole Optimizer - Pattern matching and optimization
#ifndef BTL_PEEPHOLE_H
#define BTL_PEEPHOLE_H

#include "../c23_compat.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Instruction pattern for matching
typedef struct {
    uint32_t opcode;
    uint32_t operand1;
    uint32_t operand2;
    bool wildcard_op1;
    bool wildcard_op2;
} BTL_InstructionPattern;

// Optimization rule
typedef struct {
    const char *name;
    BTL_InstructionPattern *pattern;
    size_t pattern_length;
    BTL_InstructionPattern *replacement;
    size_t replacement_length;
    uint32_t savings; // Estimated cycle savings
} BTL_OptimizationRule;

// Peephole optimizer context
typedef struct BTL_PeepholeOptimizer BTL_PeepholeOptimizer;

// Create and destroy optimizer
BTL_PeepholeOptimizer* btl_peephole_create(void);
void btl_peephole_destroy(BTL_PeepholeOptimizer *optimizer);

// Add optimization rule
bool btl_peephole_add_rule(BTL_PeepholeOptimizer *optimizer, 
                           const BTL_OptimizationRule *rule);

// Apply optimizations to instruction sequence
size_t btl_peephole_optimize(BTL_PeepholeOptimizer *optimizer,
                              BTL_InstructionPattern *instructions,
                              size_t count,
                              BTL_InstructionPattern *output,
                              size_t output_capacity);

// Get optimization statistics
uint32_t btl_peephole_get_total_savings(BTL_PeepholeOptimizer *optimizer);
size_t btl_peephole_get_rules_applied(BTL_PeepholeOptimizer *optimizer);

// Common optimization rules
extern const BTL_OptimizationRule BTL_RULE_REDUNDANT_MOVE;
extern const BTL_OptimizationRule BTL_RULE_STRENGTH_REDUCTION;
extern const BTL_OptimizationRule BTL_RULE_CONSTANT_FOLDING;
extern const BTL_OptimizationRule BTL_RULE_DEAD_CODE;

#endif // BTL_PEEPHOLE_H
