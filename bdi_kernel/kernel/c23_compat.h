// ===================================================================
// C23 Compatibility Header
// Provides C23 feature compatibility for older compilers
// ===================================================================
#ifndef C23_COMPAT_H
#define C23_COMPAT_H

// Check C standard version
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
    #define C23_AVAILABLE 1
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202000L
    #define C23_DRAFT_AVAILABLE 1  // C2x draft
#endif

// nullptr compatibility
#ifndef nullptr
    #ifdef __cplusplus
        // C++ has nullptr built-in
    #elif defined(C23_AVAILABLE) || defined(C23_DRAFT_AVAILABLE)
        // C23/C2x has nullptr
    #else
        // Fallback for older compilers
        #define nullptr ((void*)0)
    #endif
#endif

// constexpr compatibility
#ifndef constexpr
    #if defined(C23_AVAILABLE)
        // C23 has constexpr
    #elif defined(__GNUC__) || defined(__clang__)
        // Use const for older compilers
        #define constexpr static const
    #else
        #define constexpr const
    #endif
#endif

// NODISCARD compatibility
#ifndef __has_c_attribute
    #define __has_c_attribute(x) 0
#endif

#if __has_c_attribute(nodiscard)
    #define NODISCARD NODISCARD
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

#endif // C23_COMPAT_H
