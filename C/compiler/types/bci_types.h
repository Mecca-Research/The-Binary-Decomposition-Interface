// ===================================================================
// DESC: Core data types, enums, and dynamic data structures for the
//       BCI (Binary C Interface) compiler. Pure C (C99/C11).
// ===================================================================
#ifndef BCI_TYPES_H
#define BCI_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// --- Core Type Enumeration ---
// Defines all primitive types recognized by the compiler.
typedef enum {
    BCI_TYPE_UNKNOWN,
    BCI_TYPE_VOID,
    BCI_TYPE_BOOL,
    BCI_TYPE_I8, BCI_TYPE_U8,
    BCI_TYPE_I16, BCI_TYPE_U16,
    BCI_TYPE_I32, BCI_TYPE_U32,
    BCI_TYPE_I64, BCI_TYPE_U64,
    BCI_TYPE_F32, BCI_TYPE_F64,
    BCI_TYPE_PTR // Generic pointer type
} BciTypeKind;

// Represents a type in the compiler, can be extended for complex types.
typedef struct BciType {
    BciTypeKind kind;
    // For pointer types, points to the base type.
    // For structs/arrays, this could point to more detailed info.
    struct BciType* base;
} BciType;

// --- Dynamic String (similar to an sds string) ---
typedef struct {
    char* data;
    size_t len;
    size_t capacity;
} BciStr;

BciStr* bci_str_new(const char* init);
void bci_str_free(BciStr* str);
void bci_str_append(BciStr* str, const char* suffix);

// --- Generic Dynamic Array (Vector) ---
// A type-safe vector implementation using macros.
#define BciVec(T) struct { T* data; size_t len; size_t capacity; }

#define bci_vec_init(v)     do { (v)->data = NULL; (v)->len = 0; (v)->capacity = 0; } while (0)
#define bci_vec_free(v)     free((v)->data)
#define bci_vec_push(v, val) do { \
    if ((v)->len >= (v)->capacity) { \
        (v)->capacity = ((v)->capacity == 0) ? 8 : (v)->capacity * 2; \
        (v)->data = realloc((v)->data, (v)->capacity * sizeof(*(v)->data)); \
    } \
    (v)->data[(v)->len++] = (val); \
} while (0)

#endif // BCI_TYPES_H
