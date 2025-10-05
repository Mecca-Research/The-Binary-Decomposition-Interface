
#include "test_framework.h"

// Forward declarations for all test suites
extern test_suite_t vm_test_suite;
extern test_suite_t jit_test_suite;
extern test_suite_t graph_test_suite;
extern test_suite_t memory_test_suite;
extern test_suite_t bytecode_test_suite;
extern test_suite_t values_test_suite;
extern test_suite_t errors_test_suite;

// Integration test suites
extern test_suite_t vm_jit_integration_suite;
extern test_suite_t graph_vm_integration_suite;
extern test_suite_t jit_graph_integration_suite;
extern test_suite_t memory_system_integration_suite;
extern test_suite_t performance_integration_suite;
extern test_suite_t error_handling_integration_suite;

int main(int argc, char* argv[]) {
    test_framework_init();
    
    bool all_passed = true;
    int suites_run = 0;
    int suites_passed = 0;
    
    printf("BDI Kernel Comprehensive Test Suite\n");
    printf("====================================\n");
    
    // Unit test suites (150+ tests)
    printf("\n=== UNIT TESTS ===\n");
    
    // Core VM Tests (30+ tests)
    if (test_framework_run_suite(&vm_test_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // JIT Compiler Tests (25+ tests)
    if (test_framework_run_suite(&jit_test_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Graph Execution Tests (35+ tests)
    if (test_framework_run_suite(&graph_test_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Memory Management Tests (20+ tests)
    if (test_framework_run_suite(&memory_test_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Bytecode System Tests (15+ tests)
    if (test_framework_run_suite(&bytecode_test_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Value System Tests (15+ tests)
    if (test_framework_run_suite(&values_test_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Error Handling Tests (10+ tests)
    if (test_framework_run_suite(&errors_test_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Integration test suites (85+ tests)
    printf("\n=== INTEGRATION TESTS ===\n");
    
    // VM-JIT Integration Tests (20+ tests)
    if (test_framework_run_suite(&vm_jit_integration_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Graph-VM Integration Tests (20+ tests)
    if (test_framework_run_suite(&graph_vm_integration_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // JIT-Graph Integration Tests (15+ tests)
    if (test_framework_run_suite(&jit_graph_integration_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Memory System Integration Tests (10+ tests)
    if (test_framework_run_suite(&memory_system_integration_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Performance Integration Tests (10+ tests)
    if (test_framework_run_suite(&performance_integration_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Error Handling Integration Tests (10+ tests)
    if (test_framework_run_suite(&error_handling_integration_suite)) {
        suites_passed++;
    } else {
        all_passed = false;
    }
    suites_run++;
    
    // Print final summary
    printf("\n=== FINAL RESULTS ===\n");
    printf("Test Suites: %d/%d passed\n", suites_passed, suites_run);
    test_framework_print_summary();
    
    test_framework_cleanup();
    
    return all_passed ? 0 : 1;
}
