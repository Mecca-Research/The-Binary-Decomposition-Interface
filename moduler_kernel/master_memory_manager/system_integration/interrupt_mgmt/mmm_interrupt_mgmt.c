
/**
 * @file mmm_interrupt_mgmt.c
 * @brief Master Memory Manager Interrupt Management Implementation
 * 
 * Implementation of comprehensive interrupt management system.
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#include "mmm_interrupt_mgmt.h"
#include <stdlib.h>
#include <string.h>

// =============================================================================
// GLOBAL STATE
// =============================================================================

static bool g_mmm_int_initialized = false;
static mmm_interrupt_context_t g_int_context = {0};

// =============================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// =============================================================================

int mmm_interrupt_mgmt_initialize(void)
{
    if (g_mmm_int_initialized) {
        return 0;
    }
    
    memset(&g_int_context, 0, sizeof(mmm_interrupt_context_t));
    g_int_context.initialized = true;
    g_mmm_int_initialized = true;
    
    return 0;
}

int mmm_interrupt_mgmt_shutdown(void)
{
    if (!g_mmm_int_initialized) {
        return -1;
    }
    
    memset(&g_int_context, 0, sizeof(mmm_interrupt_context_t));
    g_mmm_int_initialized = false;
    
    return 0;
}

int mmm_interrupt_register_handler(uint8_t interrupt_id, uint8_t priority, 
                                   mmm_interrupt_handler_t handler, void *context, const char *name)
{
    if (!g_mmm_int_initialized || interrupt_id >= MMM_INT_MAX_INTERRUPTS || 
        priority > MMM_INT_MAX_PRIORITY || handler == NULL) {
        return -1;
    }
    
    mmm_interrupt_descriptor_t *desc = &g_int_context.interrupts[interrupt_id];
    desc->interrupt_id = interrupt_id;
    desc->priority = priority;
    desc->enabled = false;
    desc->handler = handler;
    desc->context = context;
    desc->call_count = 0;
    desc->total_cycles = 0;
    desc->max_cycles = 0;
    desc->name = name;
    
    return 0;
}

int mmm_interrupt_unregister_handler(uint8_t interrupt_id)
{
    if (!g_mmm_int_initialized || interrupt_id >= MMM_INT_MAX_INTERRUPTS) {
        return -1;
    }
    
    memset(&g_int_context.interrupts[interrupt_id], 0, sizeof(mmm_interrupt_descriptor_t));
    return 0;
}

int mmm_interrupt_enable(uint8_t interrupt_id)
{
    if (!g_mmm_int_initialized || interrupt_id >= MMM_INT_MAX_INTERRUPTS) {
        return -1;
    }
    
    g_int_context.interrupts[interrupt_id].enabled = true;
    return 0;
}

int mmm_interrupt_disable(uint8_t interrupt_id)
{
    if (!g_mmm_int_initialized || interrupt_id >= MMM_INT_MAX_INTERRUPTS) {
        return -1;
    }
    
    g_int_context.interrupts[interrupt_id].enabled = false;
    return 0;
}

mmm_interrupt_context_t *mmm_interrupt_get_context(void)
{
    if (!g_mmm_int_initialized) {
        return NULL;
    }
    
    return &g_int_context;
}
