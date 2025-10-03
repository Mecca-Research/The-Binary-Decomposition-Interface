
// ===================================================================
// DESC: Extended Type System for Phase 3 - Compiler Infrastructure
//       Adds support for structs, unions, enums, function types, and
//       generic types with full type checking and inference.
// ===================================================================
#ifndef BCI_TYPES_EXTENDED_H
#define BCI_TYPES_EXTENDED_H

#include "c23_compat.h"
#include "../types/bci_types.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// --- Extended Type Kinds ---
typedef enum {
    BCI_TYPE_EXT_STRUCT,
    BCI_TYPE_EXT_UNION,
    BCI_TYPE_EXT_ENUM,
    BCI_TYPE_EXT_FUNCTION,
    BCI_TYPE_EXT_GENERIC,
    BCI_TYPE_EXT_ARRAY,
    BCI_TYPE_EXT_SLICE
} BciTypeExtKind;

// Forward declarations
typedef struct BciStructType BciStructType;
typedef struct BciUnionType BciUnionType;
typedef struct BciEnumType BciEnumType;
typedef struct BciFunctionType BciFunctionType;
typedef struct BciGenericType BciGenericType;
typedef struct BciArrayType BciArrayType;
typedef struct BciTypeExt BciTypeExt;

// Type vector for function parameters
typedef struct {
    BciTypeExt** data;
    size_t len;
    size_t capacity;
} BciTypeExtVec;

// --- Struct Field ---
typedef struct {
    const char* name;
    BciTypeExt* type;
    size_t offset;
    size_t alignment;
} BciStructField;

// --- Struct Type ---
struct BciStructType {
    const char* name;
    BciVec(BciStructField) fields;
    size_t size;
    size_t alignment;
    bool is_packed;
};

// --- Union Variant ---
typedef struct {
    const char* name;
    BciTypeExt* type;
} BciUnionVariant;

// --- Union Type (Discriminated) ---
struct BciUnionType {
    const char* name;
    BciVec(BciUnionVariant) variants;
    size_t size;
    bool is_discriminated;
    BciTypeExt* discriminant_type;
};

// --- Enum Variant ---
typedef struct {
    const char* name;
    int64_t value;
} BciEnumVariant;

// --- Enum Type ---
struct BciEnumType {
    const char* name;
    BciVec(BciEnumVariant) variants;
    BciTypeExt* backing_type; // Integer type backing the enum
};

// --- Function Type ---
struct BciFunctionType {
    BciTypeExt* return_type;
    BciVec(BciTypeExt*) param_types;
    bool is_variadic;
    bool is_pure;
    bool is_noreturn;
};

// --- Generic Type Parameter ---
typedef struct {
    const char* name;
    BciVec(BciTypeExt*) constraints; // Type constraints
} BciGenericParam;

// --- Generic Type ---
struct BciGenericType {
    const char* name;
    BciVec(BciGenericParam) type_params;
    BciTypeExt* base_type;
    BciVec(BciTypeExt*) instantiations; // Cache of instantiated types
};

// --- Array Type ---
struct BciArrayType {
    BciTypeExt* element_type;
    size_t length;
    bool is_dynamic;
};

// --- Extended Type Structure ---
struct BciTypeExt {
    BciTypeExtKind kind;
    union {
        BciStructType struct_type;
        BciUnionType union_type;
        BciEnumType enum_type;
        BciFunctionType function_type;
        BciGenericType generic_type;
        BciArrayType array_type;
    } as;
    
    // Type metadata
    const char* name;
    size_t size;
    size_t alignment;
    bool is_complete;
};

// --- Type System API ---

// Struct type operations
[[nodiscard]] BciTypeExt* bci_type_struct_create(const char* name);
void bci_type_struct_add_field(BciTypeExt* struct_type, const char* field_name, 
                                BciTypeExt* field_type);
void bci_type_struct_finalize(BciTypeExt* struct_type);
[[nodiscard]] BciStructField* bci_type_struct_get_field(BciTypeExt* struct_type, 
                                                          const char* field_name);

// Union type operations
[[nodiscard]] BciTypeExt* bci_type_union_create(const char* name, bool is_discriminated);
void bci_type_union_add_variant(BciTypeExt* union_type, const char* variant_name, 
                                 BciTypeExt* variant_type);
void bci_type_union_finalize(BciTypeExt* union_type);

// Enum type operations
[[nodiscard]] BciTypeExt* bci_type_enum_create(const char* name, BciTypeExt* backing_type);
void bci_type_enum_add_variant(BciTypeExt* enum_type, const char* variant_name, 
                                int64_t value);
[[nodiscard]] int64_t bci_type_enum_get_value(BciTypeExt* enum_type, 
                                               const char* variant_name);

// Function type operations
[[nodiscard]] BciTypeExt* bci_type_function_create(BciTypeExt* return_type, bool is_variadic);
void bci_type_function_add_param(BciTypeExt* func_type, BciTypeExt* param_type);
[[nodiscard]] bool bci_type_function_matches(BciTypeExt* func_type, 
                                              BciTypeExtVec arg_types);

// Generic type operations
[[nodiscard]] BciTypeExt* bci_type_generic_create(const char* name, BciTypeExt* base_type);
void bci_type_generic_add_param(BciTypeExt* generic_type, const char* param_name);
[[nodiscard]] BciTypeExt* bci_type_generic_instantiate(BciTypeExt* generic_type, 
                                                        BciTypeExtVec type_args);

// Array type operations
[[nodiscard]] BciTypeExt* bci_type_array_create(BciTypeExt* element_type, size_t length);
[[nodiscard]] BciTypeExt* bci_type_slice_create(BciTypeExt* element_type);

// Type checking utilities
[[nodiscard]] bool bci_type_ext_equals(BciTypeExt* a, BciTypeExt* b);
[[nodiscard]] bool bci_type_ext_is_assignable(BciTypeExt* dest, BciTypeExt* src);
[[nodiscard]] bool bci_type_ext_is_numeric(BciTypeExt* type);
[[nodiscard]] bool bci_type_ext_is_integral(BciTypeExt* type);
[[nodiscard]] bool bci_type_ext_is_floating(BciTypeExt* type);
[[nodiscard]] size_t bci_type_ext_sizeof(BciTypeExt* type);
[[nodiscard]] size_t bci_type_ext_alignof(BciTypeExt* type);

// Type inference utilities
[[nodiscard]] BciTypeExt* bci_type_infer_binary_op(const char* op, BciTypeExt* left, 
                                                     BciTypeExt* right);
[[nodiscard]] BciTypeExt* bci_type_infer_unary_op(const char* op, BciTypeExt* operand);
[[nodiscard]] BciTypeExt* bci_type_common_type(BciTypeExt* a, BciTypeExt* b);

// Memory management
void bci_type_ext_free(BciTypeExt* type);

// Compile-time invariants
static_assert(sizeof(void*) >= 4, "Extended type system requires at least 32-bit pointers");
static_assert(sizeof(size_t) >= 4, "size_t must be at least 4 bytes");

#endif // BCI_TYPES_EXTENDED_H
