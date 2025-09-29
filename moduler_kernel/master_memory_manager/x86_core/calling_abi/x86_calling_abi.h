
/**
 * @file x86_calling_abi.h
 * @brief x86 Calling Convention and ABI Management Interface
 * 
 * Provides comprehensive x86 calling convention support including:
 * - 32-bit calling conventions (cdecl, stdcall, fastcall)
 * - 64-bit Microsoft x64 calling convention
 * - Stack frame management
 * - Parameter passing mechanisms
 * - Return value handling
 * 
 * Based on technical foundation from Assembly Language for x86 Processors 7th Edition
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef X86_CALLING_ABI_H
#define X86_CALLING_ABI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// CALLING CONVENTION DEFINITIONS
// =============================================================================

/**
 * @brief Supported calling conventions
 */
typedef enum {
    X86_CALL_CONV_CDECL = 0,    ///< C calling convention (caller cleanup)
    X86_CALL_CONV_STDCALL,      ///< Standard calling convention (callee cleanup)
    X86_CALL_CONV_FASTCALL,     ///< Fast calling convention (register parameters)
    X86_CALL_CONV_X64_MS,       ///< Microsoft x64 calling convention
    X86_CALL_CONV_X64_SYSV,     ///< System V x64 calling convention (Linux)
    X86_CALL_CONV_COUNT         ///< Total number of calling conventions
} x86_calling_convention_t;

/**
 * @brief Parameter passing methods
 */
typedef enum {
    X86_PARAM_STACK = 0,        ///< Parameter passed on stack
    X86_PARAM_REGISTER,         ///< Parameter passed in register
    X86_PARAM_MEMORY,           ///< Parameter passed by memory reference
    X86_PARAM_SPLIT             ///< Parameter split between register and stack
} x86_param_method_t;

/**
 * @brief Parameter descriptor
 */
typedef struct {
    size_t size;                ///< Parameter size in bytes
    size_t alignment;           ///< Parameter alignment requirement
    x86_param_method_t method;  ///< How parameter is passed
    int register_id;            ///< Register ID if passed in register (-1 if not)
    int stack_offset;           ///< Stack offset if passed on stack (-1 if not)
    bool is_return_param;       ///< True if this is a return parameter
} x86_param_descriptor_t;

/**
 * @brief Function signature descriptor
 */
typedef struct {
    x86_calling_convention_t convention;    ///< Calling convention used
    size_t param_count;                     ///< Number of parameters
    x86_param_descriptor_t *params;        ///< Parameter descriptors
    x86_param_descriptor_t return_value;   ///< Return value descriptor
    size_t stack_space_needed;             ///< Total stack space needed
    size_t shadow_space_size;              ///< Shadow space size (x64 only)
    bool has_varargs;                      ///< True if function has variable arguments
} x86_function_signature_t;

/**
 * @brief Stack frame layout
 */
typedef struct {
    uint32_t frame_size;        ///< Total frame size
    uint32_t local_vars_size;   ///< Size of local variables
    uint32_t param_area_size;   ///< Size of parameter area
    uint32_t shadow_space_size; ///< Size of shadow space (x64)
    uint32_t alignment_padding; ///< Padding for alignment
    bool has_frame_pointer;     ///< True if frame pointer is used
} x86_stack_frame_t;

/**
 * @brief ABI context for function calls
 */
typedef struct {
    x86_calling_convention_t convention;    ///< Active calling convention
    x86_stack_frame_t current_frame;       ///< Current stack frame
    uint32_t call_depth;                   ///< Current call depth
    bool in_function_call;                 ///< True if currently in function call
    void *stack_base;                      ///< Base of current stack
    void *stack_top;                       ///< Top of current stack
} x86_abi_context_t;

// =============================================================================
// REGISTER ASSIGNMENT TABLES
// =============================================================================

/**
 * @brief x64 Microsoft calling convention register assignments
 */
typedef enum {
    X64_MS_REG_RCX = 0,     ///< First integer parameter
    X64_MS_REG_RDX,         ///< Second integer parameter
    X64_MS_REG_R8,          ///< Third integer parameter
    X64_MS_REG_R9,          ///< Fourth integer parameter
    X64_MS_REG_COUNT        ///< Total parameter registers
} x64_ms_param_reg_t;

/**
 * @brief x64 System V calling convention register assignments
 */
typedef enum {
    X64_SYSV_REG_RDI = 0,   ///< First integer parameter
    X64_SYSV_REG_RSI,       ///< Second integer parameter
    X64_SYSV_REG_RDX,       ///< Third integer parameter
    X64_SYSV_REG_RCX,       ///< Fourth integer parameter
    X64_SYSV_REG_R8,        ///< Fifth integer parameter
    X64_SYSV_REG_R9,        ///< Sixth integer parameter
    X64_SYSV_REG_COUNT      ///< Total parameter registers
} x64_sysv_param_reg_t;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize x86 calling convention and ABI system
 * @return Status code (0 = success, negative = error)
 */
int x86_calling_abi_initialize(void);

/**
 * @brief Shutdown x86 calling convention and ABI system
 * @return Status code (0 = success, negative = error)
 */
int x86_calling_abi_shutdown(void);

/**
 * @brief Create function signature descriptor
 * @param convention Calling convention to use
 * @param param_types Array of parameter type sizes
 * @param param_count Number of parameters
 * @param return_type_size Size of return type
 * @param has_varargs True if function has variable arguments
 * @return Pointer to function signature or NULL on error
 */
x86_function_signature_t *x86_create_function_signature(
    x86_calling_convention_t convention,
    const size_t *param_types,
    size_t param_count,
    size_t return_type_size,
    bool has_varargs
);

/**
 * @brief Destroy function signature descriptor
 * @param signature Pointer to function signature to destroy
 */
void x86_destroy_function_signature(x86_function_signature_t *signature);

/**
 * @brief Setup function call according to calling convention
 * @param signature Function signature descriptor
 * @param params Array of parameter values
 * @return Status code (0 = success, negative = error)
 */
int x86_setup_function_call(const x86_function_signature_t *signature, const void **params);

/**
 * @brief Cleanup after function call
 * @param signature Function signature descriptor
 * @return Status code (0 = success, negative = error)
 */
int x86_cleanup_function_call(const x86_function_signature_t *signature);

/**
 * @brief Create stack frame for function
 * @param local_vars_size Size of local variables
 * @param param_area_size Size of parameter area
 * @param convention Calling convention
 * @return Pointer to stack frame descriptor or NULL on error
 */
x86_stack_frame_t *x86_create_stack_frame(
    size_t local_vars_size,
    size_t param_area_size,
    x86_calling_convention_t convention
);

/**
 * @brief Destroy stack frame
 * @param frame Pointer to stack frame to destroy
 */
void x86_destroy_stack_frame(x86_stack_frame_t *frame);

/**
 * @brief Get parameter register for calling convention
 * @param convention Calling convention
 * @param param_index Parameter index (0-based)
 * @return Register ID or -1 if parameter goes on stack
 */
int x86_get_param_register(x86_calling_convention_t convention, int param_index);

/**
 * @brief Calculate stack offset for parameter
 * @param signature Function signature
 * @param param_index Parameter index
 * @return Stack offset in bytes or -1 if parameter is in register
 */
int x86_get_param_stack_offset(const x86_function_signature_t *signature, int param_index);

/**
 * @brief Get calling convention name
 * @param convention Calling convention
 * @return String name of calling convention
 */
const char *x86_get_calling_convention_name(x86_calling_convention_t convention);

/**
 * @brief Check if calling convention uses shadow space
 * @param convention Calling convention
 * @return True if shadow space is used
 */
bool x86_uses_shadow_space(x86_calling_convention_t convention);

/**
 * @brief Get shadow space size for calling convention
 * @param convention Calling convention
 * @return Shadow space size in bytes
 */
size_t x86_get_shadow_space_size(x86_calling_convention_t convention);

/**
 * @brief Generate function prologue code
 * @param frame Stack frame descriptor
 * @param code_buffer Buffer to store generated code
 * @param buffer_size Size of code buffer
 * @return Number of bytes generated or negative error code
 */
int x86_generate_function_prologue(
    const x86_stack_frame_t *frame,
    uint8_t *code_buffer,
    size_t buffer_size
);

/**
 * @brief Generate function epilogue code
 * @param frame Stack frame descriptor
 * @param code_buffer Buffer to store generated code
 * @param buffer_size Size of code buffer
 * @return Number of bytes generated or negative error code
 */
int x86_generate_function_epilogue(
    const x86_stack_frame_t *frame,
    uint8_t *code_buffer,
    size_t buffer_size
);

/**
 * @brief Get current ABI context
 * @return Pointer to current ABI context
 */
x86_abi_context_t *x86_get_abi_context(void);

/**
 * @brief Set calling convention for current context
 * @param convention Calling convention to set
 * @return Status code (0 = success, negative = error)
 */
int x86_set_calling_convention(x86_calling_convention_t convention);

/**
 * @brief Validate function signature
 * @param signature Function signature to validate
 * @return True if signature is valid
 */
bool x86_validate_function_signature(const x86_function_signature_t *signature);

// =============================================================================
// INLINE HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Check if parameter fits in register
 * @param param_size Parameter size in bytes
 * @param convention Calling convention
 * @return True if parameter can fit in register
 */
static inline bool x86_param_fits_in_register(size_t param_size, x86_calling_convention_t convention)
{
    switch (convention) {
        case X86_CALL_CONV_FASTCALL:
            return param_size <= 4; // 32-bit registers
        case X86_CALL_CONV_X64_MS:
        case X86_CALL_CONV_X64_SYSV:
            return param_size <= 8; // 64-bit registers
        default:
            return false; // Stack-based conventions
    }
}

/**
 * @brief Get stack alignment for calling convention
 * @param convention Calling convention
 * @return Stack alignment in bytes
 */
static inline size_t x86_get_stack_alignment(x86_calling_convention_t convention)
{
    switch (convention) {
        case X86_CALL_CONV_X64_MS:
        case X86_CALL_CONV_X64_SYSV:
            return 16; // 16-byte alignment for x64
        default:
            return 4;  // 4-byte alignment for x86
    }
}

/**
 * @brief Check if calling convention is 64-bit
 * @param convention Calling convention
 * @return True if 64-bit calling convention
 */
static inline bool x86_is_64bit_convention(x86_calling_convention_t convention)
{
    return (convention == X86_CALL_CONV_X64_MS || convention == X86_CALL_CONV_X64_SYSV);
}

#ifdef __cplusplus
}
#endif

#endif // X86_CALLING_ABI_H
