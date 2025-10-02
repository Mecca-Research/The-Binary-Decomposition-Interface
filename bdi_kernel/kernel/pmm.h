
// ===================================================================
// BDI Kernel - Physical Memory Manager (Phase 2)
// C23 Enhanced with NUMA-Aware Physical Memory Allocation
// ===================================================================
#ifndef BDI_PMM_H
#define BDI_PMM_H

#include "c23_compat.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// ===================================================================
// PMM Constants (Compile-time constants)
// ===================================================================

#define PMM_PAGE_SIZE 4096
#define PMM_HUGE_PAGE_2MB (2 * 1024 * 1024)
#define PMM_HUGE_PAGE_1GB (1024 * 1024 * 1024)
#define PMM_MAX_PAGES (1024 * 1024)  // 4GB worth of 4KB pages
#define PMM_MAX_NUMA_NODES 8

// Page flags
#define PMM_PAGE_FREE 0x00
#define PMM_PAGE_USED 0x01
#define PMM_PAGE_RESERVED 0x02
#define PMM_PAGE_HUGE 0x04
#define PMM_PAGE_LOCKED 0x08

// ===================================================================
// PMM Structures (C23 Enhanced)
// ===================================================================

// Page descriptor (compact, 16 bytes)
typedef struct PageDescriptor {
    uint64_t physical_addr;        // Physical address
    uint32_t flags;                // Page flags
    uint16_t numa_node;            // NUMA node
    uint16_t ref_count;            // Reference count
} PageDescriptor;

_Static_assert(sizeof(PageDescriptor) == 16, "PageDescriptor must be 16 bytes");
_Static_assert(_Alignof(PageDescriptor) >= 8, "PageDescriptor alignment");

// NUMA node memory info
typedef struct NumaNodeInfo {
    size_t total_pages;            // Total pages in node
    size_t free_pages;             // Free pages in node
    size_t used_pages;             // Used pages in node
    uint64_t base_addr;            // Base physical address
    uint64_t end_addr;             // End physical address
} NumaNodeInfo;

// PMM statistics
typedef struct PmmStats {
    _Atomic size_t total_pages;
    _Atomic size_t free_pages;
    _Atomic size_t used_pages;
    _Atomic size_t huge_pages;
    _Atomic size_t numa_local_allocs;
    _Atomic size_t numa_remote_allocs;
} PmmStats;

// ===================================================================
// PMM Functions (C23 [[nodiscard]])
// ===================================================================

// PMM initialization
NODISCARD int pmm_init(void);
void pmm_shutdown(void);

// Page allocation
NODISCARD void* pmm_alloc_page(int numa_node);
NODISCARD void* pmm_alloc_pages(size_t count, int numa_node);
NODISCARD void* pmm_alloc_huge_page(size_t size, int numa_node);

// Page deallocation
NODISCARD int pmm_free_page(void* page);
NODISCARD int pmm_free_pages(void* pages, size_t count);
NODISCARD int pmm_free_huge_page(void* page, size_t size);

// Page information
NODISCARD PageDescriptor* pmm_get_page_descriptor(void* page);
NODISCARD bool pmm_is_page_free(void* page);
NODISCARD int pmm_get_page_numa_node(void* page);

// NUMA node information
NODISCARD NumaNodeInfo pmm_get_numa_info(int node);
NODISCARD int pmm_get_best_numa_node(void);

// Statistics
NODISCARD PmmStats pmm_get_stats(void);
void pmm_print_stats(void);

// ===================================================================
// Compile-Time Validations
// ===================================================================

_Static_assert(PMM_PAGE_SIZE == 4096, "PMM page size must be 4KB");
_Static_assert(PMM_HUGE_PAGE_2MB % PMM_PAGE_SIZE == 0, "2MB must be 4KB aligned");
_Static_assert(PMM_HUGE_PAGE_1GB % PMM_HUGE_PAGE_2MB == 0, "1GB must be 2MB aligned");
_Static_assert(PMM_MAX_PAGES > 0, "PMM must support at least one page");

#endif // BDI_PMM_H
