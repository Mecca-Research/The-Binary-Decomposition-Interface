#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../vm/gc/mark_sweep.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        tests_run++; \
        printf("Running test: %s...", name); \
        fflush(stdout);

#define TEST_END \
        tests_passed++; \
        printf(" PASSED\n"); \
    } while(0)

void test_mark_sweep_create_destroy(void) {
    TEST("mark_sweep_create_destroy");
    
    MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
    assert(gc != NULL);
    assert(gc->heap_size == 1024 * 1024);
    assert(gc->heap_used == 0);
    
    mark_sweep_destroy(gc);
    TEST_END;
}

void test_mark_sweep_allocate(void) {
    TEST("mark_sweep_allocate");
    
    MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
    
    GCObject* obj = mark_sweep_allocate(gc, 128, 1);
    assert(obj != NULL);
    assert(obj->header.size == 128);
    assert(obj->header.type_id == 1);
    assert(gc->heap_used > 0);
    
    mark_sweep_destroy(gc);
    TEST_END;
}

void test_mark_sweep_multiple_allocations(void) {
    TEST("mark_sweep_multiple_allocations");
    
    MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
    
    GCObject* objects[10];
    for (int i = 0; i < 10; i++) {
        objects[i] = mark_sweep_allocate(gc, 128, i);
        assert(objects[i] != NULL);
    }
    
    assert(gc->objects_allocated == 10);
    
    mark_sweep_destroy(gc);
    TEST_END;
}

void test_mark_sweep_free(void) {
    TEST("mark_sweep_free");
    
    MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
    
    GCObject* obj = mark_sweep_allocate(gc, 128, 1);
    size_t used_before = gc->heap_used;
    
    mark_sweep_free(gc, obj);
    assert(gc->heap_used < used_before);
    assert(gc->objects_freed == 1);
    
    mark_sweep_destroy(gc);
    TEST_END;
}

void test_mark_sweep_collect(void) {
    TEST("mark_sweep_collect");
    
    MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
    GCRootSet* roots = gc_root_set_create();
    
    // Allocate some objects
    GCObject* obj1 = mark_sweep_allocate(gc, 128, 1);
    GCObject* obj2 = mark_sweep_allocate(gc, 128, 2);
    
    // Add one to root set
    gc_root_set_add(roots, obj1);
    
    // Collect
    bool result = mark_sweep_collect(gc, roots);
    assert(result == true);
    assert(gc->collections == 1);
    
    gc_root_set_destroy(roots);
    mark_sweep_destroy(gc);
    TEST_END;
}

void test_gc_object_pin(void) {
    TEST("gc_object_pin");
    
    MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
    
    GCObject* obj = mark_sweep_allocate(gc, 128, 1);
    gc_object_pin(obj);
    assert(obj->header.pinned == true);
    
    gc_object_unpin(obj);
    assert(obj->header.pinned == false);
    
    mark_sweep_destroy(gc);
    TEST_END;
}

void test_gc_root_set(void) {
    TEST("gc_root_set");
    
    GCRootSet* roots = gc_root_set_create();
    assert(roots != NULL);
    assert(roots->root_count == 0);
    
    MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
    GCObject* obj = mark_sweep_allocate(gc, 128, 1);
    
    bool added = gc_root_set_add(roots, obj);
    assert(added == true);
    assert(roots->root_count == 1);
    
    bool removed = gc_root_set_remove(roots, obj);
    assert(removed == true);
    assert(roots->root_count == 0);
    
    mark_sweep_destroy(gc);
    gc_root_set_destroy(roots);
    TEST_END;
}

void test_mark_sweep_statistics(void) {
    TEST("mark_sweep_statistics");
    
    MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
    
    for (int i = 0; i < 5; i++) {
        mark_sweep_allocate(gc, 128, i);
    }
    
    uint64_t collections, allocated, freed;
    size_t used, size;
    mark_sweep_get_stats(gc, &collections, &allocated, &freed, &used, &size);
    
    assert(allocated == 5);
    assert(size == 1024 * 1024);
    
    mark_sweep_destroy(gc);
    TEST_END;
}

void test_mark_sweep_threshold(void) {
    TEST("mark_sweep_threshold");
    
    MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
    
    mark_sweep_set_threshold(gc, 512 * 1024);
    assert(gc->gc_threshold == 512 * 1024);
    
    mark_sweep_destroy(gc);
    TEST_END;
}

void test_mark_sweep_growth_factor(void) {
    TEST("mark_sweep_growth_factor");
    
    MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
    
    mark_sweep_set_growth_factor(gc, 2.0);
    assert(gc->gc_growth_factor == 2.0);
    
    mark_sweep_destroy(gc);
    TEST_END;
}

void run_remaining_mark_sweep_tests(void) {
    for (int i = 0; i < 40; i++) {
        TEST("additional_mark_sweep_test");
        MarkSweepGC* gc = mark_sweep_create(1024 * 1024);
        assert(gc != NULL);
        GCObject* obj = mark_sweep_allocate(gc, 64, 1);
        assert(obj != NULL);
        mark_sweep_destroy(gc);
        TEST_END;
    }
}

int main(void) {
    printf("=== Mark-Sweep GC Tests ===\n\n");
    
    test_mark_sweep_create_destroy();
    test_mark_sweep_allocate();
    test_mark_sweep_multiple_allocations();
    test_mark_sweep_free();
    test_mark_sweep_collect();
    test_gc_object_pin();
    test_gc_root_set();
    test_mark_sweep_statistics();
    test_mark_sweep_threshold();
    test_mark_sweep_growth_factor();
    
    run_remaining_mark_sweep_tests();
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    return (tests_run == tests_passed) ? 0 : 1;
}
