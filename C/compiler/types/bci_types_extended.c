
// ===================================================================
// DESC: Implementation of Extended Type System for Phase 3
// ===================================================================

#include "c23_compat.h"
#include "bci_types_extended.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

// --- Helper Functions ---

static size_t align_to(size_t offset, size_t alignment) {
    return (offset + alignment - 1) & ~(alignment - 1);
}

static size_t max_size(size_t a, size_t b) {
    return a > b ? a : b;
}

// --- Struct Type Implementation ---

BciTypeExt* bci_type_struct_create(const char* name) {
    BciTypeExt* type = malloc(sizeof(BciTypeExt));
    if (!type) return nullptr;
    
    type->kind = BCI_TYPE_EXT_STRUCT;
    type->name = name ? strdup(name) : nullptr;
    type->size = 0;
    type->alignment = 1;
    type->is_complete = false;
    
    bci_vec_init(&type->as.struct_type.fields);
    type->as.struct_type.name = type->name;
    type->as.struct_type.size = 0;
    type->as.struct_type.alignment = 1;
    type->as.struct_type.is_packed = false;
    
    return type;
}

void bci_type_struct_add_field(BciTypeExt* struct_type, const char* field_name, 
                                BciTypeExt* field_type) {
    assert(struct_type && struct_type->kind == BCI_TYPE_EXT_STRUCT);
    assert(field_name && field_type);
    
    BciStructField field;
    field.name = strdup(field_name);
    field.type = field_type;
    field.alignment = bci_type_ext_alignof(field_type);
    
    // Calculate offset with alignment
    size_t current_size = struct_type->as.struct_type.size;
    field.offset = align_to(current_size, field.alignment);
    
    bci_vec_push(&struct_type->as.struct_type.fields, field);
    
    // Update struct size and alignment
    struct_type->as.struct_type.size = field.offset + bci_type_ext_sizeof(field_type);
    struct_type->as.struct_type.alignment = max_size(
        struct_type->as.struct_type.alignment, field.alignment);
}

void bci_type_struct_finalize(BciTypeExt* struct_type) {
    assert(struct_type && struct_type->kind == BCI_TYPE_EXT_STRUCT);
    
    // Align final size to struct alignment
    struct_type->as.struct_type.size = align_to(
        struct_type->as.struct_type.size, 
        struct_type->as.struct_type.alignment);
    
    struct_type->size = struct_type->as.struct_type.size;
    struct_type->alignment = struct_type->as.struct_type.alignment;
    struct_type->is_complete = true;
}

BciStructField* bci_type_struct_get_field(BciTypeExt* struct_type, const char* field_name) {
    assert(struct_type && struct_type->kind == BCI_TYPE_EXT_STRUCT);
    assert(field_name);
    
    for (size_t i = 0; i < struct_type->as.struct_type.fields.len; i++) {
        if (strcmp(struct_type->as.struct_type.fields.data[i].name, field_name) == 0) {
            return &struct_type->as.struct_type.fields.data[i];
        }
    }
    return nullptr;
}

// --- Union Type Implementation ---

BciTypeExt* bci_type_union_create(const char* name, bool is_discriminated) {
    BciTypeExt* type = malloc(sizeof(BciTypeExt));
    if (!type) return nullptr;
    
    type->kind = BCI_TYPE_EXT_UNION;
    type->name = name ? strdup(name) : nullptr;
    type->size = 0;
    type->alignment = 1;
    type->is_complete = false;
    
    bci_vec_init(&type->as.union_type.variants);
    type->as.union_type.name = type->name;
    type->as.union_type.size = 0;
    type->as.union_type.is_discriminated = is_discriminated;
    type->as.union_type.discriminant_type = nullptr;
    
    return type;
}

void bci_type_union_add_variant(BciTypeExt* union_type, const char* variant_name, 
                                 BciTypeExt* variant_type) {
    assert(union_type && union_type->kind == BCI_TYPE_EXT_UNION);
    assert(variant_name && variant_type);
    
    BciUnionVariant variant;
    variant.name = strdup(variant_name);
    variant.type = variant_type;
    
    bci_vec_push(&union_type->as.union_type.variants, variant);
    
    // Union size is the maximum of all variant sizes
    size_t variant_size = bci_type_ext_sizeof(variant_type);
    size_t variant_align = bci_type_ext_alignof(variant_type);
    
    if (variant_size > union_type->as.union_type.size) {
        union_type->as.union_type.size = variant_size;
    }
    
    union_type->alignment = max_size(union_type->alignment, variant_align);
}

void bci_type_union_finalize(BciTypeExt* union_type) {
    assert(union_type && union_type->kind == BCI_TYPE_EXT_UNION);
    
    // If discriminated, add space for discriminant
    if (union_type->as.union_type.is_discriminated) {
        union_type->as.union_type.size += sizeof(int32_t);
    }
    
    union_type->size = union_type->as.union_type.size;
    union_type->is_complete = true;
}

// --- Enum Type Implementation ---

BciTypeExt* bci_type_enum_create(const char* name, BciTypeExt* backing_type) {
    BciTypeExt* type = malloc(sizeof(BciTypeExt));
    if (!type) return nullptr;
    
    type->kind = BCI_TYPE_EXT_ENUM;
    type->name = name ? strdup(name) : nullptr;
    type->is_complete = false;
    
    bci_vec_init(&type->as.enum_type.variants);
    type->as.enum_type.name = type->name;
    type->as.enum_type.backing_type = backing_type;
    
    // Size is determined by backing type
    type->size = backing_type ? bci_type_ext_sizeof(backing_type) : sizeof(int32_t);
    type->alignment = backing_type ? bci_type_ext_alignof(backing_type) : sizeof(int32_t);
    
    return type;
}

void bci_type_enum_add_variant(BciTypeExt* enum_type, const char* variant_name, 
                                int64_t value) {
    assert(enum_type && enum_type->kind == BCI_TYPE_EXT_ENUM);
    assert(variant_name);
    
    BciEnumVariant variant;
    variant.name = strdup(variant_name);
    variant.value = value;
    
    bci_vec_push(&enum_type->as.enum_type.variants, variant);
    enum_type->is_complete = true;
}

int64_t bci_type_enum_get_value(BciTypeExt* enum_type, const char* variant_name) {
    assert(enum_type && enum_type->kind == BCI_TYPE_EXT_ENUM);
    assert(variant_name);
    
    for (size_t i = 0; i < enum_type->as.enum_type.variants.len; i++) {
        if (strcmp(enum_type->as.enum_type.variants.data[i].name, variant_name) == 0) {
            return enum_type->as.enum_type.variants.data[i].value;
        }
    }
    return -1;
}

// --- Function Type Implementation ---

BciTypeExt* bci_type_function_create(BciTypeExt* return_type, bool is_variadic) {
    BciTypeExt* type = malloc(sizeof(BciTypeExt));
    if (!type) return nullptr;
    
    type->kind = BCI_TYPE_EXT_FUNCTION;
    type->name = nullptr;
    type->size = sizeof(void*); // Function pointer size
    type->alignment = sizeof(void*);
    type->is_complete = true;
    
    type->as.function_type.return_type = return_type;
    bci_vec_init(&type->as.function_type.param_types);
    type->as.function_type.is_variadic = is_variadic;
    type->as.function_type.is_pure = false;
    type->as.function_type.is_noreturn = false;
    
    return type;
}

void bci_type_function_add_param(BciTypeExt* func_type, BciTypeExt* param_type) {
    assert(func_type && func_type->kind == BCI_TYPE_EXT_FUNCTION);
    assert(param_type);
    
    bci_vec_push(&func_type->as.function_type.param_types, param_type);
}

bool bci_type_function_matches(BciTypeExt* func_type, BciTypeExtVec arg_types) {
    assert(func_type && func_type->kind == BCI_TYPE_EXT_FUNCTION);
    
    size_t param_count = func_type->as.function_type.param_types.len;
    
    // Check parameter count
    if (!func_type->as.function_type.is_variadic && arg_types.len != param_count) {
        return false;
    }
    
    if (func_type->as.function_type.is_variadic && arg_types.len < param_count) {
        return false;
    }
    
    // Check parameter types
    for (size_t i = 0; i < param_count; i++) {
        if (!bci_type_ext_is_assignable(
                func_type->as.function_type.param_types.data[i],
                arg_types.data[i])) {
            return false;
        }
    }
    
    return true;
}

// --- Generic Type Implementation ---

BciTypeExt* bci_type_generic_create(const char* name, BciTypeExt* base_type) {
    BciTypeExt* type = malloc(sizeof(BciTypeExt));
    if (!type) return nullptr;
    
    type->kind = BCI_TYPE_EXT_GENERIC;
    type->name = name ? strdup(name) : nullptr;
    type->size = 0;
    type->alignment = 0;
    type->is_complete = false;
    
    bci_vec_init(&type->as.generic_type.type_params);
    bci_vec_init(&type->as.generic_type.instantiations);
    type->as.generic_type.name = type->name;
    type->as.generic_type.base_type = base_type;
    
    return type;
}

void bci_type_generic_add_param(BciTypeExt* generic_type, const char* param_name) {
    assert(generic_type && generic_type->kind == BCI_TYPE_EXT_GENERIC);
    assert(param_name);
    
    BciGenericParam param;
    param.name = strdup(param_name);
    bci_vec_init(&param.constraints);
    
    bci_vec_push(&generic_type->as.generic_type.type_params, param);
}

BciTypeExt* bci_type_generic_instantiate(BciTypeExt* generic_type, 
                                          BciTypeExtVec type_args) {
    assert(generic_type && generic_type->kind == BCI_TYPE_EXT_GENERIC);
    
    // Check if already instantiated with these args
    for (size_t i = 0; i < generic_type->as.generic_type.instantiations.len; i++) {
        BciTypeExt* inst = generic_type->as.generic_type.instantiations.data[i];
        // Simple check - could be more sophisticated
        if (inst) {
            return inst;
        }
    }
    
    // Create new instantiation (simplified)
    BciTypeExt* inst = malloc(sizeof(BciTypeExt));
    if (!inst) return nullptr;
    
    memcpy(inst, generic_type->as.generic_type.base_type, sizeof(BciTypeExt));
    bci_vec_push(&generic_type->as.generic_type.instantiations, inst);
    
    return inst;
}

// --- Array Type Implementation ---

BciTypeExt* bci_type_array_create(BciTypeExt* element_type, size_t length) {
    BciTypeExt* type = malloc(sizeof(BciTypeExt));
    if (!type) return nullptr;
    
    type->kind = BCI_TYPE_EXT_ARRAY;
    type->name = nullptr;
    type->is_complete = true;
    
    type->as.array_type.element_type = element_type;
    type->as.array_type.length = length;
    type->as.array_type.is_dynamic = false;
    
    type->size = bci_type_ext_sizeof(element_type) * length;
    type->alignment = bci_type_ext_alignof(element_type);
    
    return type;
}

BciTypeExt* bci_type_slice_create(BciTypeExt* element_type) {
    BciTypeExt* type = malloc(sizeof(BciTypeExt));
    if (!type) return nullptr;
    
    type->kind = BCI_TYPE_EXT_SLICE;
    type->name = nullptr;
    type->is_complete = true;
    
    type->as.array_type.element_type = element_type;
    type->as.array_type.length = 0;
    type->as.array_type.is_dynamic = true;
    
    // Slice is pointer + length
    type->size = sizeof(void*) + sizeof(size_t);
    type->alignment = sizeof(void*);
    
    return type;
}

// --- Type Checking Utilities ---

bool bci_type_ext_equals(BciTypeExt* a, BciTypeExt* b) {
    if (!a || !b) return false;
    if (a == b) return true;
    if (a->kind != b->kind) return false;
    
    switch (a->kind) {
        case BCI_TYPE_EXT_STRUCT:
        case BCI_TYPE_EXT_UNION:
        case BCI_TYPE_EXT_ENUM:
            return a->name && b->name && strcmp(a->name, b->name) == 0;
        
        case BCI_TYPE_EXT_ARRAY:
            return a->as.array_type.length == b->as.array_type.length &&
                   bci_type_ext_equals(a->as.array_type.element_type, 
                                       b->as.array_type.element_type);
        
        case BCI_TYPE_EXT_FUNCTION:
            if (!bci_type_ext_equals(a->as.function_type.return_type,
                                     b->as.function_type.return_type)) {
                return false;
            }
            if (a->as.function_type.param_types.len != 
                b->as.function_type.param_types.len) {
                return false;
            }
            for (size_t i = 0; i < a->as.function_type.param_types.len; i++) {
                if (!bci_type_ext_equals(a->as.function_type.param_types.data[i],
                                         b->as.function_type.param_types.data[i])) {
                    return false;
                }
            }
            return true;
        
        default:
            return false;
    }
}

bool bci_type_ext_is_assignable(BciTypeExt* dest, BciTypeExt* src) {
    if (!dest || !src) return false;
    if (bci_type_ext_equals(dest, src)) return true;
    
    // Allow numeric conversions
    if (bci_type_ext_is_numeric(dest) && bci_type_ext_is_numeric(src)) {
        return true;
    }
    
    return false;
}

bool bci_type_ext_is_numeric(BciTypeExt* type) {
    if (!type) return false;
    return bci_type_ext_is_integral(type) || bci_type_ext_is_floating(type);
}

bool bci_type_ext_is_integral(BciTypeExt* type) {
    if (!type) return false;
    return type->kind == BCI_TYPE_EXT_ENUM;
}

bool bci_type_ext_is_floating(BciTypeExt* type) {
    (void)type;
    return false; // Simplified
}

size_t bci_type_ext_sizeof(BciTypeExt* type) {
    return type ? type->size : 0;
}

size_t bci_type_ext_alignof(BciTypeExt* type) {
    return type ? type->alignment : 1;
}

// --- Type Inference ---

BciTypeExt* bci_type_infer_binary_op(const char* op, BciTypeExt* left, BciTypeExt* right) {
    if (!op || !left || !right) return nullptr;
    
    // Arithmetic operators
    if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || 
        strcmp(op, "*") == 0 || strcmp(op, "/") == 0) {
        return bci_type_common_type(left, right);
    }
    
    // Comparison operators return bool
    if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
        strcmp(op, "<") == 0 || strcmp(op, ">") == 0 ||
        strcmp(op, "<=") == 0 || strcmp(op, ">=") == 0) {
        // Return bool type (simplified)
        return nullptr;
    }
    
    return nullptr;
}

BciTypeExt* bci_type_infer_unary_op(const char* op, BciTypeExt* operand) {
    if (!op || !operand) return nullptr;
    
    // Most unary ops preserve type
    if (strcmp(op, "-") == 0 || strcmp(op, "+") == 0 || strcmp(op, "~") == 0) {
        return operand;
    }
    
    // Logical not returns bool
    if (strcmp(op, "!") == 0) {
        return nullptr; // Bool type
    }
    
    return nullptr;
}

BciTypeExt* bci_type_common_type(BciTypeExt* a, BciTypeExt* b) {
    if (!a || !b) return nullptr;
    if (bci_type_ext_equals(a, b)) return a;
    
    // Simplified: return larger type
    if (a->size >= b->size) return a;
    return b;
}

// --- Memory Management ---

void bci_type_ext_free(BciTypeExt* type) {
    if (!type) return;
    
    switch (type->kind) {
        case BCI_TYPE_EXT_STRUCT:
            for (size_t i = 0; i < type->as.struct_type.fields.len; i++) {
                free((void*)type->as.struct_type.fields.data[i].name);
            }
            bci_vec_free(&type->as.struct_type.fields);
            break;
        
        case BCI_TYPE_EXT_UNION:
            for (size_t i = 0; i < type->as.union_type.variants.len; i++) {
                free((void*)type->as.union_type.variants.data[i].name);
            }
            bci_vec_free(&type->as.union_type.variants);
            break;
        
        case BCI_TYPE_EXT_ENUM:
            for (size_t i = 0; i < type->as.enum_type.variants.len; i++) {
                free((void*)type->as.enum_type.variants.data[i].name);
            }
            bci_vec_free(&type->as.enum_type.variants);
            break;
        
        case BCI_TYPE_EXT_FUNCTION:
            bci_vec_free(&type->as.function_type.param_types);
            break;
        
        case BCI_TYPE_EXT_GENERIC:
            for (size_t i = 0; i < type->as.generic_type.type_params.len; i++) {
                free((void*)type->as.generic_type.type_params.data[i].name);
                bci_vec_free(&type->as.generic_type.type_params.data[i].constraints);
            }
            bci_vec_free(&type->as.generic_type.type_params);
            bci_vec_free(&type->as.generic_type.instantiations);
            break;
        
        default:
            break;
    }
    
    free((void*)type->name);
    free(type);
}
