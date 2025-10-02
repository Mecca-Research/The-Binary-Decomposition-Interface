
// ===================================================================
// BDI Kernel - Virtual Memory Manager (Phase 2)
// C23 Enhanced with NUMA-Aware Virtual Memory Management
// ===================================================================
#ifndef BDI_VMM_H
#define BDI_VMM_H

#include "c23_compat.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// ===================================================================
// VMM Constants (Compile-time constants)
// ===================================================================

#define VMM_PAGE_SIZE 4096
#define VMM_HUGE_PAGE_SIZE (2 * 1024 * 1024)
#define VMM_ADDRESS_SPACE_SIZE (1ULL << 48)  // 256TB
#define VMM_INITIAL_ADDRESS_SPACE (4ULL << 30)  // 4 GB initial address space
#define VMM_MAX_REGIONS 1024

// Virtual memory flags
#define VM_READ 0x01
#define VM_WRITE 0x02
#define VM_EXEC 0x04
#define VM_USER 0x08
#define VM_HUGE 0x10
#define VM_LOCKED 0x20

// ===================================================================
// VMM Structures (C23 Enhanced)
// ===================================================================

// Virtual memory region
typedef struct VmRegion {
    void* start;                   // Virtual start address
    void* end;                     // Virtual end address
    size_t size;                   // Region size
    uint32_t flags;                // Access flags
    int numa_node;                 // NUMA node affinity
    _Atomic uint32_t ref_count;    // Reference count
} VmRegion;

_Static_assert(sizeof(VmRegion) <= 64, "VmRegion should fit in cache line");

// Page table entry
typedef struct PageTableEntry {
    uint64_t physical_addr;        // Physical address
    uint32_t flags;                // Page flags
    uint16_t numa_node;            // NUMA node
    uint16_t reserved;             // Reserved
} PageTableEntry;

_Static_assert(sizeof(PageTableEntry) == 16, "PageTableEntry must be 16 bytes");

// VMM statistics
typedef struct VmmStats {
    _Atomic size_t total_regions;
    _Atomic size_t active_regions;
    _Atomic size_t total_mapped;
    _Atomic size_t numa_local_maps;
    _Atomic size_t numa_remote_maps;
    _Atomic size_t huge_page_maps;
} VmmStats;

// ===================================================================
// VMM Functions (C23 [[nodiscard]])
// ===================================================================

// VMM initialization
NODISCARD int vmm_init(void);
void vmm_shutdown(void);

// Virtual memory mapping
NODISCARD void* vmm_map(void* physical, size_t size, uint32_t flags);
NODISCARD void* vmm_map_numa(void* physical, size_t size, uint32_t flags, int numa_node);
NODISCARD int vmm_unmap(void* virtual_addr, size_t size);

// Virtual memory allocation
NODISCARD void* vmm_alloc(size_t size, uint32_t flags);
NODISCARD void* vmm_alloc_numa(size_t size, uint32_t flags, int numa_node);
NODISCARD int vmm_free(void* addr, size_t size);

// Region management
NODISCARD VmRegion* vmm_find_region(void* addr);
NODISCARD int vmm_protect(void* addr, size_t size, uint32_t flags);
NODISCARD bool vmm_is_mapped(void* addr);

// Address translation
NODISCARD void* vmm_virt_to_phys(void* virtual_addr);
NODISCARD void* vmm_phys_to_virt(void* physical_addr);

// Statistics
NODISCARD VmmStats vmm_get_stats(void);
void vmm_print_stats(void);

// ===================================================================
// Compile-Time Validations
// ===================================================================

_Static_assert(VMM_PAGE_SIZE == 4096, "VMM page size must be 4KB");
_Static_assert(VMM_HUGE_PAGE_SIZE % VMM_PAGE_SIZE == 0, "Huge page must be page-aligned");
_Static_assert(VMM_ADDRESS_SPACE_SIZE > 0, "Address space must be positive");

#endif // BDI_VMM_H
