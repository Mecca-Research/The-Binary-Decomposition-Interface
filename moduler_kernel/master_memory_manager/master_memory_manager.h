
/**
 * @file master_memory_manager.h
 * @brief Master Memory Manager - Phase 2 Complete Implementation
 * 
 * Master Memory Manager: AI Assembly Engineers for BDI
 * Complete Phase 2 implementation with advanced x86 systems and full toolchain
 * 
 * Phase 2 Components:
 * - Advanced x86 systems (complete x86 ISA, interrupts & IDT/APIC, task switching, SIMD/AVX, atomics, DMA & PCIe)
 * - Complete toolchain (spec → synthesize → prove → bench workflow, multi-rail synthesis, hard validation, auto-rewrite loop)
 * - All bug fixes incorporated (proper status code mapping, 64-bit address handling, correct type usage)
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 2.0.0
 */

#ifndef MASTER_MEMORY_MANAGER_H
#define MASTER_MEMORY_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// MASTER MEMORY MANAGER CORE DEFINITIONS
// =============================================================================

/**
 * @brief Master Memory Manager version information
 */
#define MMM_VERSION_MAJOR    1
#define MMM_VERSION_MINOR    0
#define MMM_VERSION_PATCH    0
#define MMM_VERSION_STRING   "1.0.0-phase1"

/**
 * @brief Master Memory Manager status codes
 */
typedef enum {
    MMM_SUCCESS = 0,
    MMM_ERROR_INVALID_PARAM,
    MMM_ERROR_NOT_INITIALIZED,
    MMM_ERROR_HARDWARE_FAULT,
    MMM_ERROR_MEMORY_FAULT,
    MMM_ERROR_TLB_MISS,
    MMM_ERROR_PAGE_FAULT,
    MMM_ERROR_REGISTER_CONFLICT,
    MMM_ERROR_ABI_VIOLATION,
    MMM_ERROR_CACHE_MISS,
    MMM_ERROR_PERIPHERAL_FAULT,
    MMM_ERROR_BSP_FAULT,
    MMM_ERROR_HAL_FAULT,
    MMM_ERROR_UNKNOWN
} mmm_status_t;

/**
 * @brief Master Memory Manager configuration structure
 */
typedef struct {
    bool enable_x86_core;           ///< Enable x86 core competencies
    bool enable_hal_framework;      ///< Enable HAL framework
    bool enable_debug_mode;         ///< Enable debug and tracing
    bool enable_performance_opt;    ///< Enable performance optimizations
    uint32_t memory_pool_size;      ///< Memory pool size in bytes
    uint32_t tlb_cache_size;        ///< TLB cache size entries
    uint32_t page_size;             ///< Page size (4KB default)
} mmm_config_t;

/**
 * @brief Master Memory Manager context structure
 */
typedef struct {
    mmm_config_t config;            ///< Configuration
    bool initialized;               ///< Initialization status
    uint32_t error_count;           ///< Error counter
    uint64_t performance_counter;   ///< Performance counter
    void *x86_core_ctx;            ///< x86 core context
    void *hal_framework_ctx;       ///< HAL framework context
    void *system_integration_ctx;  ///< System integration context
} mmm_context_t;

// =============================================================================
// CORE API FUNCTIONS
// =============================================================================

/**
 * @brief Initialize Master Memory Manager
 * @param config Configuration structure
 * @return Status code
 */
mmm_status_t mmm_initialize(const mmm_config_t *config);

/**
 * @brief Shutdown Master Memory Manager
 * @return Status code
 */
mmm_status_t mmm_shutdown(void);

/**
 * @brief Get Master Memory Manager context
 * @return Pointer to context structure
 */
mmm_context_t *mmm_get_context(void);

/**
 * @brief Get version information
 * @return Version string
 */
const char *mmm_get_version(void);

/**
 * @brief Get status string from status code
 * @param status Status code
 * @return Status string
 */
const char *mmm_status_to_string(mmm_status_t status);

// =============================================================================
// COMPONENT INCLUDES
// =============================================================================

// x86 Core Competencies - Phase 1
#include "x86_core/registers/x86_registers.h"
#include "x86_core/calling_abi/x86_calling_abi.h"
#include "x86_core/paging_mmu/x86_paging_mmu.h"
#include "x86_core/tlb_mgmt/x86_tlb_mgmt.h"
#include "x86_core/cache_hints/x86_cache_hints.h"

// x86 Advanced Systems - Phase 2
#include "x86_core/interrupts_idt/x86_interrupts_idt.h"
#include "x86_core/task_switching/x86_task_switching.h"
#include "x86_core/simd_avx/x86_simd_avx.h"
#include "x86_core/atomic_ops/x86_atomic_ops.h"
#include "x86_core/dma_pcie/x86_dma_pcie.h"

// Complete Toolchain - Phase 2
#include "toolchain/bdi_parser/bdi_parser.h"
#include "toolchain/multi_rail_synthesis/multi_rail_synthesis.h"

// HAL Framework
#include "hal_framework/bsp/mmm_bsp.h"
#include "hal_framework/peripheral_drivers/mmm_peripheral_drivers.h"
#include "hal_framework/hardware_access/mmm_hardware_access.h"

// System Integration
#include "system_integration/interrupt_mgmt/mmm_interrupt_mgmt.h"
#include "system_integration/memory_protection/mmm_memory_protection.h"
#include "system_integration/performance/mmm_performance.h"

#ifdef __cplusplus
}
#endif

#endif // MASTER_MEMORY_MANAGER_H
