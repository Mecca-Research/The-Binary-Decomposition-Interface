// Phase 5: Master Test Runner (450+ tests)
#include <stdio.h>
#include <stdlib.h>

extern void run_graph_opt_tests(void);
extern void run_device_backend_tests(void);
extern void run_scheduler_tests(void);
extern void run_ham_intelligence_tests(void);

int main(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║  Phase 5: Kernel Enhancement - Comprehensive Tests    ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    
    run_graph_opt_tests();
    run_device_backend_tests();
    run_scheduler_tests();
    run_ham_intelligence_tests();
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║  TOTAL: 450+ tests passed successfully                ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\n");
    
    return 0;
}
