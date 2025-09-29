
// ===================================================================
// BDI Master Memory Manager - Comprehensive Test Suite
// Tests for Phase 2: Advanced systems, toolchain, multi-rail synthesis
// ===================================================================

#include "../master_memory_manager.h"
#include "../x86_advanced.h"
#include "../toolchain.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>

// ===================================================================
// Test Framework
// ===================================================================

typedef struct {
    const char *name;
    bool (*test_func)(void);
    bool passed;
    double execution_time;
} test_case_t;

static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define RUN_TEST(test_func) run_test(#test_func, test_func)

static bool run_test(const char *name, bool (*test_func)(void)) {
    printf("Running test: %s... ", name);
    fflush(stdout);
    
    clock_t start = clock();
    bool result = test_func();
    clock_t end = clock();
    
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    tests_run++;
    if (result) {
        tests_passed++;
        printf("PASSED (%.3fs)\n", time_taken);
    } else {
        tests_failed++;
        printf("FAILED (%.3fs)\n", time_taken);
    }
    
    return result;
}

// ===================================================================
// Core MMM Tests
// ===================================================================

static bool test_mmm_create_destroy(void) {
    bdi_cpu_capabilities_t caps = {0};
    caps.sse = true;
    caps.sse2 = true;
    caps.avx = true;
    
    mmm_master_memory_manager_t *mmm = mmm_create(&caps);
    if (!mmm) return false;
    
    // Verify initialization
    if (mmm->simd_features == MMM_SIMD_NONE) return false;
    if (mmm->task_method != MMM_TASK_SWITCH_SOFTWARE) return false;
    
    mmm_destroy(mmm);
    return true;
}

static bool test_mmm_initialize_shutdown(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm) return false;
    
    bool init_result = mmm_initialize(mmm);
    if (!init_result) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Check that SIMD features were detected
    if (mmm->simd_features == MMM_SIMD_NONE) {
        mmm_destroy(mmm);
        return false;
    }
    
    mmm_shutdown(mmm);
    mmm_destroy(mmm);
    return true;
}

static bool test_mmm_instruction_execution(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm || !mmm_initialize(mmm)) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Test basic instruction execution
    uint8_t mov_instruction[] = {0x89, 0xC3}; // mov ebx, eax
    bool result = mmm_execute_instruction(mmm, MMM_INSTR_DATA_MOVEMENT, 
                                         mov_instruction, sizeof(mov_instruction));
    
    if (!result) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Check performance counters
    uint64_t instructions, cycles;
    mmm_get_performance_stats(mmm, &instructions, &cycles, NULL, NULL);
    
    if (instructions == 0 || cycles == 0) {
        mmm_destroy(mmm);
        return false;
    }
    
    mmm_destroy(mmm);
    return true;
}

static bool test_mmm_instruction_decoding(void) {
    uint8_t test_instructions[][4] = {
        {0x89, 0xC3, 0x00, 0x00},       // mov ebx, eax
        {0x01, 0xD8, 0x00, 0x00},       // add eax, ebx
        {0x0F, 0x10, 0x00, 0x00}        // movups xmm0, [eax]
    };
    
    mmm_instruction_category_t expected_categories[] = {
        MMM_INSTR_DATA_MOVEMENT,
        MMM_INSTR_ARITHMETIC,
        MMM_INSTR_SIMD_SSE
    };
    
    for (int i = 0; i < 3; i++) {
        mmm_instruction_category_t category;
        mmm_addressing_mode_t addressing;
        
        bool result = mmm_decode_instruction(test_instructions[i], 2, 
                                           &category, &addressing);
        
        if (!result || category != expected_categories[i]) {
            return false;
        }
    }
    
    return true;
}

// ===================================================================
// Interrupt Management Tests
// ===================================================================

static void test_interrupt_handler(mmm_interrupt_context_t *ctx) {
    // Simple test interrupt handler
    ctx->rax = 0xDEADBEEF;
}

static bool test_mmm_interrupt_setup(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm || !mmm_initialize(mmm)) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Setup IDT
    bool setup_result = mmm_setup_idt(mmm, 256);
    if (!setup_result) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Install interrupt handler
    bool install_result = mmm_install_interrupt_handler(mmm, 0x20, 
                                                       test_interrupt_handler,
                                                       MMM_IDT_INTERRUPT_GATE_32, 0);
    
    if (!install_result) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Verify IDT entry
    if (!mmm->idt_table || mmm->idt_size != 256) {
        mmm_destroy(mmm);
        return false;
    }
    
    mmm_idt_entry_t *entry = &mmm->idt_table[0x20];
    if (!entry->p || entry->type != MMM_IDT_INTERRUPT_GATE_32) {
        mmm_destroy(mmm);
        return false;
    }
    
    mmm_destroy(mmm);
    return true;
}

// ===================================================================
// Task Switching Tests
// ===================================================================

static void test_task_entry(void) {
    // Simple test task entry point
    while (1) {
        // Task body
    }
}

static bool test_mmm_task_switching(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm || !mmm_initialize(mmm)) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Setup task switching
    bool setup_result = mmm_setup_task_switching(mmm, MMM_TASK_SWITCH_SOFTWARE);
    if (!setup_result) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Create a test task
    mmm_task_context_t task;
    void *stack = malloc(4096);
    if (!stack) {
        mmm_destroy(mmm);
        return false;
    }
    
    bool create_result = mmm_create_task(mmm, &task, test_task_entry, stack, 4096);
    if (!create_result) {
        free(stack);
        mmm_destroy(mmm);
        return false;
    }
    
    // Verify task context
    if (task.task_id == 0 || task.kernel_stack != stack) {
        free(stack);
        mmm_destroy(mmm);
        return false;
    }
    
    free(stack);
    mmm_destroy(mmm);
    return true;
}

// ===================================================================
// SIMD/AVX Tests
// ===================================================================

static bool test_mmm_simd_detection(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm || !mmm_initialize(mmm)) {
        mmm_destroy(mmm);
        return false;
    }
    
    bool detect_result = mmm_detect_simd_features(mmm);
    if (!detect_result) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Should have detected at least SSE on modern systems
    if (!(mmm->simd_features & MMM_SIMD_SSE)) {
        mmm_destroy(mmm);
        return false;
    }
    
    mmm_destroy(mmm);
    return true;
}

static bool test_mmm_simd_optimization(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm || !mmm_initialize(mmm)) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Test SIMD code optimization
    uint8_t input_code[] = {0x89, 0xC3, 0x01, 0xD8}; // mov ebx, eax; add eax, ebx
    void *output_code = NULL;
    size_t output_size = 0;
    
    mmm_simd_hints_t hints = {
        .prefer_avx512 = false,
        .prefer_fma = true,
        .allow_unaligned = true,
        .vector_width = 256,
        .cache_line_size = 64
    };
    
    bool optimize_result = mmm_optimize_simd_code(mmm, input_code, sizeof(input_code),
                                                 &output_code, &output_size, &hints);
    
    if (!optimize_result || !output_code || output_size == 0) {
        mmm_destroy(mmm);
        return false;
    }
    
    free(output_code);
    mmm_destroy(mmm);
    return true;
}

// ===================================================================
// Atomic Operations Tests
// ===================================================================

static bool test_mmm_atomic_operations(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm || !mmm_initialize(mmm)) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Test atomic load/store
    uint64_t test_value = 42;
    uint64_t loaded_value = 0;
    
    bool store_result = mmm_atomic_operation(mmm, MMM_ATOMIC_STORE, &test_value, 
                                           &test_value, NULL, MMM_MEMORY_ORDER_SEQ_CST);
    if (!store_result) {
        mmm_destroy(mmm);
        return false;
    }
    
    bool load_result = mmm_atomic_operation(mmm, MMM_ATOMIC_LOAD, &test_value,
                                          &loaded_value, NULL, MMM_MEMORY_ORDER_SEQ_CST);
    if (!load_result || loaded_value != 42) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Test atomic compare-exchange
    uint64_t expected = 42;
    uint64_t new_value = 84;
    
    bool cas_result = mmm_atomic_operation(mmm, MMM_ATOMIC_COMPARE_EXCHANGE, &test_value,
                                         &new_value, &expected, MMM_MEMORY_ORDER_SEQ_CST);
    if (!cas_result) {
        mmm_destroy(mmm);
        return false;
    }
    
    mmm_destroy(mmm);
    return true;
}

static bool test_mmm_memory_fences(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm || !mmm_initialize(mmm)) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Test different fence types
    mmm_memory_fence(mmm, MMM_FENCE_LOAD);
    mmm_memory_fence(mmm, MMM_FENCE_STORE);
    mmm_memory_fence(mmm, MMM_FENCE_FULL);
    
    // If we get here without crashing, fences work
    mmm_destroy(mmm);
    return true;
}

// ===================================================================
// DMA Management Tests
// ===================================================================

static bool test_mmm_dma_setup(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm || !mmm_initialize(mmm)) {
        mmm_destroy(mmm);
        return false;
    }
    
    bool setup_result = mmm_setup_dma(mmm, 16);
    if (!setup_result) {
        mmm_destroy(mmm);
        return false;
    }
    
    if (!mmm->dma_descriptors || mmm->dma_descriptor_count != 16) {
        mmm_destroy(mmm);
        return false;
    }
    
    mmm_destroy(mmm);
    return true;
}

static bool test_mmm_dma_transfer(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm || !mmm_initialize(mmm) || !mmm_setup_dma(mmm, 16)) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Setup test buffers
    uint8_t src_buffer[1024];
    uint8_t dst_buffer[1024];
    
    // Initialize source buffer
    for (int i = 0; i < 1024; i++) {
        src_buffer[i] = i & 0xFF;
    }
    memset(dst_buffer, 0, sizeof(dst_buffer));
    
    // Setup DMA descriptor
    mmm_dma_descriptor_t descriptor = {
        .src_addr = (uint64_t)src_buffer,
        .dst_addr = (uint64_t)dst_buffer,
        .length = 1024,
        .flags = MMM_DMA_MEMORY_TO_MEMORY,
        .next = NULL
    };
    
    bool transfer_result = mmm_start_dma_transfer(mmm, &descriptor);
    if (!transfer_result) {
        mmm_destroy(mmm);
        return false;
    }
    
    bool wait_result = mmm_wait_dma_completion(mmm, 1000);
    if (!wait_result) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Verify transfer
    if (memcmp(src_buffer, dst_buffer, 1024) != 0) {
        mmm_destroy(mmm);
        return false;
    }
    
    mmm_destroy(mmm);
    return true;
}

// ===================================================================
// Toolchain Tests
// ===================================================================

static bool test_toolchain_create_destroy(void) {
    mmm_toolchain_t *toolchain = mmm_toolchain_create("/tmp");
    if (!toolchain) return false;
    
    if (strcmp(toolchain->working_directory, "/tmp") != 0) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    mmm_toolchain_destroy(toolchain);
    return true;
}

static bool test_toolchain_initialize(void) {
    mmm_toolchain_t *toolchain = mmm_toolchain_create("/tmp");
    if (!toolchain) return false;
    
    bool init_result = mmm_toolchain_initialize(toolchain);
    if (!init_result) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    if (!toolchain->toolchain_initialized) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    mmm_toolchain_shutdown(toolchain);
    mmm_toolchain_destroy(toolchain);
    return true;
}

static bool test_bdi_spec_parsing(void) {
    mmm_toolchain_t *toolchain = mmm_toolchain_create("/tmp");
    if (!toolchain || !mmm_toolchain_initialize(toolchain)) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    const char *test_spec = 
        "# Test BDI specification\n"
        "input_node(latency=1, throughput=2)\n"
        "compute_node(latency=5, throughput=1)\n"
        "output_node(latency=1, throughput=1)\n"
        "input_node -> compute_node [latency=2]\n"
        "compute_node -> output_node [latency=1]\n";
    
    bdi_graph_t *graph = NULL;
    bool parse_result = mmm_parse_bdi_specification(toolchain, test_spec, &graph);
    
    if (!parse_result || !graph) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    // Verify parsed graph
    if (graph->node_count == 0) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    bool validate_result = mmm_validate_bdi_graph(graph);
    if (!validate_result) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    mmm_toolchain_destroy(toolchain);
    return true;
}

static bool test_multi_rail_synthesis(void) {
    mmm_toolchain_t *toolchain = mmm_toolchain_create("/tmp");
    if (!toolchain || !mmm_toolchain_initialize(toolchain)) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    // Create a simple test graph
    bdi_graph_t graph = {0};
    strcpy(graph.name, "test_graph");
    graph.node_count = 3;
    graph.nodes = calloc(3, sizeof(bdi_graph_node_t));
    
    if (!graph.nodes) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    // Setup nodes
    graph.nodes[0].id = 0;
    graph.nodes[0].type = BDI_NODE_INPUT;
    strcpy(graph.nodes[0].name, "input");
    
    graph.nodes[1].id = 1;
    graph.nodes[1].type = BDI_NODE_COMPUTE;
    strcpy(graph.nodes[1].name, "compute");
    
    graph.nodes[2].id = 2;
    graph.nodes[2].type = BDI_NODE_OUTPUT;
    strcpy(graph.nodes[2].name, "output");
    
    bool synthesis_result = mmm_synthesize_all_rails(toolchain, &graph, NULL);
    
    if (!synthesis_result) {
        free(graph.nodes);
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    // Verify synthesis results
    multi_rail_synthesis_t *synthesis = &toolchain->synthesis;
    
    if (synthesis->asm_rail.token_count == 0) {
        free(graph.nodes);
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    if (!synthesis->c_rail.source_code || synthesis->c_rail.source_length == 0) {
        free(graph.nodes);
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    if (!synthesis->proof_rail.proof_code || synthesis->proof_rail.proof_length == 0) {
        free(graph.nodes);
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    free(graph.nodes);
    mmm_toolchain_destroy(toolchain);
    return true;
}

static bool test_complete_workflow(void) {
    mmm_toolchain_t *toolchain = mmm_toolchain_create("/tmp");
    if (!toolchain || !mmm_toolchain_initialize(toolchain)) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    const char *test_spec = 
        "# Simple test specification\n"
        "input_node(latency=1)\n"
        "compute_node(latency=2)\n"
        "output_node(latency=1)\n";
    
    mmm_synthesis_constraints_t constraints = {0};
    constraints.max_latency_cycles = 100;
    constraints.min_uops_per_cycle = 1;
    constraints.max_code_size_bytes = 1024;
    constraints.allow_memory_ops = true;
    constraints.allow_branches = true;
    constraints.allow_simd = true;
    
    void *final_asm = NULL;
    void *final_c = NULL;
    void *final_proof = NULL;
    mmm_validation_result_t validation_result = {0};
    
    bool workflow_result = mmm_run_complete_workflow(toolchain, test_spec, &constraints,
                                                    &final_asm, &final_c, &final_proof,
                                                    &validation_result);
    
    if (!workflow_result) {
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    // Verify outputs
    if (!final_asm || !final_c || !final_proof) {
        free(final_asm);
        free(final_c);
        free(final_proof);
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    // Verify validation result
    if (!validation_result.passed) {
        free(final_asm);
        free(final_c);
        free(final_proof);
        mmm_toolchain_destroy(toolchain);
        return false;
    }
    
    free(final_asm);
    free(final_c);
    free(final_proof);
    mmm_toolchain_destroy(toolchain);
    return true;
}

// ===================================================================
// Performance Tests
// ===================================================================

static bool test_performance_counters(void) {
    mmm_master_memory_manager_t *mmm = mmm_create(NULL);
    if (!mmm || !mmm_initialize(mmm)) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Reset counters
    mmm_reset_performance_counters(mmm);
    
    uint64_t instructions, cycles, cache_misses, tlb_misses;
    mmm_get_performance_stats(mmm, &instructions, &cycles, &cache_misses, &tlb_misses);
    
    if (instructions != 0 || cycles != 0 || cache_misses != 0 || tlb_misses != 0) {
        mmm_destroy(mmm);
        return false;
    }
    
    // Execute some instructions
    uint8_t test_instruction[] = {0x89, 0xC3}; // mov ebx, eax
    for (int i = 0; i < 10; i++) {
        mmm_execute_instruction(mmm, MMM_INSTR_DATA_MOVEMENT, 
                               test_instruction, sizeof(test_instruction));
    }
    
    mmm_get_performance_stats(mmm, &instructions, &cycles, &cache_misses, &tlb_misses);
    
    if (instructions != 10 || cycles == 0) {
        mmm_destroy(mmm);
        return false;
    }
    
    mmm_destroy(mmm);
    return true;
}

// ===================================================================
// Main Test Runner
// ===================================================================

int main(void) {
    printf("BDI Master Memory Manager - Phase 2 Test Suite\n");
    printf("===============================================\n\n");
    
    // Core MMM tests
    printf("Core MMM Tests:\n");
    RUN_TEST(test_mmm_create_destroy);
    RUN_TEST(test_mmm_initialize_shutdown);
    RUN_TEST(test_mmm_instruction_execution);
    RUN_TEST(test_mmm_instruction_decoding);
    printf("\n");
    
    // Interrupt management tests
    printf("Interrupt Management Tests:\n");
    RUN_TEST(test_mmm_interrupt_setup);
    printf("\n");
    
    // Task switching tests
    printf("Task Switching Tests:\n");
    RUN_TEST(test_mmm_task_switching);
    printf("\n");
    
    // SIMD/AVX tests
    printf("SIMD/AVX Tests:\n");
    RUN_TEST(test_mmm_simd_detection);
    RUN_TEST(test_mmm_simd_optimization);
    printf("\n");
    
    // Atomic operations tests
    printf("Atomic Operations Tests:\n");
    RUN_TEST(test_mmm_atomic_operations);
    RUN_TEST(test_mmm_memory_fences);
    printf("\n");
    
    // DMA management tests
    printf("DMA Management Tests:\n");
    RUN_TEST(test_mmm_dma_setup);
    RUN_TEST(test_mmm_dma_transfer);
    printf("\n");
    
    // Toolchain tests
    printf("Toolchain Tests:\n");
    RUN_TEST(test_toolchain_create_destroy);
    RUN_TEST(test_toolchain_initialize);
    RUN_TEST(test_bdi_spec_parsing);
    RUN_TEST(test_multi_rail_synthesis);
    RUN_TEST(test_complete_workflow);
    printf("\n");
    
    // Performance tests
    printf("Performance Tests:\n");
    RUN_TEST(test_performance_counters);
    printf("\n");
    
    // Summary
    printf("Test Results:\n");
    printf("=============\n");
    printf("Total tests: %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", (double)tests_passed / tests_run * 100.0);
    
    return (tests_failed == 0) ? 0 : 1;
}
