// BTL Instruction Scheduler Tests
#include "../btl/btl_scheduler.h"
#include <stdio.h>
#include <assert.h>

static int tests_passed = 0;

#define TEST(name) printf("Testing %s... ", name); fflush(stdout);
#define PASS() printf("PASS\n"); tests_passed++;

void test_scheduler_creation(void) {
    TEST("scheduler creation");
    
    BTL_Scheduler *sched = btl_scheduler_create();
    assert(sched != NULL);
    assert(btl_scheduler_get_instruction_count(sched) == 0);
    
    btl_scheduler_destroy(sched);
    PASS();
}

void test_add_instructions(void) {
    TEST("add instructions");
    
    BTL_Scheduler *sched = btl_scheduler_create();
    
    uint32_t id1 = btl_scheduler_add_instruction(sched, 0x01, 1);
    uint32_t id2 = btl_scheduler_add_instruction(sched, 0x89, 1);
    uint32_t id3 = btl_scheduler_add_instruction(sched, 0xC3, 1);
    
    assert(id1 == 0);
    assert(id2 == 1);
    assert(id3 == 2);
    assert(btl_scheduler_get_instruction_count(sched) == 3);
    
    btl_scheduler_destroy(sched);
    PASS();
}

void test_dependencies(void) {
    TEST("dependency graph");
    
    BTL_Scheduler *sched = btl_scheduler_create();
    
    uint32_t id1 = btl_scheduler_add_instruction(sched, 0x01, 2);
    uint32_t id2 = btl_scheduler_add_instruction(sched, 0x89, 1);
    uint32_t id3 = btl_scheduler_add_instruction(sched, 0x31, 1);
    
    btl_scheduler_add_dependency(sched, id1, id2);
    btl_scheduler_add_dependency(sched, id2, id3);
    
    bool success = btl_scheduler_build_graph(sched);
    assert(success);
    
    btl_scheduler_destroy(sched);
    PASS();
}

void test_scheduling(void) {
    TEST("instruction scheduling");
    
    BTL_Scheduler *sched = btl_scheduler_create();
    
    uint32_t id1 = btl_scheduler_add_instruction(sched, 0x01, 2);
    uint32_t id2 = btl_scheduler_add_instruction(sched, 0x89, 1);
    uint32_t id3 = btl_scheduler_add_instruction(sched, 0x31, 1);
    
    btl_scheduler_add_dependency(sched, id1, id2);
    
    bool success = btl_scheduler_schedule(sched);
    assert(success);
    
    size_t count;
    const uint32_t *schedule = btl_scheduler_get_schedule(sched, &count);
    assert(schedule != NULL);
    assert(count == 3);
    
    btl_scheduler_destroy(sched);
    PASS();
}

void test_critical_path(void) {
    TEST("critical path calculation");
    
    BTL_Scheduler *sched = btl_scheduler_create();
    
    uint32_t id1 = btl_scheduler_add_instruction(sched, 0x01, 3);
    uint32_t id2 = btl_scheduler_add_instruction(sched, 0x89, 2);
    uint32_t id3 = btl_scheduler_add_instruction(sched, 0x31, 1);
    
    btl_scheduler_add_dependency(sched, id1, id2);
    btl_scheduler_add_dependency(sched, id2, id3);
    
    btl_scheduler_build_graph(sched);
    
    uint32_t critical_path = btl_scheduler_get_critical_path(sched);
    assert(critical_path == 6); // 3 + 2 + 1
    
    btl_scheduler_destroy(sched);
    PASS();
}

int main(void) {
    printf("=== BTL Instruction Scheduler Tests ===\n\n");
    
    test_scheduler_creation();
    test_add_instructions();
    test_dependencies();
    test_scheduling();
    test_critical_path();
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    return 0;
}
