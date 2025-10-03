
// BTL Peephole Optimizer Implementation
#include "btl_peephole.h"
#include <stdlib.h>
#include <string.h>

#define MAX_RULES 128

struct BTL_PeepholeOptimizer {
    BTL_OptimizationRule *rules;
    size_t num_rules;
    size_t rules_capacity;
    
    uint32_t total_savings;
    size_t rules_applied;
};

BTL_PeepholeOptimizer* btl_peephole_create(void) {
    BTL_PeepholeOptimizer *optimizer = malloc(sizeof(BTL_PeepholeOptimizer));
    if (!optimizer) return NULL;
    
    optimizer->rules = calloc(MAX_RULES, sizeof(BTL_OptimizationRule));
    if (!optimizer->rules) {
        free(optimizer);
        return NULL;
    }
    
    optimizer->num_rules = 0;
    optimizer->rules_capacity = MAX_RULES;
    optimizer->total_savings = 0;
    optimizer->rules_applied = 0;
    
    return optimizer;
}

void btl_peephole_destroy(BTL_PeepholeOptimizer *optimizer) {
    if (!optimizer) return;
    
    // Free rule patterns
    for (size_t i = 0; i < optimizer->num_rules; i++) {
        free(optimizer->rules[i].pattern);
        free(optimizer->rules[i].replacement);
    }
    
    free(optimizer->rules);
    free(optimizer);
}

bool btl_peephole_add_rule(BTL_PeepholeOptimizer *optimizer,
                           const BTL_OptimizationRule *rule) {
    if (!optimizer || !rule || optimizer->num_rules >= optimizer->rules_capacity) {
        return false;
    }
    
    BTL_OptimizationRule *new_rule = &optimizer->rules[optimizer->num_rules];
    
    // Copy rule
    new_rule->name = rule->name;
    new_rule->pattern_length = rule->pattern_length;
    new_rule->replacement_length = rule->replacement_length;
    new_rule->savings = rule->savings;
    
    // Allocate and copy pattern
    new_rule->pattern = malloc(rule->pattern_length * sizeof(BTL_InstructionPattern));
    if (!new_rule->pattern) return false;
    memcpy(new_rule->pattern, rule->pattern, 
           rule->pattern_length * sizeof(BTL_InstructionPattern));
    
    // Allocate and copy replacement
    new_rule->replacement = malloc(rule->replacement_length * sizeof(BTL_InstructionPattern));
    if (!new_rule->replacement) {
        free(new_rule->pattern);
        return false;
    }
    memcpy(new_rule->replacement, rule->replacement,
           rule->replacement_length * sizeof(BTL_InstructionPattern));
    
    optimizer->num_rules++;
    return true;
}

// Check if pattern matches at given position
static bool match_pattern(const BTL_InstructionPattern *instructions,
                          size_t start,
                          size_t count,
                          const BTL_InstructionPattern *pattern,
                          size_t pattern_length) {
    if (start + pattern_length > count) return false;
    
    for (size_t i = 0; i < pattern_length; i++) {
        const BTL_InstructionPattern *inst = &instructions[start + i];
        const BTL_InstructionPattern *pat = &pattern[i];
        
        // Check opcode
        if (inst->opcode != pat->opcode) return false;
        
        // Check operands (if not wildcard)
        if (!pat->wildcard_op1 && inst->operand1 != pat->operand1) return false;
        if (!pat->wildcard_op2 && inst->operand2 != pat->operand2) return false;
    }
    
    return true;
}

size_t btl_peephole_optimize(BTL_PeepholeOptimizer *optimizer,
                              BTL_InstructionPattern *instructions,
                              size_t count,
                              BTL_InstructionPattern *output,
                              size_t output_capacity) {
    if (!optimizer || !instructions || !output) return 0;
    
    size_t output_pos = 0;
    size_t i = 0;
    
    while (i < count && output_pos < output_capacity) {
        bool matched = false;
        
        // Try to match each rule
        for (size_t r = 0; r < optimizer->num_rules; r++) {
            const BTL_OptimizationRule *rule = &optimizer->rules[r];
            
            if (match_pattern(instructions, i, count, rule->pattern, rule->pattern_length)) {
                // Apply optimization
                if (output_pos + rule->replacement_length <= output_capacity) {
                    memcpy(&output[output_pos], rule->replacement,
                           rule->replacement_length * sizeof(BTL_InstructionPattern));
                    output_pos += rule->replacement_length;
                    i += rule->pattern_length;
                    
                    optimizer->total_savings += rule->savings;
                    optimizer->rules_applied++;
                    matched = true;
                    break;
                }
            }
        }
        
        // If no rule matched, copy instruction as-is
        if (!matched) {
            if (output_pos < output_capacity) {
                output[output_pos++] = instructions[i++];
            } else {
                break;
            }
        }
    }
    
    return output_pos;
}

uint32_t btl_peephole_get_total_savings(BTL_PeepholeOptimizer *optimizer) {
    return optimizer ? optimizer->total_savings : 0;
}

size_t btl_peephole_get_rules_applied(BTL_PeepholeOptimizer *optimizer) {
    return optimizer ? optimizer->rules_applied : 0;
}

// Common optimization rules (examples)

// Redundant move elimination: MOV r1, r1 -> NOP
static BTL_InstructionPattern redundant_move_pattern[] = {
    {0x89, 0, 0, false, false} // MOV with same src/dst
};
static BTL_InstructionPattern redundant_move_replacement[] = {
    {0x90, 0, 0, false, false} // NOP
};

const BTL_OptimizationRule BTL_RULE_REDUNDANT_MOVE = {
    "Redundant Move Elimination",
    redundant_move_pattern,
    1,
    redundant_move_replacement,
    1,
    1 // 1 cycle saved
};

// Strength reduction: MUL by power of 2 -> SHL
static BTL_InstructionPattern strength_reduction_pattern[] = {
    {0xF7, 0, 2, true, false} // MUL by 2
};
static BTL_InstructionPattern strength_reduction_replacement[] = {
    {0xD1, 0, 1, true, false} // SHL by 1
};

const BTL_OptimizationRule BTL_RULE_STRENGTH_REDUCTION = {
    "Strength Reduction (MUL->SHL)",
    strength_reduction_pattern,
    1,
    strength_reduction_replacement,
    1,
    2 // 2 cycles saved
};

// Constant folding: ADD r1, 0 -> NOP
static BTL_InstructionPattern constant_folding_pattern[] = {
    {0x01, 0, 0, true, false} // ADD with 0
};
static BTL_InstructionPattern constant_folding_replacement[] = {
    {0x90, 0, 0, false, false} // NOP
};

const BTL_OptimizationRule BTL_RULE_CONSTANT_FOLDING = {
    "Constant Folding (ADD 0)",
    constant_folding_pattern,
    1,
    constant_folding_replacement,
    1,
    1 // 1 cycle saved
};

// Dead code elimination: XOR r1, r1 followed by unused result
static BTL_InstructionPattern dead_code_pattern[] = {
    {0x31, 0, 0, false, false} // XOR r, r (sets to 0)
};
static BTL_InstructionPattern dead_code_replacement[] = {
    {0x90, 0, 0, false, false} // NOP (if result unused)
};

const BTL_OptimizationRule BTL_RULE_DEAD_CODE = {
    "Dead Code Elimination",
    dead_code_pattern,
    1,
    dead_code_replacement,
    1,
    1 // 1 cycle saved
};
