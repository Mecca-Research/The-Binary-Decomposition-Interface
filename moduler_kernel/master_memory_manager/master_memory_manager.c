
/**
 * @file master_memory_manager.c
 * @brief Master Memory Manager - Phase 1 Core Implementation
 * 
 * Master Memory Manager: AI Assembly Engineers for BDI
 * Core implementation providing x86 competencies and HAL framework
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#include "master_memory_manager.h"
#include <string.h>
#include <stdio.h>

// =============================================================================
// GLOBAL CONTEXT
// =============================================================================

static mmm_context_t g_mmm_context = {0};
static bool g_mmm_initialized = false;

// =============================================================================
// STATUS STRING MAPPING
// =============================================================================

static const char *status_strings[] = {
    [MMM_SUCCESS] = "Success",
    [MMM_ERROR_INVALID_PARAM] = "Invalid parameter",
    [MMM_ERROR_NOT_INITIALIZED] = "Not initialized",
    [MMM_ERROR_HARDWARE_FAULT] = "Hardware fault",
    [MMM_ERROR_MEMORY_FAULT] = "Memory fault",
    [MMM_ERROR_TLB_MISS] = "TLB miss",
    [MMM_ERROR_PAGE_FAULT] = "Page fault",
    [MMM_ERROR_REGISTER_CONFLICT] = "Register conflict",
    [MMM_ERROR_ABI_VIOLATION] = "ABI violation",
    [MMM_ERROR_CACHE_MISS] = "Cache miss",
    [MMM_ERROR_PERIPHERAL_FAULT] = "Peripheral fault",
    [MMM_ERROR_BSP_FAULT] = "BSP fault",
    [MMM_ERROR_HAL_FAULT] = "HAL fault",
    [MMM_ERROR_UNKNOWN] = "Unknown error"
};

// =============================================================================
// PRIVATE FUNCTION DECLARATIONS
// =============================================================================

static mmm_status_t mmm_validate_config(const mmm_config_t *config);
static mmm_status_t mmm_initialize_x86_core(void);
static mmm_status_t mmm_initialize_hal_framework(void);
static mmm_status_t mmm_initialize_system_integration(void);
static void mmm_cleanup_resources(void);

// =============================================================================
// PUBLIC API IMPLEMENTATION
// =============================================================================

mmm_status_t mmm_initialize(const mmm_config_t *config)
{
    mmm_status_t status;
    
    // Validate input parameters
    if (config == NULL) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    // Check if already initialized
    if (g_mmm_initialized) {
        return MMM_SUCCESS;
    }
    
    // Validate configuration
    status = mmm_validate_config(config);
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    // Initialize context
    memset(&g_mmm_context, 0, sizeof(mmm_context_t));
    memcpy(&g_mmm_context.config, config, sizeof(mmm_config_t));
    
    // Initialize x86 core competencies
    if (config->enable_x86_core) {
        status = mmm_initialize_x86_core();
        if (status != MMM_SUCCESS) {
            mmm_cleanup_resources();
            return status;
        }
    }
    
    // Initialize HAL framework
    if (config->enable_hal_framework) {
        status = mmm_initialize_hal_framework();
        if (status != MMM_SUCCESS) {
            mmm_cleanup_resources();
            return status;
        }
    }
    
    // Initialize system integration
    status = mmm_initialize_system_integration();
    if (status != MMM_SUCCESS) {
        mmm_cleanup_resources();
        return status;
    }
    
    // Mark as initialized
    g_mmm_context.initialized = true;
    g_mmm_initialized = true;
    
    return MMM_SUCCESS;
}

mmm_status_t mmm_shutdown(void)
{
    if (!g_mmm_initialized) {
        return MMM_ERROR_NOT_INITIALIZED;
    }
    
    // Cleanup resources
    mmm_cleanup_resources();
    
    // Reset context
    memset(&g_mmm_context, 0, sizeof(mmm_context_t));
    g_mmm_initialized = false;
    
    return MMM_SUCCESS;
}

mmm_context_t *mmm_get_context(void)
{
    if (!g_mmm_initialized) {
        return NULL;
    }
    
    return &g_mmm_context;
}

const char *mmm_get_version(void)
{
    return MMM_VERSION_STRING;
}

const char *mmm_status_to_string(mmm_status_t status)
{
    if (status >= sizeof(status_strings) / sizeof(status_strings[0])) {
        return "Invalid status code";
    }
    
    return status_strings[status];
}

// =============================================================================
// PRIVATE FUNCTION IMPLEMENTATIONS
// =============================================================================

static mmm_status_t mmm_validate_config(const mmm_config_t *config)
{
    // Validate memory pool size
    if (config->memory_pool_size == 0 || config->memory_pool_size > (1UL << 30)) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    // Validate TLB cache size
    if (config->tlb_cache_size == 0 || config->tlb_cache_size > 1024) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    // Validate page size (must be power of 2, minimum 4KB)
    if (config->page_size < 4096 || (config->page_size & (config->page_size - 1)) != 0) {
        return MMM_ERROR_INVALID_PARAM;
    }
    
    return MMM_SUCCESS;
}

static mmm_status_t mmm_initialize_x86_core(void)
{
    mmm_status_t status;
    
    // Initialize x86 registers management
    status = x86_registers_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    // Initialize calling convention and ABI
    status = x86_calling_abi_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    // Initialize paging and MMU
    status = x86_paging_mmu_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    // Initialize TLB management
    status = x86_tlb_mgmt_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    // Initialize cache hints
    status = x86_cache_hints_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    return MMM_SUCCESS;
}

static mmm_status_t mmm_initialize_hal_framework(void)
{
    mmm_status_t status;
    
    // Initialize BSP layer
    status = mmm_bsp_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    // Initialize peripheral drivers
    status = mmm_peripheral_drivers_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    // Initialize hardware access layer
    status = mmm_hardware_access_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    return MMM_SUCCESS;
}

static mmm_status_t mmm_initialize_system_integration(void)
{
    mmm_status_t status;
    
    // Initialize interrupt management
    status = mmm_interrupt_mgmt_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    // Initialize memory protection
    status = mmm_memory_protection_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    // Initialize performance optimization
    status = mmm_performance_initialize();
    if (status != MMM_SUCCESS) {
        return status;
    }
    
    return MMM_SUCCESS;
}

static void mmm_cleanup_resources(void)
{
    // Cleanup x86 core components
    if (g_mmm_context.config.enable_x86_core) {
        x86_cache_hints_shutdown();
        x86_tlb_mgmt_shutdown();
        x86_paging_mmu_shutdown();
        x86_calling_abi_shutdown();
        x86_registers_shutdown();
    }
    
    // Cleanup HAL framework components
    if (g_mmm_context.config.enable_hal_framework) {
        mmm_hardware_access_shutdown();
        mmm_peripheral_drivers_shutdown();
        mmm_bsp_shutdown();
    }
    
    // Cleanup system integration components
    mmm_performance_shutdown();
    mmm_memory_protection_shutdown();
    mmm_interrupt_mgmt_shutdown();
}
