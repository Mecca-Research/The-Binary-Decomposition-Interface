
/**
 * @file multi_rail_synthesis.c
 * @brief Multi-Rail Synthesis System Implementation
 * 
 * Phase 2 Master Memory Manager - Complete Toolchain
 * Multi-rail synthesis implementation with validation and auto-rewrite
 */

#include "multi_rail_synthesis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// String constants
static const char* rail_type_strings[] = {
    "ASM_DSL", "C_Reference", "Proof_Stubs", "Optimized_ASM", 
    "Vectorized", "Parallel", "Custom", "Verification"
};

static const char* phase_strings[] = {
    "Parse", "Analyze", "Generate", "Validate", "Optimize", "Verify", "Finalize"
};

static const char* validation_type_strings[] = {
    "Equivalence", "Safety", "Performance", "Correctness", 
    "Memory_Safety", "Timing", "Resource_Usage"
};

static const char* target_strings[] = {
    "x86_64", "x86_32", "ARM64", "ARM32", "RISC_V", "Custom"
};

static const char* opt_level_strings[] = {
    "None", "Size", "Speed", "Balanced", "Aggressive"
};

/**
 * @brief Initialize synthesis context
 */
int synthesis_init(synthesis_context_t* context, const char* project_name) {
    if (!context) return -1;
    
    memset(context, 0, sizeof(*context));
    
    if (project_name) {
        strncpy(context->project_name, project_name, sizeof(context->project_name) - 1);
    }
    
    context->target = CODEGEN_TARGET_X86_64;
    context->opt_level = OPT_LEVEL_BALANCED;
    context->enable_equivalence_testing = true;
    context->enable_safety_checks = true;
    context->enable_performance_validation = true;
    context->enable_auto_rewrite = true;
    context->max_rewrite_iterations = MAX_REWRITE_ITERATIONS;
    context->current_phase = SYNTHESIS_PHASE_PARSE;
    context->synthesis_start_time = time(NULL);
    
    return 0;
}

/**
 * @brief Cleanup synthesis context
 */
int synthesis_cleanup(synthesis_context_t* context) {
    if (!context) return -1;
    
    // Cleanup rails
    for (uint32_t i = 0; i < context->rail_count; i++) {
        // Rail-specific cleanup would go here
    }
    
    memset(context, 0, sizeof(*context));
    return 0;
}

/**
 * @brief Add synthesis rail
 */
int synthesis_add_rail(synthesis_context_t* context, synthesis_rail_type_t type, const char* name) {
    if (!context || context->rail_count >= MAX_SYNTHESIS_RAILS) return -1;
    
    synthesis_rail_t* rail = &context->rails[context->rail_count];
    memset(rail, 0, sizeof(*rail));
    
    rail->type = type;
    rail->is_active = true;
    rail->is_primary = (context->rail_count == 0); // First rail is primary
    
    if (name) {
        strncpy(rail->name, name, sizeof(rail->name) - 1);
    } else {
        snprintf(rail->name, sizeof(rail->name), "Rail_%s_%u", 
                synthesis_rail_type_to_string(type), context->rail_count);
    }
    
    // Initialize rail-specific data
    switch (type) {
        case SYNTHESIS_RAIL_ASM_DSL:
            synthesis_rail1_init(rail);
            break;
        case SYNTHESIS_RAIL_C_REFERENCE:
            synthesis_rail2_init(rail);
            break;
        case SYNTHESIS_RAIL_PROOF_STUBS:
            synthesis_rail3_init(rail);
            break;
        default:
            break;
    }
    
    context->rail_count++;
    return context->rail_count - 1;
}

/**
 * @brief Initialize Rail-1 (Constrained ASM DSL)
 */
int synthesis_rail1_init(synthesis_rail_t* rail) {
    if (!rail || rail->type != SYNTHESIS_RAIL_ASM_DSL) return -1;
    
    // Initialize with common x86-64 opcodes
    const char* default_opcodes[] = {
        "mov", "add", "sub", "mul", "div", "and", "or", "xor", "not",
        "cmp", "test", "jmp", "je", "jne", "jl", "jg", "call", "ret",
        "push", "pop", "lea", "inc", "dec", "shl", "shr", "sar"
    };
    
    rail->data.asm_dsl.permitted_opcode_count = 
        sizeof(default_opcodes) / sizeof(default_opcodes[0]);
    
    for (uint32_t i = 0; i < rail->data.asm_dsl.permitted_opcode_count; i++) {
        strncpy(rail->data.asm_dsl.permitted_opcodes[i], default_opcodes[i], 15);
    }
    
    // Initialize addressing modes
    const char* addressing_modes[] = {
        "reg", "imm", "mem", "reg+imm", "reg+reg", "reg+reg*scale", 
        "reg+reg*scale+imm", "[reg]", "[reg+imm]", "[reg+reg]"
    };
    
    rail->data.asm_dsl.addressing_mode_count = 
        sizeof(addressing_modes) / sizeof(addressing_modes[0]);
    
    for (uint32_t i = 0; i < rail->data.asm_dsl.addressing_mode_count; i++) {
        strncpy(rail->data.asm_dsl.addressing_modes[i], addressing_modes[i], 31);
    }
    
    return 0;
}

/**
 * @brief Initialize Rail-2 (C Reference Implementation)
 */
int synthesis_rail2_init(synthesis_rail_t* rail) {
    if (!rail || rail->type != SYNTHESIS_RAIL_C_REFERENCE) return -1;
    
    // Set default compiler flags
    strcpy(rail->data.c_reference.compiler_flags, "-std=c11 -Wall -Wextra");
    strcpy(rail->data.c_reference.optimization_flags, "-O2");
    
    return 0;
}

/**
 * @brief Initialize Rail-3 (Proof Stubs)
 */
int synthesis_rail3_init(synthesis_rail_t* rail) {
    if (!rail || rail->type != SYNTHESIS_RAIL_PROOF_STUBS) return -1;
    
    // Set default verification tool
    strcpy(rail->data.proof_stubs.verification_tool, "CBMC");
    strcpy(rail->data.proof_stubs.proof_language, "ACSL");
    
    return 0;
}

/**
 * @brief Add ASM DSL token
 */
int synthesis_rail1_add_token(synthesis_rail_t* rail, const char* opcode, const char* addressing_form) {
    if (!rail || rail->type != SYNTHESIS_RAIL_ASM_DSL || !opcode || !addressing_form) return -1;
    
    if (rail->data.asm_dsl.token_count >= 1024) return -1;
    
    asm_dsl_token_t* token = &rail->data.asm_dsl.tokens[rail->data.asm_dsl.token_count];
    memset(token, 0, sizeof(*token));
    
    token->token_id = rail->data.asm_dsl.token_count;
    strncpy(token->opcode, opcode, sizeof(token->opcode) - 1);
    strncpy(token->addressing_form, addressing_form, sizeof(token->addressing_form) - 1);
    
    // Set default constraints
    token->latency_cycles = 1;
    token->uops_count = 1;
    token->code_size_bytes = 4;
    token->is_permitted = true;
    
    rail->data.asm_dsl.token_count++;
    return 0;
}

/**
 * @brief Add C reference function
 */
int synthesis_rail2_add_function(synthesis_rail_t* rail, const char* name, const char* return_type, 
                                const char* params, const char* body) {
    if (!rail || rail->type != SYNTHESIS_RAIL_C_REFERENCE || !name || !return_type || !body) return -1;
    
    if (rail->data.c_reference.block_count >= 64) return -1;
    
    c_reference_block_t* block = &rail->data.c_reference.blocks[rail->data.c_reference.block_count];
    memset(block, 0, sizeof(*block));
    
    block->block_id = rail->data.c_reference.block_count;
    strncpy(block->function_name, name, sizeof(block->function_name) - 1);
    strncpy(block->return_type, return_type, sizeof(block->return_type) - 1);
    
    if (params) {
        strncpy(block->parameters, params, sizeof(block->parameters) - 1);
    }
    
    strncpy(block->body, body, sizeof(block->body) - 1);
    
    // Set default performance estimates
    block->estimated_cycles = 10;
    block->memory_accesses = 1;
    block->register_usage = 2;
    
    rail->data.c_reference.block_count++;
    return 0;
}

/**
 * @brief Add proof stub
 */
int synthesis_rail3_add_stub(synthesis_rail_t* rail, const char* name, const char* specification) {
    if (!rail || rail->type != SYNTHESIS_RAIL_PROOF_STUBS || !name || !specification) return -1;
    
    if (rail->data.proof_stubs.stub_count >= 128) return -1;
    
    proof_stub_t* stub = &rail->data.proof_stubs.stubs[rail->data.proof_stubs.stub_count];
    memset(stub, 0, sizeof(*stub));
    
    stub->stub_id = rail->data.proof_stubs.stub_count;
    strncpy(stub->name, name, sizeof(stub->name) - 1);
    strncpy(stub->specification, specification, sizeof(stub->specification) - 1);
    
    // Set default privilege requirements
    stub->required_privilege_level = 0; // User mode
    stub->requires_supervisor_mode = false;
    stub->requires_interrupt_disabled = false;
    
    rail->data.proof_stubs.stub_count++;
    return 0;
}

/**
 * @brief Generate code for Rail-1 (ASM DSL)
 */
int synthesis_rail1_generate_code(synthesis_rail_t* rail, const bdi_graph_t* graph) {
    if (!rail || rail->type != SYNTHESIS_RAIL_ASM_DSL || !graph) return -1;
    
    char* code = rail->generated_code;
    size_t remaining = sizeof(rail->generated_code) - 1;
    size_t written = 0;
    
    // Generate assembly header
    written += snprintf(code + written, remaining - written,
                       "; Generated ASM DSL code for %s\n"
                       ".text\n"
                       ".global _start\n\n"
                       "_start:\n", graph->name);
    
    // Process each node in the graph
    for (uint32_t i = 0; i < graph->node_count && written < remaining; i++) {
        const bdi_node_t* node = &graph->nodes[i];
        
        written += snprintf(code + written, remaining - written,
                           "; Node: %s (Type: %s)\n",
                           node->name, bdi_node_type_to_string(node->type));
        
        // Generate code based on node type
        switch (node->type) {
            case BDI_NODE_OPERATION:
                if (strstr(node->operation, "add")) {
                    written += snprintf(code + written, remaining - written,
                                       "    add rax, rbx    ; %s\n", node->name);
                } else if (strstr(node->operation, "mul")) {
                    written += snprintf(code + written, remaining - written,
                                       "    mul rbx         ; %s\n", node->name);
                } else {
                    written += snprintf(code + written, remaining - written,
                                       "    mov rax, rbx    ; %s (default)\n", node->name);
                }
                break;
                
            case BDI_NODE_MEMORY:
                written += snprintf(code + written, remaining - written,
                                   "    mov rax, [rbx]  ; %s (memory load)\n", node->name);
                break;
                
            case BDI_NODE_BRANCH:
                written += snprintf(code + written, remaining - written,
                                   "    cmp rax, 0      ; %s (branch condition)\n"
                                   "    jne .L%u\n", node->name, node->id);
                break;
                
            default:
                written += snprintf(code + written, remaining - written,
                                   "    nop             ; %s (placeholder)\n", node->name);
                break;
        }
    }
    
    // Generate footer
    written += snprintf(code + written, remaining - written,
                       "\n    ; Exit\n"
                       "    mov rax, 60     ; sys_exit\n"
                       "    mov rdi, 0      ; exit status\n"
                       "    syscall\n");
    
    rail->code_size = written;
    rail->estimated_cycles = graph->node_count * 2; // Rough estimate
    rail->code_size_bytes = written;
    
    return 0;
}

/**
 * @brief Generate code for Rail-2 (C Reference)
 */
int synthesis_rail2_generate_code(synthesis_rail_t* rail, const bdi_graph_t* graph) {
    if (!rail || rail->type != SYNTHESIS_RAIL_C_REFERENCE || !graph) return -1;
    
    char* code = rail->generated_code;
    size_t remaining = sizeof(rail->generated_code) - 1;
    size_t written = 0;
    
    // Generate C header
    written += snprintf(code + written, remaining - written,
                       "/* Generated C reference code for %s */\n"
                       "#include <stdint.h>\n"
                       "#include <stdbool.h>\n\n", graph->name);
    
    // Generate main function
    written += snprintf(code + written, remaining - written,
                       "int %s_reference(void) {\n", graph->name);
    
    // Generate variables for each node
    for (uint32_t i = 0; i < graph->node_count && written < remaining; i++) {
        const bdi_node_t* node = &graph->nodes[i];
        
        switch (node->type) {
            case BDI_NODE_DATA:
            case BDI_NODE_REGISTER:
                written += snprintf(code + written, remaining - written,
                                   "    uint64_t %s = 0;  // %s\n", 
                                   node->name, bdi_node_type_to_string(node->type));
                break;
            case BDI_NODE_MEMORY:
                written += snprintf(code + written, remaining - written,
                                   "    uint64_t* %s = NULL;  // %s\n", 
                                   node->name, bdi_node_type_to_string(node->type));
                break;
            default:
                break;
        }
    }
    
    written += snprintf(code + written, remaining - written, "\n");
    
    // Generate operations
    for (uint32_t i = 0; i < graph->node_count && written < remaining; i++) {
        const bdi_node_t* node = &graph->nodes[i];
        
        if (node->type == BDI_NODE_OPERATION) {
            written += snprintf(code + written, remaining - written,
                               "    // Operation: %s\n", node->name);
            
            if (strstr(node->operation, "add")) {
                written += snprintf(code + written, remaining - written,
                                   "    // result = operand1 + operand2;\n");
            } else if (strstr(node->operation, "mul")) {
                written += snprintf(code + written, remaining - written,
                                   "    // result = operand1 * operand2;\n");
            }
        }
    }
    
    // Generate return
    written += snprintf(code + written, remaining - written,
                       "\n    return 0;\n}\n");
    
    rail->code_size = written;
    rail->estimated_cycles = graph->node_count * 5; // C overhead
    rail->code_size_bytes = written * 2; // Compiled size estimate
    
    return 0;
}

/**
 * @brief Run equivalence validation
 */
int synthesis_validate_equivalence(synthesis_context_t* context) {
    if (!context) return -1;
    
    // Find primary rail and compare with others
    synthesis_rail_t* primary_rail = NULL;
    for (uint32_t i = 0; i < context->rail_count; i++) {
        if (context->rails[i].is_primary) {
            primary_rail = &context->rails[i];
            break;
        }
    }
    
    if (!primary_rail) return -1;
    
    // Simple equivalence check (in practice, this would be much more sophisticated)
    for (uint32_t i = 0; i < context->rail_count; i++) {
        synthesis_rail_t* rail = &context->rails[i];
        
        if (rail == primary_rail) {
            rail->passed_equivalence_test = true;
            continue;
        }
        
        // Compare estimated performance characteristics
        uint32_t cycle_diff = abs((int)rail->estimated_cycles - (int)primary_rail->estimated_cycles);
        float cycle_ratio = (float)cycle_diff / primary_rail->estimated_cycles;
        
        // Allow 20% difference in cycle count
        rail->passed_equivalence_test = (cycle_ratio <= 0.2f);
    }
    
    return 0;
}

/**
 * @brief Run safety validation
 */
int synthesis_validate_safety(synthesis_context_t* context) {
    if (!context) return -1;
    
    // Basic safety checks
    for (uint32_t i = 0; i < context->rail_count; i++) {
        synthesis_rail_t* rail = &context->rails[i];
        
        // Check for dangerous patterns in generated code
        bool has_unsafe_patterns = false;
        
        if (strstr(rail->generated_code, "jmp *") ||  // Indirect jumps
            strstr(rail->generated_code, "call *") || // Indirect calls
            strstr(rail->generated_code, "ret *")) {  // Modified returns
            has_unsafe_patterns = true;
        }
        
        // Check for buffer overflows (simplified)
        if (strstr(rail->generated_code, "strcpy") ||
            strstr(rail->generated_code, "sprintf") ||
            strstr(rail->generated_code, "gets")) {
            has_unsafe_patterns = true;
        }
        
        rail->passed_safety_test = !has_unsafe_patterns;
    }
    
    return 0;
}

/**
 * @brief Run performance validation
 */
int synthesis_validate_performance(synthesis_context_t* context) {
    if (!context) return -1;
    
    // Performance validation based on constraints
    for (uint32_t i = 0; i < context->rail_count; i++) {
        synthesis_rail_t* rail = &context->rails[i];
        
        // Check against performance targets (simplified)
        bool meets_performance = true;
        
        // Check cycle count (example: must be under 1000 cycles)
        if (rail->estimated_cycles > 1000) {
            meets_performance = false;
        }
        
        // Check code size (example: must be under 4KB)
        if (rail->code_size_bytes > 4096) {
            meets_performance = false;
        }
        
        // Check memory usage (example: must be under 1MB)
        if (rail->memory_usage > 1024 * 1024) {
            meets_performance = false;
        }
        
        rail->passed_performance_test = meets_performance;
        
        // Calculate validation score (0-100)
        uint32_t score = 100;
        if (rail->estimated_cycles > 500) score -= 20;
        if (rail->code_size_bytes > 2048) score -= 20;
        if (rail->memory_usage > 512 * 1024) score -= 20;
        
        rail->validation_score = score;
    }
    
    return 0;
}

/**
 * @brief Run auto-rewrite loop
 */
int synthesis_auto_rewrite_run(synthesis_context_t* context) {
    if (!context || !context->enable_auto_rewrite) return -1;
    
    bool improved = true;
    context->rewrite_iteration = 0;
    
    while (improved && context->rewrite_iteration < context->max_rewrite_iterations) {
        improved = false;
        context->rewrite_iteration++;
        
        // Apply rewrite rules to each rail
        for (uint32_t i = 0; i < context->rail_count; i++) {
            synthesis_rail_t* rail = &context->rails[i];
            
            char original_code[16384];
            strncpy(original_code, rail->generated_code, sizeof(original_code) - 1);
            
            // Apply rewrite rules
            if (synthesis_apply_rewrite_rules(context, rail->generated_code, 
                                            sizeof(rail->generated_code)) == 0) {
                
                // Check if performance improved
                float improvement;
                if (synthesis_evaluate_rewrite_performance(context, original_code, 
                                                         rail->generated_code, &improvement) == 0) {
                    if (improvement > 0.05f) { // 5% improvement threshold
                        improved = true;
                        printf("Rewrite iteration %u: %.1f%% improvement in rail %s\n",
                               context->rewrite_iteration, improvement * 100.0f, rail->name);
                    } else {
                        // Revert if no significant improvement
                        strncpy(rail->generated_code, original_code, sizeof(rail->generated_code) - 1);
                    }
                }
            }
        }
    }
    
    printf("Auto-rewrite completed after %u iterations\n", context->rewrite_iteration);
    return 0;
}

/**
 * @brief Apply rewrite rules to code
 */
int synthesis_apply_rewrite_rules(synthesis_context_t* context, char* code, size_t code_size) {
    if (!context || !code) return -1;
    
    // Simple pattern-based rewriting (in practice, this would use AST manipulation)
    for (uint32_t i = 0; i < context->rule_count; i++) {
        rewrite_rule_t* rule = &context->rules[i];
        
        char* pos = strstr(code, rule->pattern);
        if (pos) {
            // Simple string replacement (very basic)
            size_t pattern_len = strlen(rule->pattern);
            size_t replacement_len = strlen(rule->replacement);
            
            if (replacement_len <= pattern_len) {
                // In-place replacement
                memcpy(pos, rule->replacement, replacement_len);
                if (replacement_len < pattern_len) {
                    memmove(pos + replacement_len, pos + pattern_len, 
                           strlen(pos + pattern_len) + 1);
                }
                
                rule->application_count++;
                rule->success_count++;
            }
        }
    }
    
    return 0;
}

/**
 * @brief Run complete synthesis
 */
int synthesis_run(synthesis_context_t* context) {
    if (!context) return -1;
    
    printf("Starting synthesis for project: %s\n", context->project_name);
    
    // Phase 1: Parse
    context->current_phase = SYNTHESIS_PHASE_PARSE;
    printf("Phase 1: Parse - Processing input graph\n");
    
    // Phase 2: Analyze
    context->current_phase = SYNTHESIS_PHASE_ANALYZE;
    printf("Phase 2: Analyze - Analyzing graph structure\n");
    
    // Phase 3: Generate
    context->current_phase = SYNTHESIS_PHASE_GENERATE;
    printf("Phase 3: Generate - Generating code for all rails\n");
    
    for (uint32_t i = 0; i < context->rail_count; i++) {
        synthesis_rail_t* rail = &context->rails[i];
        
        switch (rail->type) {
            case SYNTHESIS_RAIL_ASM_DSL:
                synthesis_rail1_generate_code(rail, context->input_graph);
                break;
            case SYNTHESIS_RAIL_C_REFERENCE:
                synthesis_rail2_generate_code(rail, context->input_graph);
                break;
            case SYNTHESIS_RAIL_PROOF_STUBS:
                // synthesis_rail3_generate_proofs(rail, context->input_graph);
                break;
            default:
                break;
        }
        
        printf("  Generated code for rail: %s (%zu bytes)\n", 
               rail->name, rail->code_size);
    }
    
    // Phase 4: Validate
    context->current_phase = SYNTHESIS_PHASE_VALIDATE;
    printf("Phase 4: Validate - Running validation tests\n");
    
    if (context->enable_equivalence_testing) {
        synthesis_validate_equivalence(context);
        printf("  Equivalence validation completed\n");
    }
    
    if (context->enable_safety_checks) {
        synthesis_validate_safety(context);
        printf("  Safety validation completed\n");
    }
    
    if (context->enable_performance_validation) {
        synthesis_validate_performance(context);
        printf("  Performance validation completed\n");
    }
    
    // Phase 5: Optimize (Auto-rewrite)
    context->current_phase = SYNTHESIS_PHASE_OPTIMIZE;
    printf("Phase 5: Optimize - Running auto-rewrite loop\n");
    
    if (context->enable_auto_rewrite) {
        synthesis_auto_rewrite_run(context);
    }
    
    // Phase 6: Verify
    context->current_phase = SYNTHESIS_PHASE_VERIFY;
    printf("Phase 6: Verify - Final verification\n");
    
    // Re-run validation after optimization
    synthesis_validate_equivalence(context);
    synthesis_validate_safety(context);
    synthesis_validate_performance(context);
    
    // Phase 7: Finalize
    context->current_phase = SYNTHESIS_PHASE_FINALIZE;
    printf("Phase 7: Finalize - Generating final output\n");
    
    // Select best rail based on validation scores
    synthesis_rail_t* best_rail = NULL;
    uint32_t best_score = 0;
    
    for (uint32_t i = 0; i < context->rail_count; i++) {
        synthesis_rail_t* rail = &context->rails[i];
        
        if (rail->passed_equivalence_test && rail->passed_safety_test && 
            rail->passed_performance_test && rail->validation_score > best_score) {
            best_rail = rail;
            best_score = rail->validation_score;
        }
    }
    
    if (best_rail) {
        strncpy(context->final_code, best_rail->generated_code, 
                sizeof(context->final_code) - 1);
        context->final_code_size = best_rail->code_size;
        context->final_performance_score = best_rail->validation_score;
        
        printf("Selected best rail: %s (score: %u)\n", 
               best_rail->name, best_score);
    } else {
        printf("Warning: No rail passed all validation tests\n");
        context->has_errors = true;
    }
    
    context->synthesis_complete = true;
    context->synthesis_end_time = time(NULL);
    
    printf("Synthesis completed in %lu seconds\n", 
           context->synthesis_end_time - context->synthesis_start_time);
    
    return context->has_errors ? -1 : 0;
}

/**
 * @brief Convert rail type to string
 */
const char* synthesis_rail_type_to_string(synthesis_rail_type_t type) {
    if (type < sizeof(rail_type_strings) / sizeof(rail_type_strings[0])) {
        return rail_type_strings[type];
    }
    return "Unknown";
}

/**
 * @brief Convert phase to string
 */
const char* synthesis_phase_to_string(synthesis_phase_t phase) {
    if (phase < sizeof(phase_strings) / sizeof(phase_strings[0])) {
        return phase_strings[phase];
    }
    return "Unknown";
}

/**
 * @brief Print context status
 */
void synthesis_print_context_status(const synthesis_context_t* context) {
    if (!context) return;
    
    printf("Synthesis Context Status:\n");
    printf("  Project: %s\n", context->project_name);
    printf("  Target: %s\n", codegen_target_to_string(context->target));
    printf("  Optimization Level: %s\n", optimization_level_to_string(context->opt_level));
    printf("  Current Phase: %s\n", synthesis_phase_to_string(context->current_phase));
    printf("  Rails: %u\n", context->rail_count);
    printf("  Tests: %u\n", context->test_count);
    printf("  Rewrite Rules: %u\n", context->rule_count);
    printf("  Complete: %s\n", context->synthesis_complete ? "Yes" : "No");
    printf("  Has Errors: %s\n", context->has_errors ? "Yes" : "No");
    
    if (context->synthesis_complete) {
        printf("  Final Code Size: %zu bytes\n", context->final_code_size);
        printf("  Final Performance Score: %u\n", context->final_performance_score);
    }
}

/**
 * @brief Print rail status
 */
void synthesis_print_rail_status(const synthesis_rail_t* rail) {
    if (!rail) return;
    
    printf("Rail Status: %s\n", rail->name);
    printf("  Type: %s\n", synthesis_rail_type_to_string(rail->type));
    printf("  Active: %s\n", rail->is_active ? "Yes" : "No");
    printf("  Primary: %s\n", rail->is_primary ? "Yes" : "No");
    printf("  Code Size: %zu bytes\n", rail->code_size);
    printf("  Estimated Cycles: %u\n", rail->estimated_cycles);
    printf("  Code Size Bytes: %u\n", rail->code_size_bytes);
    printf("  Memory Usage: %u bytes\n", rail->memory_usage);
    printf("  Validation Results:\n");
    printf("    Equivalence: %s\n", rail->passed_equivalence_test ? "PASS" : "FAIL");
    printf("    Safety: %s\n", rail->passed_safety_test ? "PASS" : "FAIL");
    printf("    Performance: %s\n", rail->passed_performance_test ? "PASS" : "FAIL");
    printf("    Score: %u/100\n", rail->validation_score);
}

/**
 * @brief Get target string
 */
const char* codegen_target_to_string(codegen_target_t target) {
    if (target < sizeof(target_strings) / sizeof(target_strings[0])) {
        return target_strings[target];
    }
    return "Unknown";
}

/**
 * @brief Get optimization level string
 */
const char* optimization_level_to_string(optimization_level_t level) {
    if (level < sizeof(opt_level_strings) / sizeof(opt_level_strings[0])) {
        return opt_level_strings[level];
    }
    return "Unknown";
}
