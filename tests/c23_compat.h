
// C23 Compatibility Header for Tests
// Provides C23 features for compilers with partial support

#ifndef BDI_C23_COMPAT_H
#define BDI_C23_COMPAT_H

#include <assert.h>

// nullptr support
#ifndef nullptr
#define nullptr ((void*)0)
#endif

// static_assert support
#ifndef static_assert
#define static_assert _Static_assert
#endif

// Attribute support for older compilers
#ifndef __has_c_attribute
#define __has_c_attribute(x) 0
#endif

// nodiscard attribute
#if __has_c_attribute(nodiscard)
// Use native attribute
#elif defined(__GNUC__) || defined(__clang__)
#define nodiscard warn_unused_result
#endif

// maybe_unused attribute
#if __has_c_attribute(maybe_unused)
// Use native attribute
#elif defined(__GNUC__) || defined(__clang__)
#define maybe_unused unused
#endif

// noreturn attribute  
#if __has_c_attribute(noreturn)
// Use native attribute
#elif defined(__GNUC__) || defined(__clang__)
#define noreturn noreturn
#endif

// fallthrough attribute
#if __has_c_attribute(fallthrough)
// Use native attribute
#elif defined(__GNUC__) && __GNUC__ >= 7
#define fallthrough fallthrough
#else
#define fallthrough
#endif

// typeof support
#ifndef typeof
#if defined(__GNUC__) || defined(__clang__)
#define typeof __typeof__
#endif
#endif

// auto keyword
#ifndef auto
#if defined(__GNUC__) || defined(__clang__)
#define auto __auto_type
#endif
#endif

#endif // BDI_C23_COMPAT_H

