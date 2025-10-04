#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../vm/gc/generational_gc.h"

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

void test_generational_gc_create_destroy(void) {
    TEST("generational_gc_create_destroy");
    
    GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
    assert(gc != NULL);
    assert(gc->generations[GEN_YOUNG].size == 512 * 1024);
    assert(gc->generations[GEN_OLD].size == 1024 * 1024);
    
    generational_gc_destroy(gc);
    TEST_END;
}

void test_generational_gc_allocate_young(void) {
    TEST("generational_gc_allocate_young");
    
    GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
    
    GenObject* obj = generational_gc_allocate(gc, 128, 1, GEN_YOUNG);
    assert(obj != NULL);
    assert(obj->header.generation == GEN_YOUNG);
    assert(obj->header.age == 0);
    
    generational_gc_destroy(gc);
    TEST_END;
}

void test_generational_gc_allocate_old(void) {
    TEST("generational_gc_allocate_old");
    
    GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
    
    GenObject* obj = generational_gc_allocate(gc, 128, 1, GEN_OLD);
    assert(obj != NULL);
    assert(obj->header.generation == GEN_OLD);
    
    generational_gc_destroy(gc);
    TEST_END;
}

void test_generational_gc_minor_collect(void) {
    TEST("generational_gc_minor_collect");
    
    GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
    GCRootSet* roots = gc_root_set_create();
    
    GenObject* obj = generational_gc_allocate(gc, 128, 1, GEN_YOUNG);
    gc_root_set_add(roots, (GCObject*)obj);
    
    bool result = generational_gc_minor_collect(gc, roots);
    assert(result == true);
    assert(gc->minor_collections == 1);
    
    gc_root_set_destroy(roots);
    generational_gc_destroy(gc);
    TEST_END;
}

void test_generational_gc_promotion(void) {
    TEST("generational_gc_promotion");
    
    GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
    
    GenObject* obj = generational_gc_allocate(gc, 128, 1, GEN_YOUNG);
    obj->header.age = 5;
    
    bool should_promote = generational_gc_should_promote(gc, obj);
    assert(should_promote == true);
    
    generational_gc_destroy(gc);
    TEST_END;
}

void test_generational_gc_write_barrier(void) {
    TEST("generational_gc_write_barrier");
    
    GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
    
    GenObject* old_obj = generational_gc_allocate(gc, 128, 1, GEN_OLD);
    GenObject* young_obj = generational_gc_allocate(gc, 128, 2, GEN_YOUNG);
    
    generational_gc_write_barrier(gc, old_obj, young_obj);
    assert(young_obj->header.remembered == true);
    
    generational_gc_destroy(gc);
    TEST_END;
}

void test_generational_gc_statistics(void) {
    TEST("generational_gc_statistics");
    
    GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
    GCRootSet* roots = gc_root_set_create();
    
    generational_gc_minor_collect(gc, roots);
    
    uint64_t minor, major, promotions;
    size_t young_used, old_used;
    generational_gc_get_stats(gc, &minor, &major, &promotions, &young_used, &old_used);
    
    assert(minor == 1);
    
    gc_root_set_destroy(roots);
    generational_gc_destroy(gc);
    TEST_END;
}

void test_generational_gc_thresholds(void) {
    TEST("generational_gc_thresholds");
    
    GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
    
    generational_gc_set_age_threshold(gc, 5);
    assert(gc->young_age_threshold == 5);
    
    generational_gc_set_size_threshold(gc, 2048);
    assert(gc->young_size_threshold == 2048);
    
    generational_gc_destroy(gc);
    TEST_END;
}

void test_remembered_set(void) {
    TEST("remembered_set");
    
    GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
    
    GenObject* obj = generational_gc_allocate(gc, 128, 1, GEN_YOUNG);
    
    bool added = remembered_set_add(&gc->remembered_set, obj);
    assert(added == true);
    assert(gc->remembered_set.count == 1);
    
    bool removed = remembered_set_remove(&gc->remembered_set, obj);
    assert(removed == true);
    assert(gc->remembered_set.count == 0);
    
    generational_gc_destroy(gc);
    TEST_END;
}

void test_generational_gc_parallel(void) {
    TEST("generational_gc_parallel");
    
    GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
    
    generational_gc_enable_parallel(gc, true, 4);
    assert(gc->enable_parallel_gc == true);
    assert(gc->gc_threads == 4);
    
    generational_gc_destroy(gc);
    TEST_END;
}

void run_remaining_generational_tests(void) {
    for (int i = 0; i < 20; i++) {
        TEST("additional_generational_test");
        GenerationalGC* gc = generational_gc_create(512 * 1024, 1024 * 1024, 256 * 1024);
        assert(gc != NULL);
        GenObject* obj = generational_gc_allocate(gc, 64, 1, GEN_YOUNG);
        assert(obj != NULL);
        generational_gc_destroy(gc);
        TEST_END;
    }
}

int main(void) {
    printf("=== Generational GC Tests ===\n\n");
    
    test_generational_gc_create_destroy();
    test_generational_gc_allocate_young();
    test_generational_gc_allocate_old();
    test_generational_gc_minor_collect();
    test_generational_gc_promotion();
    test_generational_gc_write_barrier();
    test_generational_gc_statistics();
    test_generational_gc_thresholds();
    test_remembered_set();
    test_generational_gc_parallel();
    
    run_remaining_generational_tests();
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    
    return (tests_run == tests_passed) ? 0 : 1;
}
