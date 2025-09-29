
/**
 * @file x86_calling_abi.c
 * @brief x86 Calling Convention and ABI Management Implementation
 * 
 * Implementation of comprehensive x86 calling convention support
 * providing parameter passing, stack management, and ABI compliance.
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#include "x86_calling_abi.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// =============================================================================
// GLOBAL STATE
// =============================================================================

static bool g_x86_abi_initialized = false;
static x86_abi_context_t g_abi_context = {0};

// =============================================================================
// CALLING CONVENTION TABLES
// =============================================================================

static const char *calling_convention_names[] = {
    [X86_CALL_CONV_CDECL] = "cdecl",
    [X86_CALL_CONV_STDCALL] = "stdcall", 
    [X86_CALL_CONV_FASTCALL] = "fastcall",
    [X86_CALL_CONV_X64_MS] = "x64_ms",
    [X86_CALL_CONV_X64_SYSV] = "x64_sysv"
};

// x64 Microsoft calling convention parameter registers
static const int x64_ms_param_registers[] = {
    1, // RCX
    3, // RDX  
    8, // R8
    9  // R9
};

// x64 System V calling convention parameter registers
static const int x64_sysv_param_registers[] = {
    6, // RDI
    4, // RSI
    3, // RDX
    1, // RCX
    8, // R8
    9  // R9
};

// =============================================================================
// PRIVATE FUNCTION DECLARATIONS
// =============================================================================

static size_t x86_align_size(size_t size, size_t alignment);
static int x86_calculate_param_layout(x86_function_signature_t *signature);
static size_t x86_calculate_stack_space(const x86_function_signature_t *signature);

// =============================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// =============================================================================

int x86_calling_abi_initialize(void)
{
    if (g_x86_abi_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize ABI context
    memset(&g_abi_context, 0, sizeof(x86_abi_context_t));
    g_abi_context.convention = X86_CALL_CONV_CDECL; // Default convention
    
    g_x86_abi_initialized = true;
    return 0;
}

int x86_calling_abi_shutdown(void)
{
    if (!g_x86_abi_initialized) {
        return -1; // Not initialized
    }
    
    // Cleanup any allocated resources
    memset(&g_abi_context, 0, sizeof(x86_abi_context_t));
    
    g_x86_abi_initialized = false;
    return 0;
}

x86_function_signature_t *x86_create_function_signature(
    x86_calling_convention_t convention,
    const size_t *param_types,
    size_t param_count,
    size_t return_type_size,
    bool has_varargs)
{
    if (!g_x86_abi_initialized || param_types == NULL) {
        return NULL;
    }
    
    // Allocate signature structure
    x86_function_signature_t *signature = calloc(1, sizeof(x86_function_signature_t));
    if (signature == NULL) {
        return NULL;
    }
    
    // Allocate parameter descriptors
    if (param_count > 0) {
        signature->params = calloc(param_count, sizeof(x86_param_descriptor_t));
        if (signature->params == NULL) {
            free(signature);
            return NULL;
        }
    }
    
    // Initialize signature
    signature->convention = convention;
    signature->param_count = param_count;
    signature->has_varargs = has_varargs;
    
    // Initialize parameter descriptors
    for (size_t i = 0; i < param_count; i++) {
        signature->params[i].size = param_types[i];
        signature->params[i].alignment = (param_types[i] >= 8) ? 8 : param_types[i];
        signature->params[i].method = X86_PARAM_STACK; // Default to stack
        signature->params[i].register_id = -1;
        signature->params[i].stack_offset = -1;
        signature->params[i].is_return_param = false;
    }
    
    // Initialize return value descriptor
    signature->return_value.size = return_type_size;
    signature->return_value.alignment = (return_type_size >= 8) ? 8 : return_type_size;
    signature->return_value.method = X86_PARAM_REGISTER; // Usually in EAX/RAX
    signature->return_value.register_id = 0; // EAX/RAX
    signature->return_value.stack_offset = -1;
    signature->return_value.is_return_param = true;
    
    // Calculate parameter layout
    if (x86_calculate_param_layout(signature) != 0) {
        x86_destroy_function_signature(signature);
        return NULL;
    }
    
    return signature;
}

void x86_destroy_function_signature(x86_function_signature_t *signature)
{
    if (signature == NULL) {
        return;
    }
    
    if (signature->params != NULL) {
        free(signature->params);
    }
    
    free(signature);
}

int x86_setup_function_call(const x86_function_signature_t *signature, const void **params)
{
    if (!g_x86_abi_initialized || signature == NULL || params == NULL) {
        return -1;
    }
    
    // Mark that we're in a function call
    g_abi_context.in_function_call = true;
    g_abi_context.call_depth++;
    
    // Setup parameters according to calling convention
    for (size_t i = 0; i < signature->param_count; i++) {
        const x86_param_descriptor_t *param = &signature->params[i];
        
        if (param->method == X86_PARAM_REGISTER) {
            // Parameter goes in register - would use inline assembly here
            // This is a placeholder for the actual register setup
        } else if (param->method == X86_PARAM_STACK) {
            // Parameter goes on stack - would manipulate stack here
            // This is a placeholder for the actual stack setup
        }
    }
    
    return 0;
}

int x86_cleanup_function_call(const x86_function_signature_t *signature)
{
    if (!g_x86_abi_initialized || signature == NULL) {
        return -1;
    }
    
    // Cleanup stack if caller is responsible (cdecl)
    if (signature->convention == X86_CALL_CONV_CDECL) {
        // Would adjust stack pointer here
        // This is a placeholder for actual stack cleanup
    }
    
    // Update call state
    if (g_abi_context.call_depth > 0) {
        g_abi_context.call_depth--;
    }
    
    if (g_abi_context.call_depth == 0) {
        g_abi_context.in_function_call = false;
    }
    
    return 0;
}

x86_stack_frame_t *x86_create_stack_frame(
    size_t local_vars_size,
    size_t param_area_size,
    x86_calling_convention_t convention)
{
    if (!g_x86_abi_initialized) {
        return NULL;
    }
    
    x86_stack_frame_t *frame = calloc(1, sizeof(x86_stack_frame_t));
    if (frame == NULL) {
        return NULL;
    }
    
    // Calculate frame components
    frame->local_vars_size = (uint32_t)local_vars_size;
    frame->param_area_size = (uint32_t)param_area_size;
    frame->shadow_space_size = (uint32_t)x86_get_shadow_space_size(convention);
    frame->has_frame_pointer = true; // Usually use frame pointer
    
    // Calculate alignment padding
    size_t alignment = x86_get_stack_alignment(convention);
    size_t total_size = local_vars_size + param_area_size + frame->shadow_space_size;
    size_t aligned_size = x86_align_size(total_size, alignment);
    frame->alignment_padding = (uint32_t)(aligned_size - total_size);
    
    // Calculate total frame size
    frame->frame_size = (uint32_t)aligned_size;
    
    return frame;
}

void x86_destroy_stack_frame(x86_stack_frame_t *frame)
{
    if (frame != NULL) {
        free(frame);
    }
}

int x86_get_param_register(x86_calling_convention_t convention, int param_index)
{
    switch (convention) {
        case X86_CALL_CONV_FASTCALL:
            // First two parameters in ECX, EDX
            if (param_index == 0) return 1; // ECX
            if (param_index == 1) return 3; // EDX
            break;
            
        case X86_CALL_CONV_X64_MS:
            if (param_index >= 0 && param_index < 4) {
                return x64_ms_param_registers[param_index];
            }
            break;
            
        case X86_CALL_CONV_X64_SYSV:
            if (param_index >= 0 && param_index < 6) {
                return x64_sysv_param_registers[param_index];
            }
            break;
            
        default:
            break;
    }
    
    return -1; // Parameter goes on stack
}

int x86_get_param_stack_offset(const x86_function_signature_t *signature, int param_index)
{
    if (signature == NULL || param_index < 0 || param_index >= (int)signature->param_count) {
        return -1;
    }
    
    return signature->params[param_index].stack_offset;
}

const char *x86_get_calling_convention_name(x86_calling_convention_t convention)
{
    if (convention >= 0 && convention < X86_CALL_CONV_COUNT) {
        return calling_convention_names[convention];
    }
    return "unknown";
}

bool x86_uses_shadow_space(x86_calling_convention_t convention)
{
    return (convention == X86_CALL_CONV_X64_MS);
}

size_t x86_get_shadow_space_size(x86_calling_convention_t convention)
{
    if (convention == X86_CALL_CONV_X64_MS) {
        return 32; // 4 * 8 bytes for register parameters
    }
    return 0;
}

int x86_generate_function_prologue(
    const x86_stack_frame_t *frame,
    uint8_t *code_buffer,
    size_t buffer_size)
{
    if (frame == NULL || code_buffer == NULL || buffer_size < 16) {
        return -1;
    }
    
    // This is a placeholder implementation
    // In a real implementation, this would generate actual x86 machine code
    // for the function prologue (push ebp, mov ebp esp, sub esp, frame_size, etc.)
    
    int bytes_written = 0;
    
    // Placeholder: just return the number of bytes that would be written
    if (frame->has_frame_pointer) {
        bytes_written += 1; // push ebp/rbp
        bytes_written += 2; // mov ebp/rbp, esp/rsp
    }
    
    if (frame->frame_size > 0) {
        bytes_written += 6; // sub esp/rsp, frame_size
    }
    
    return bytes_written;
}

int x86_generate_function_epilogue(
    const x86_stack_frame_t *frame,
    uint8_t *code_buffer,
    size_t buffer_size)
{
    if (frame == NULL || code_buffer == NULL || buffer_size < 16) {
        return -1;
    }
    
    // This is a placeholder implementation
    // In a real implementation, this would generate actual x86 machine code
    // for the function epilogue (mov esp ebp, pop ebp, ret, etc.)
    
    int bytes_written = 0;
    
    if (frame->has_frame_pointer) {
        bytes_written += 2; // mov esp/rsp, ebp/rbp
        bytes_written += 1; // pop ebp/rbp
    } else if (frame->frame_size > 0) {
        bytes_written += 6; // add esp/rsp, frame_size
    }
    
    bytes_written += 1; // ret
    
    return bytes_written;
}

x86_abi_context_t *x86_get_abi_context(void)
{
    if (!g_x86_abi_initialized) {
        return NULL;
    }
    
    return &g_abi_context;
}

int x86_set_calling_convention(x86_calling_convention_t convention)
{
    if (!g_x86_abi_initialized || convention >= X86_CALL_CONV_COUNT) {
        return -1;
    }
    
    g_abi_context.convention = convention;
    return 0;
}

bool x86_validate_function_signature(const x86_function_signature_t *signature)
{
    if (signature == NULL) {
        return false;
    }
    
    // Check calling convention
    if (signature->convention >= X86_CALL_CONV_COUNT) {
        return false;
    }
    
    // Check parameter count
    if (signature->param_count > 0 && signature->params == NULL) {
        return false;
    }
    
    // Validate each parameter
    for (size_t i = 0; i < signature->param_count; i++) {
        const x86_param_descriptor_t *param = &signature->params[i];
        
        // Check parameter size
        if (param->size == 0 || param->size > 1024) {
            return false;
        }
        
        // Check alignment
        if (param->alignment == 0 || (param->alignment & (param->alignment - 1)) != 0) {
            return false;
        }
    }
    
    return true;
}

// =============================================================================
// PRIVATE FUNCTION IMPLEMENTATIONS
// =============================================================================

static size_t x86_align_size(size_t size, size_t alignment)
{
    return (size + alignment - 1) & ~(alignment - 1);
}

static int x86_calculate_param_layout(x86_function_signature_t *signature)
{
    if (signature == NULL) {
        return -1;
    }
    
    int stack_offset = 0;
    int reg_param_count = 0;
    
    // Calculate parameter layout based on calling convention
    for (size_t i = 0; i < signature->param_count; i++) {
        x86_param_descriptor_t *param = &signature->params[i];
        
        // Check if parameter can go in register
        int reg_id = x86_get_param_register(signature->convention, reg_param_count);
        
        if (reg_id >= 0 && x86_param_fits_in_register(param->size, signature->convention)) {
            // Parameter goes in register
            param->method = X86_PARAM_REGISTER;
            param->register_id = reg_id;
            param->stack_offset = -1;
            reg_param_count++;
        } else {
            // Parameter goes on stack
            param->method = X86_PARAM_STACK;
            param->register_id = -1;
            
            // Align stack offset
            stack_offset = (int)x86_align_size(stack_offset, param->alignment);
            param->stack_offset = stack_offset;
            stack_offset += (int)param->size;
        }
    }
    
    // Calculate total stack space needed
    signature->stack_space_needed = x86_calculate_stack_space(signature);
    signature->shadow_space_size = x86_get_shadow_space_size(signature->convention);
    
    return 0;
}

static size_t x86_calculate_stack_space(const x86_function_signature_t *signature)
{
    if (signature == NULL) {
        return 0;
    }
    
    size_t max_offset = 0;
    
    // Find the maximum stack offset
    for (size_t i = 0; i < signature->param_count; i++) {
        const x86_param_descriptor_t *param = &signature->params[i];
        
        if (param->method == X86_PARAM_STACK && param->stack_offset >= 0) {
            size_t param_end = param->stack_offset + param->size;
            if (param_end > max_offset) {
                max_offset = param_end;
            }
        }
    }
    
    // Align to stack alignment
    size_t alignment = x86_get_stack_alignment(signature->convention);
    return x86_align_size(max_offset, alignment);
}
