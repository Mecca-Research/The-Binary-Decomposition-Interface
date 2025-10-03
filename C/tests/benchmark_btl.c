// BTL Performance Benchmarks
#include "../btl/btl_regalloc.h"
#include "../btl/btl_scheduler.h"
#include "../btl/btl_peephole.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

void benchmark_regalloc(void) {
    printf("\n=== Register Allocation Benchmarks ===\n");
    
    const int NUM_VARS = 100;
    
    double start = get_time_ms();
    BTL_RegAllocator *alloc = btl_regalloc_create(16, BTL_REG_GENERAL);
    
    for (int i = 0; i < NUM_VARS; i++) {
        btl_regalloc_add_interval(alloc, i, i * 10, i * 10 + 50);
    }
    
    btl_regalloc_linear_scan(alloc);
    double end = get_time_ms();
    
    printf("Linear scan (%d variables): %.2f ms\n", NUM_VARS, end - start);
    printf("Spills: %u\n", btl_regalloc_get_spill_count(alloc));
    
    btl_regalloc_destroy(alloc);
}

void benchmark_scheduler(void) {
    printf("\n=== Instruction Scheduling Benchmarks ===\n");
    
    const int NUM_INSTS = 100;
    
    double start = get_time_ms();
    BTL_Scheduler *sched = btl_scheduler_create();
    
    for (int i = 0; i < NUM_INSTS; i++) {
        btl_scheduler_add_instruction(sched, 0x01 + (i % 10), 1 + (i % 3));
    }
    
    // Add some dependencies
    for (int i = 0; i < NUM_INSTS - 1; i += 2) {
        btl_scheduler_add_dependency(sched, i, i + 1);
    }
    
    btl_scheduler_schedule(sched);
    double end = get_time_ms();
    
    printf("List scheduling (%d instructions): %.2f ms\n", NUM_INSTS, end - start);
    printf("Critical path: %u cycles\n", btl_scheduler_get_critical_path(sched));
    
    btl_scheduler_destroy(sched);
}

void benchmark_peephole(void) {
    printf("\n=== Peephole Optimization Benchmarks ===\n");
    
    const int NUM_INSTS = 1000;
    
    BTL_PeepholeOptimizer *opt = btl_peephole_create();
    btl_peephole_add_rule(opt, &BTL_RULE_REDUNDANT_MOVE);
    btl_peephole_add_rule(opt, &BTL_RULE_STRENGTH_REDUCTION);
    btl_peephole_add_rule(opt, &BTL_RULE_CONSTANT_FOLDING);
    
    BTL_InstructionPattern *input = malloc(NUM_INSTS * sizeof(BTL_InstructionPattern));
    BTL_InstructionPattern *output = malloc(NUM_INSTS * sizeof(BTL_InstructionPattern));
    
    for (int i = 0; i < NUM_INSTS; i++) {
        input[i].opcode = 0x01 + (i % 20);
        input[i].operand1 = i;
        input[i].operand2 = i * 2;
        input[i].wildcard_op1 = false;
        input[i].wildcard_op2 = false;
    }
    
    double start = get_time_ms();
    size_t count = btl_peephole_optimize(opt, input, NUM_INSTS, output, NUM_INSTS);
    double end = get_time_ms();
    
    printf("Peephole optimization (%d instructions): %.2f ms\n", NUM_INSTS, end - start);
    printf("Output instructions: %zu\n", count);
    printf("Rules applied: %zu\n", btl_peephole_get_rules_applied(opt));
    printf("Total savings: %u cycles\n", btl_peephole_get_total_savings(opt));
    
    free(input);
    free(output);
    btl_peephole_destroy(opt);
}

int main(void) {
    printf("=== BTL Performance Benchmarks ===\n");
    
    benchmark_regalloc();
    benchmark_scheduler();
    benchmark_peephole();
    
    return 0;
}
