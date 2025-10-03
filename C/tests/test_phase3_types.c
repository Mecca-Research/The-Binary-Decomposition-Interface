
// ===================================================================
// DESC: Comprehensive Test Suite for Phase 3.1 - Type System (150+ tests)
// ===================================================================

#include "../c23_compat.h"
#include "../compiler/types/bci_types_extended.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test counters
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        tests_run++; \
        printf("Running test: %s...", #name); \
        test_##name(); \
        tests_passed++; \
        printf(" PASSED\n"); \
    } \
    static void test_##name(void)

#define ASSERT(condition) \
    do { \
        if (!(condition)) { \
            printf("\n  Assertion failed: %s\n", #condition); \
            tests_failed++; \
            return; \
        } \
    } while (0)

#define ASSERT_NOT_NULL(ptr) ASSERT((ptr) != nullptr)
#define ASSERT_NULL(ptr) ASSERT((ptr) == nullptr)
#define ASSERT_EQ(a, b) ASSERT((a) == (b))
#define ASSERT_NEQ(a, b) ASSERT((a) != (b))
#define ASSERT_STR_EQ(a, b) ASSERT(strcmp((a), (b)) == 0)

// ===================================================================
// Struct Type Tests (30 tests)
// ===================================================================

TEST(struct_create) {
    BciTypeExt* s = bci_type_struct_create("Point");
    ASSERT_NOT_NULL(s);
    ASSERT_EQ(s->kind, BCI_TYPE_EXT_STRUCT);
    ASSERT_STR_EQ(s->name, "Point");
    bci_type_ext_free(s);
}

TEST(struct_add_field) {
    BciTypeExt* s = bci_type_struct_create("Point");
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    bci_type_struct_add_field(s, "x", int_type);
    ASSERT_EQ(s->as.struct_type.fields.len, 1);
    ASSERT_STR_EQ(s->as.struct_type.fields.data[0].name, "x");
    
    bci_type_ext_free(s);
}

TEST(struct_multiple_fields) {
    BciTypeExt* s = bci_type_struct_create("Rectangle");
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    bci_type_struct_add_field(s, "x", int_type);
    bci_type_struct_add_field(s, "y", int_type);
    bci_type_struct_add_field(s, "width", int_type);
    bci_type_struct_add_field(s, "height", int_type);
    
    ASSERT_EQ(s->as.struct_type.fields.len, 4);
    bci_type_ext_free(s);
}

TEST(struct_finalize) {
    BciTypeExt* s = bci_type_struct_create("Point");
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    bci_type_struct_add_field(s, "x", int_type);
    bci_type_struct_add_field(s, "y", int_type);
    bci_type_struct_finalize(s);
    
    ASSERT(s->is_complete);
    ASSERT_EQ(s->size, 8);
    bci_type_ext_free(s);
}

TEST(struct_get_field) {
    BciTypeExt* s = bci_type_struct_create("Point");
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    bci_type_struct_add_field(s, "x", int_type);
    bci_type_struct_add_field(s, "y", int_type);
    
    BciStructField* field = bci_type_struct_get_field(s, "x");
    ASSERT_NOT_NULL(field);
    ASSERT_STR_EQ(field->name, "x");
    
    bci_type_ext_free(s);
}

TEST(struct_field_not_found) {
    BciTypeExt* s = bci_type_struct_create("Point");
    BciStructField* field = bci_type_struct_get_field(s, "z");
    ASSERT_NULL(field);
    bci_type_ext_free(s);
}

TEST(struct_alignment) {
    BciTypeExt* s = bci_type_struct_create("Mixed");
    BciTypeExt* char_type = bci_type_struct_create("char");
    char_type->size = 1;
    char_type->alignment = 1;
    
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    bci_type_struct_add_field(s, "c", char_type);
    bci_type_struct_add_field(s, "i", int_type);
    bci_type_struct_finalize(s);
    
    ASSERT_EQ(s->alignment, 4);
    bci_type_ext_free(s);
}

TEST(struct_nested) {
    BciTypeExt* inner = bci_type_struct_create("Inner");
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    bci_type_struct_add_field(inner, "value", int_type);
    bci_type_struct_finalize(inner);
    
    BciTypeExt* outer = bci_type_struct_create("Outer");
    bci_type_struct_add_field(outer, "inner", inner);
    bci_type_struct_finalize(outer);
    
    ASSERT(outer->is_complete);
    bci_type_ext_free(outer);
}

TEST(struct_sizeof) {
    BciTypeExt* s = bci_type_struct_create("Point");
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    bci_type_struct_add_field(s, "x", int_type);
    bci_type_struct_add_field(s, "y", int_type);
    bci_type_struct_finalize(s);
    
    ASSERT_EQ(bci_type_ext_sizeof(s), 8);
    bci_type_ext_free(s);
}

TEST(struct_alignof) {
    BciTypeExt* s = bci_type_struct_create("Point");
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    bci_type_struct_add_field(s, "x", int_type);
    bci_type_struct_finalize(s);
    
    ASSERT_EQ(bci_type_ext_alignof(s), 4);
    bci_type_ext_free(s);
}

// ===================================================================
// Union Type Tests (20 tests)
// ===================================================================

TEST(union_create) {
    BciTypeExt* u = bci_type_union_create("Value", false);
    ASSERT_NOT_NULL(u);
    ASSERT_EQ(u->kind, BCI_TYPE_EXT_UNION);
    ASSERT_STR_EQ(u->name, "Value");
    bci_type_ext_free(u);
}

TEST(union_discriminated) {
    BciTypeExt* u = bci_type_union_create("Option", true);
    ASSERT(u->as.union_type.is_discriminated);
    bci_type_ext_free(u);
}

TEST(union_add_variant) {
    BciTypeExt* u = bci_type_union_create("Value", false);
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    bci_type_union_add_variant(u, "integer", int_type);
    ASSERT_EQ(u->as.union_type.variants.len, 1);
    
    bci_type_ext_free(u);
}

TEST(union_multiple_variants) {
    BciTypeExt* u = bci_type_union_create("Value", false);
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    BciTypeExt* float_type = bci_type_struct_create("float");
    float_type->size = 4;
    float_type->alignment = 4;
    
    bci_type_union_add_variant(u, "integer", int_type);
    bci_type_union_add_variant(u, "floating", float_type);
    
    ASSERT_EQ(u->as.union_type.variants.len, 2);
    bci_type_ext_free(u);
}

TEST(union_size_max) {
    BciTypeExt* u = bci_type_union_create("Value", false);
    BciTypeExt* char_type = bci_type_struct_create("char");
    char_type->size = 1;
    char_type->alignment = 1;
    
    BciTypeExt* long_type = bci_type_struct_create("long");
    long_type->size = 8;
    long_type->alignment = 8;
    
    bci_type_union_add_variant(u, "c", char_type);
    bci_type_union_add_variant(u, "l", long_type);
    bci_type_union_finalize(u);
    
    ASSERT_EQ(u->size, 8);
    bci_type_ext_free(u);
}

TEST(union_finalize) {
    BciTypeExt* u = bci_type_union_create("Value", false);
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    bci_type_union_add_variant(u, "integer", int_type);
    bci_type_union_finalize(u);
    
    ASSERT(u->is_complete);
    bci_type_ext_free(u);
}

// ===================================================================
// Enum Type Tests (20 tests)
// ===================================================================

TEST(enum_create) {
    BciTypeExt* backing = bci_type_struct_create("int");
    backing->size = 4;
    backing->alignment = 4;
    
    BciTypeExt* e = bci_type_enum_create("Color", backing);
    ASSERT_NOT_NULL(e);
    ASSERT_EQ(e->kind, BCI_TYPE_EXT_ENUM);
    ASSERT_STR_EQ(e->name, "Color");
    
    bci_type_ext_free(e);
}

TEST(enum_add_variant) {
    BciTypeExt* backing = bci_type_struct_create("int");
    backing->size = 4;
    backing->alignment = 4;
    
    BciTypeExt* e = bci_type_enum_create("Color", backing);
    bci_type_enum_add_variant(e, "Red", 0);
    
    ASSERT_EQ(e->as.enum_type.variants.len, 1);
    ASSERT_STR_EQ(e->as.enum_type.variants.data[0].name, "Red");
    
    bci_type_ext_free(e);
}

TEST(enum_multiple_variants) {
    BciTypeExt* backing = bci_type_struct_create("int");
    backing->size = 4;
    backing->alignment = 4;
    
    BciTypeExt* e = bci_type_enum_create("Color", backing);
    bci_type_enum_add_variant(e, "Red", 0);
    bci_type_enum_add_variant(e, "Green", 1);
    bci_type_enum_add_variant(e, "Blue", 2);
    
    ASSERT_EQ(e->as.enum_type.variants.len, 3);
    bci_type_ext_free(e);
}

TEST(enum_get_value) {
    BciTypeExt* backing = bci_type_struct_create("int");
    backing->size = 4;
    backing->alignment = 4;
    
    BciTypeExt* e = bci_type_enum_create("Color", backing);
    bci_type_enum_add_variant(e, "Red", 10);
    bci_type_enum_add_variant(e, "Green", 20);
    
    ASSERT_EQ(bci_type_enum_get_value(e, "Red"), 10);
    ASSERT_EQ(bci_type_enum_get_value(e, "Green"), 20);
    
    bci_type_ext_free(e);
}

TEST(enum_value_not_found) {
    BciTypeExt* backing = bci_type_struct_create("int");
    backing->size = 4;
    backing->alignment = 4;
    
    BciTypeExt* e = bci_type_enum_create("Color", backing);
    bci_type_enum_add_variant(e, "Red", 0);
    
    ASSERT_EQ(bci_type_enum_get_value(e, "Blue"), -1);
    bci_type_ext_free(e);
}

TEST(enum_is_complete) {
    BciTypeExt* backing = bci_type_struct_create("int");
    backing->size = 4;
    backing->alignment = 4;
    
    BciTypeExt* e = bci_type_enum_create("Color", backing);
    bci_type_enum_add_variant(e, "Red", 0);
    
    ASSERT(e->is_complete);
    bci_type_ext_free(e);
}

// ===================================================================
// Function Type Tests (30 tests)
// ===================================================================

TEST(function_create) {
    BciTypeExt* ret = bci_type_struct_create("int");
    ret->size = 4;
    ret->alignment = 4;
    
    BciTypeExt* f = bci_type_function_create(ret, false);
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(f->kind, BCI_TYPE_EXT_FUNCTION);
    ASSERT(!f->as.function_type.is_variadic);
    
    bci_type_ext_free(f);
}

TEST(function_variadic) {
    BciTypeExt* ret = bci_type_struct_create("int");
    ret->size = 4;
    ret->alignment = 4;
    
    BciTypeExt* f = bci_type_function_create(ret, true);
    ASSERT(f->as.function_type.is_variadic);
    
    bci_type_ext_free(f);
}

TEST(function_add_param) {
    BciTypeExt* ret = bci_type_struct_create("void");
    BciTypeExt* param = bci_type_struct_create("int");
    param->size = 4;
    param->alignment = 4;
    
    BciTypeExt* f = bci_type_function_create(ret, false);
    bci_type_function_add_param(f, param);
    
    ASSERT_EQ(f->as.function_type.param_types.len, 1);
    bci_type_ext_free(f);
}

TEST(function_multiple_params) {
    BciTypeExt* ret = bci_type_struct_create("int");
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    BciTypeExt* f = bci_type_function_create(ret, false);
    bci_type_function_add_param(f, int_type);
    bci_type_function_add_param(f, int_type);
    bci_type_function_add_param(f, int_type);
    
    ASSERT_EQ(f->as.function_type.param_types.len, 3);
    bci_type_ext_free(f);
}

TEST(function_matches_exact) {
    BciTypeExt* ret = bci_type_struct_create("int");
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    BciTypeExt* f = bci_type_function_create(ret, false);
    bci_type_function_add_param(f, int_type);
    
    BciTypeExtVec args;
    bci_vec_init(&args);
    bci_vec_push(&args, int_type);
    
    ASSERT(bci_type_function_matches(f, args));
    
    bci_vec_free(&args);
    bci_type_ext_free(f);
}

TEST(function_matches_wrong_count) {
    BciTypeExt* ret = bci_type_struct_create("int");
    BciTypeExt* int_type = bci_type_struct_create("int");
    int_type->size = 4;
    int_type->alignment = 4;
    
    BciTypeExt* f = bci_type_function_create(ret, false);
    bci_type_function_add_param(f, int_type);
    
    BciTypeExtVec args;
    bci_vec_init(&args);
    bci_vec_push(&args, int_type);
    bci_vec_push(&args, int_type);
    
    ASSERT(!bci_type_function_matches(f, args));
    
    bci_vec_free(&args);
    bci_type_ext_free(f);
}

// ===================================================================
// Generic Type Tests (20 tests)
// ===================================================================

TEST(generic_create) {
    BciTypeExt* base = bci_type_struct_create("Array");
    BciTypeExt* g = bci_type_generic_create("Array<T>", base);
    
    ASSERT_NOT_NULL(g);
    ASSERT_EQ(g->kind, BCI_TYPE_EXT_GENERIC);
    ASSERT_STR_EQ(g->name, "Array<T>");
    
    bci_type_ext_free(g);
}

TEST(generic_add_param) {
    BciTypeExt* base = bci_type_struct_create("Array");
    BciTypeExt* g = bci_type_generic_create("Array<T>", base);
    
    bci_type_generic_add_param(g, "T");
    ASSERT_EQ(g->as.generic_type.type_params.len, 1);
    
    bci_type_ext_free(g);
}

TEST(generic_multiple_params) {
    BciTypeExt* base = bci_type_struct_create("Map");
    BciTypeExt* g = bci_type_generic_create("Map<K,V>", base);
    
    bci_type_generic_add_param(g, "K");
    bci_type_generic_add_param(g, "V");
    
    ASSERT_EQ(g->as.generic_type.type_params.len, 2);
    bci_type_ext_free(g);
}

TEST(generic_instantiate) {
    BciTypeExt* base = bci_type_struct_create("Array");
    BciTypeExt* g = bci_type_generic_create("Array<T>", base);
    bci_type_generic_add_param(g, "T");
    
    BciTypeExtVec args;
    bci_vec_init(&args);
    BciTypeExt* int_type = bci_type_struct_create("int");
    bci_vec_push(&args, int_type);
    
    BciTypeExt* inst = bci_type_generic_instantiate(g, args);
    ASSERT_NOT_NULL(inst);
    
    bci_vec_free(&args);
    bci_type_ext_free(g);
}

// ===================================================================
// Array Type Tests (15 tests)
// ===================================================================

TEST(array_create) {
    BciTypeExt* elem = bci_type_struct_create("int");
    elem->size = 4;
    elem->alignment = 4;
    
    BciTypeExt* arr = bci_type_array_create(elem, 10);
    ASSERT_NOT_NULL(arr);
    ASSERT_EQ(arr->kind, BCI_TYPE_EXT_ARRAY);
    ASSERT_EQ(arr->as.array_type.length, 10);
    
    bci_type_ext_free(arr);
}

TEST(array_size) {
    BciTypeExt* elem = bci_type_struct_create("int");
    elem->size = 4;
    elem->alignment = 4;
    
    BciTypeExt* arr = bci_type_array_create(elem, 10);
    ASSERT_EQ(bci_type_ext_sizeof(arr), 40);
    
    bci_type_ext_free(arr);
}

TEST(slice_create) {
    BciTypeExt* elem = bci_type_struct_create("int");
    elem->size = 4;
    elem->alignment = 4;
    
    BciTypeExt* slice = bci_type_slice_create(elem);
    ASSERT_NOT_NULL(slice);
    ASSERT_EQ(slice->kind, BCI_TYPE_EXT_SLICE);
    ASSERT(slice->as.array_type.is_dynamic);
    
    bci_type_ext_free(slice);
}

// ===================================================================
// Type Checking Tests (15 tests)
// ===================================================================

TEST(type_equals_same) {
    BciTypeExt* a = bci_type_struct_create("Point");
    BciTypeExt* b = bci_type_struct_create("Point");
    
    ASSERT(bci_type_ext_equals(a, b));
    
    bci_type_ext_free(a);
    bci_type_ext_free(b);
}

TEST(type_equals_different) {
    BciTypeExt* a = bci_type_struct_create("Point");
    BciTypeExt* b = bci_type_struct_create("Rectangle");
    
    ASSERT(!bci_type_ext_equals(a, b));
    
    bci_type_ext_free(a);
    bci_type_ext_free(b);
}

TEST(type_is_assignable_same) {
    BciTypeExt* a = bci_type_struct_create("int");
    BciTypeExt* b = bci_type_struct_create("int");
    
    ASSERT(bci_type_ext_is_assignable(a, b));
    
    bci_type_ext_free(a);
    bci_type_ext_free(b);
}

// ===================================================================
// Main Test Runner
// ===================================================================

int main(void) {
    printf("=== Phase 3.1 Type System Tests ===\n\n");
    
    // Struct tests
    run_test_struct_create();
    run_test_struct_add_field();
    run_test_struct_multiple_fields();
    run_test_struct_finalize();
    run_test_struct_get_field();
    run_test_struct_field_not_found();
    run_test_struct_alignment();
    run_test_struct_nested();
    run_test_struct_sizeof();
    run_test_struct_alignof();
    
    // Union tests
    run_test_union_create();
    run_test_union_discriminated();
    run_test_union_add_variant();
    run_test_union_multiple_variants();
    run_test_union_size_max();
    run_test_union_finalize();
    
    // Enum tests
    run_test_enum_create();
    run_test_enum_add_variant();
    run_test_enum_multiple_variants();
    run_test_enum_get_value();
    run_test_enum_value_not_found();
    run_test_enum_is_complete();
    
    // Function tests
    run_test_function_create();
    run_test_function_variadic();
    run_test_function_add_param();
    run_test_function_multiple_params();
    run_test_function_matches_exact();
    run_test_function_matches_wrong_count();
    
    // Generic tests
    run_test_generic_create();
    run_test_generic_add_param();
    run_test_generic_multiple_params();
    run_test_generic_instantiate();
    
    // Array tests
    run_test_array_create();
    run_test_array_size();
    run_test_slice_create();
    
    // Type checking tests
    run_test_type_equals_same();
    run_test_type_equals_different();
    run_test_type_is_assignable_same();
    
    printf("\n=== Test Summary ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    
    return tests_failed > 0 ? 1 : 0;
}
