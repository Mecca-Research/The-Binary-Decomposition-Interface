
// C23 Compatibility Header
// Provides C23 features for compilers with partial support

#include "c23_compat.h"
#ifndef BDI_C23_COMPAT_H
#define BDI_C23_COMPAT_H

// nullptr support
#ifndef nullptr
#define nullptr ((void*)0)
#endif

// Attribute support for older compilers
#ifndef __has_c_attribute
#define __has_c_attribute(x) 0
#endif

// nodiscard attribute
#if __has_c_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#elif defined(__GNUC__) || defined(__clang__)
#define NODISCARD __attribute__((warn_unused_result))
#else
#define NODISCARD
#endif

// maybe_unused attribute
#if __has_c_attribute(maybe_unused)
#define MAYBE_UNUSED [[maybe_unused]]
#elif defined(__GNUC__) || defined(__clang__)
#define MAYBE_UNUSED __attribute__((unused))
#else
#define MAYBE_UNUSED
#endif

// noreturn attribute
#if __has_c_attribute(noreturn)
#define NORETURN [[noreturn]]
#elif defined(__GNUC__) || defined(__clang__)
#define NORETURN __attribute__((noreturn))
#else
#define NORETURN
#endif

// fallthrough attribute
#if __has_c_attribute(fallthrough)
#define FALLTHROUGH [[fallthrough]]
#elif defined(__GNUC__) && __GNUC__ >= 7
#define FALLTHROUGH __attribute__((fallthrough))
#else
#define FALLTHROUGH
#endif

// typeof support
#ifndef typeof
#if defined(__GNUC__) || defined(__clang__)
#define typeof __typeof__
#else
#define typeof(x) void*
#endif
#endif

// auto keyword (limited support)
#ifndef auto
#if defined(__GNUC__) || defined(__clang__)
#define auto __auto_type
#else
#define auto
#endif
#endif

#endif // BDI_C23_COMPAT_H

