// ===================================================================
// C23 Compatibility Header - Phase 2 Enhanced
// Provides C23 feature compatibility for older compilers
// Phase 2: Added _Thread_local, typeof, _Atomic support
// ===================================================================
#ifndef C23_COMPAT_H
#define C23_COMPAT_H

#include <stddef.h>
#include <stdint.h>

// Check C standard version
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define C23_AVAILABLE 1
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202000L
    #define C23_DRAFT_AVAILABLE 1  // C2x draft
#endif

// nullptr compatibility
#ifdef __cplusplus
    // C++ has nullptr built-in
#elif defined(C23_AVAILABLE) || defined(C23_DRAFT_AVAILABLE)
    // C23/C2x has nullptr
#else
    // Fallback for older compilers - always define it
    #ifndef nullptr
        #define nullptr ((void*)0)
    #endif
#endif

// NODISCARD compatibility (Phase 1)
#ifndef __has_c_attribute
    #define __has_c_attribute(x) 0
#endif

#if __has_c_attribute(nodiscard)
    #define NODISCARD [[nodiscard]]
#elif defined(__GNUC__) || defined(__clang__)
    #define NODISCARD __attribute__((warn_unused_result))
#else
    #define NODISCARD
#endif

// _Static_assert compatibility
#ifndef _Static_assert
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        // C11 has _Static_assert
    #else
        #define _Static_assert(expr, msg) typedef char static_assert_##__LINE__[(expr)?1:-1]
    #endif
#endif

// ===================================================================
// PHASE 2: New C23 Features
// ===================================================================

// _Thread_local compatibility (Phase 2)
#ifndef _Thread_local
    #if defined(C23_AVAILABLE) || defined(C23_DRAFT_AVAILABLE)
        // C23/C2x has _Thread_local
    #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        // C11 has _Thread_local
    #elif defined(__GNUC__) || defined(__clang__)
        #define _Thread_local __thread
    #elif defined(_MSC_VER)
        #define _Thread_local __declspec(thread)
    #else
        #define _Thread_local
    #endif
#endif

// typeof compatibility (Phase 2)
#ifndef typeof
    #if defined(C23_AVAILABLE)
        // C23 has typeof
    #elif defined(__GNUC__) || defined(__clang__)
        #define typeof __typeof__
    #else
        // Fallback: no typeof support
        #define typeof(x) void*
    #endif
#endif

// _Alignof compatibility (Phase 2)
#ifndef _Alignof
    #if defined(C23_AVAILABLE) || defined(C23_DRAFT_AVAILABLE)
        // C23/C2x has _Alignof
    #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        // C11 has _Alignof
    #elif defined(__GNUC__) || defined(__clang__)
        #define _Alignof __alignof__
    #else
        #define _Alignof(x) sizeof(x)
    #endif
#endif

// _Atomic compatibility (Phase 2)
#ifndef _Atomic
    #if defined(C23_AVAILABLE) || defined(C23_DRAFT_AVAILABLE)
        // C23/C2x has _Atomic
    #elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
        #include <stdatomic.h>
        // C11 has _Atomic
    #elif defined(__GNUC__) || defined(__clang__)
        #define _Atomic(T) T volatile
    #else
        #define _Atomic(T) T
    #endif
#endif

// Atomic operations compatibility
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    #include <stdatomic.h>
    #define ATOMIC_LOAD(ptr) atomic_load(ptr)
    #define ATOMIC_STORE(ptr, val) atomic_store(ptr, val)
    #define ATOMIC_FETCH_ADD(ptr, val) atomic_fetch_add(ptr, val)
    #define ATOMIC_FETCH_SUB(ptr, val) atomic_fetch_sub(ptr, val)
#elif defined(__GNUC__) || defined(__clang__)
    #define ATOMIC_LOAD(ptr) __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
    #define ATOMIC_STORE(ptr, val) __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
    #define ATOMIC_FETCH_ADD(ptr, val) __atomic_fetch_add(ptr, val, __ATOMIC_SEQ_CST)
    #define ATOMIC_FETCH_SUB(ptr, val) __atomic_fetch_sub(ptr, val, __ATOMIC_SEQ_CST)
#else
    #define ATOMIC_LOAD(ptr) (*(ptr))
    #define ATOMIC_STORE(ptr, val) (*(ptr) = (val))
    #define ATOMIC_FETCH_ADD(ptr, val) ((*(ptr))++)
    #define ATOMIC_FETCH_SUB(ptr, val) ((*(ptr))--)
#endif

#endif // C23_COMPAT_H
