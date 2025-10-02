
// ===================================================================
// BDI Kernel - Virtual Memory Manager Implementation (Phase 2)
// C23 Enhanced with NUMA-Aware Virtual Memory Management
// ===================================================================
#include "vmm.h"
#include "pmm.h"
#include "memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ===================================================================
// Global State
// ===================================================================

static bool vmm_initialized = false;
static VmmStats vmm_stats = {0};

// Virtual memory regions
static VmRegion regions[VMM_MAX_REGIONS];
static _Atomic size_t region_count = 0;

// Simple page table (for demonstration)
static PageTableEntry* page_table = NULL;
static size_t page_table_size = 0;

// ===================================================================
// VMM Initialization
// ===================================================================

NODISCARD int vmm_init(void) {
    if (vmm_initialized) {
        return 0;
    }
    
    printf("VMM: Initializing virtual memory manager\n");
    
    // Allocate page table
    page_table_size = VMM_ADDRESS_SPACE_SIZE / VMM_PAGE_SIZE;
    size_t table_bytes = page_table_size * sizeof(PageTableEntry);
    
    page_table = (PageTableEntry*)numa_alloc_local(table_bytes);
    if (page_table == NULL) {
        printf("VMM: Failed to allocate page table\n");
        return -1;
    }
    
    memset(page_table, 0, table_bytes);
    
    // Initialize regions
    memset(regions, 0, sizeof(regions));
    ATOMIC_STORE(&region_count, 0);
    
    // Initialize statistics
    ATOMIC_STORE(&vmm_stats.total_regions, 0);
    ATOMIC_STORE(&vmm_stats.active_regions, 0);
    ATOMIC_STORE(&vmm_stats.total_mapped, 0);
    ATOMIC_STORE(&vmm_stats.numa_local_maps, 0);
    ATOMIC_STORE(&vmm_stats.numa_remote_maps, 0);
    ATOMIC_STORE(&vmm_stats.huge_page_maps, 0);
    
    vmm_initialized = true;
    printf("VMM: Initialization complete\n");
    return 0;
}

void vmm_shutdown(void) {
    if (!vmm_initialized) {
        return;
    }
    
    if (page_table != NULL) {
        numa_free(page_table, page_table_size * sizeof(PageTableEntry));
        page_table = NULL;
    }
    
    vmm_initialized = false;
}

// ===================================================================
// Virtual Memory Mapping
// ===================================================================

NODISCARD void* vmm_map(void* physical, size_t size, uint32_t flags) {
    return vmm_map_numa(physical, size, flags, numa_current_node());
}

NODISCARD void* vmm_map_numa(void* physical, size_t size, uint32_t flags, int numa_node) {
    if (!vmm_initialized || physical == NULL || size == 0) {
        return NULL;
    }
    
    // Find free region
    size_t index = ATOMIC_FETCH_ADD(&region_count, 1);
    if (index >= VMM_MAX_REGIONS) {
        ATOMIC_FETCH_SUB(&region_count, 1);
        return NULL;
    }
    
    // Allocate virtual address space
    // In a real implementation, this would use a virtual address allocator
    // For now, use a simple linear allocation
    void* virtual_addr = (void*)(0x100000000ULL + (index * size));
    
    // Initialize region
    VmRegion* region = &regions[index];
    region->start = virtual_addr;
    region->end = (void*)((uintptr_t)virtual_addr + size);
    region->size = size;
    region->flags = flags;
    region->numa_node = numa_node;
    ATOMIC_STORE(&region->ref_count, 1);
    
    // Map pages in page table
    size_t num_pages = (size + VMM_PAGE_SIZE - 1) / VMM_PAGE_SIZE;
    for (size_t i = 0; i < num_pages; i++) {
        uintptr_t virt = (uintptr_t)virtual_addr + (i * VMM_PAGE_SIZE);
        uintptr_t phys = (uintptr_t)physical + (i * VMM_PAGE_SIZE);
        
        size_t page_index = virt / VMM_PAGE_SIZE;
        if (page_index < page_table_size) {
            page_table[page_index].physical_addr = phys;
            page_table[page_index].flags = flags;
            page_table[page_index].numa_node = (uint16_t)numa_node;
        }
    }
    
    // Update statistics
    ATOMIC_FETCH_ADD(&vmm_stats.total_regions, 1);
    ATOMIC_FETCH_ADD(&vmm_stats.active_regions, 1);
    ATOMIC_FETCH_ADD(&vmm_stats.total_mapped, size);
    
    if (numa_node == numa_current_node()) {
        ATOMIC_FETCH_ADD(&vmm_stats.numa_local_maps, 1);
    } else {
        ATOMIC_FETCH_ADD(&vmm_stats.numa_remote_maps, 1);
    }
    
    if (flags & VM_HUGE) {
        ATOMIC_FETCH_ADD(&vmm_stats.huge_page_maps, 1);
    }
    
    return virtual_addr;
}

NODISCARD int vmm_unmap(void* virtual_addr, size_t size) {
    if (!vmm_initialized || virtual_addr == NULL || size == 0) {
        return -1;
    }
    
    // Find region
    VmRegion* region = vmm_find_region(virtual_addr);
    if (region == NULL) {
        return -1;
    }
    
    // Decrement reference count
    uint32_t ref = ATOMIC_FETCH_SUB(&region->ref_count, 1);
    if (ref > 1) {
        return 0;  // Still referenced
    }
    
    // Unmap pages from page table
    size_t num_pages = (size + VMM_PAGE_SIZE - 1) / VMM_PAGE_SIZE;
    for (size_t i = 0; i < num_pages; i++) {
        uintptr_t virt = (uintptr_t)virtual_addr + (i * VMM_PAGE_SIZE);
        size_t page_index = virt / VMM_PAGE_SIZE;
        
        if (page_index < page_table_size) {
            memset(&page_table[page_index], 0, sizeof(PageTableEntry));
        }
    }
    
    // Clear region
    memset(region, 0, sizeof(VmRegion));
    
    // Update statistics
    ATOMIC_FETCH_SUB(&vmm_stats.active_regions, 1);
    ATOMIC_FETCH_SUB(&vmm_stats.total_mapped, size);
    
    return 0;
}

// ===================================================================
// Virtual Memory Allocation
// ===================================================================

NODISCARD void* vmm_alloc(size_t size, uint32_t flags) {
    return vmm_alloc_numa(size, flags, numa_current_node());
}

NODISCARD void* vmm_alloc_numa(size_t size, uint32_t flags, int numa_node) {
    if (size == 0) {
        return NULL;
    }
    
    // Allocate physical pages
    size_t num_pages = (size + VMM_PAGE_SIZE - 1) / VMM_PAGE_SIZE;
    void* physical = pmm_alloc_pages(num_pages, numa_node);
    if (physical == NULL) {
        return NULL;
    }
    
    // Map to virtual address space
    void* virtual_addr = vmm_map_numa(physical, size, flags, numa_node);
    if (virtual_addr == NULL) {
        pmm_free_pages(physical, num_pages);
        return NULL;
    }
    
    return virtual_addr;
}

NODISCARD int vmm_free(void* addr, size_t size) {
    if (addr == NULL || size == 0) {
        return -1;
    }
    
    // Get physical address
    void* physical = vmm_virt_to_phys(addr);
    if (physical == NULL) {
        return -1;
    }
    
    // Unmap virtual memory
    if (vmm_unmap(addr, size) != 0) {
        return -1;
    }
    
    // Free physical pages
    size_t num_pages = (size + VMM_PAGE_SIZE - 1) / VMM_PAGE_SIZE;
    return pmm_free_pages(physical, num_pages);
}

// ===================================================================
// Region Management
// ===================================================================

NODISCARD VmRegion* vmm_find_region(void* addr) {
    if (addr == NULL) {
        return NULL;
    }
    
    size_t count = ATOMIC_LOAD(&region_count);
    for (size_t i = 0; i < count && i < VMM_MAX_REGIONS; i++) {
        VmRegion* region = &regions[i];
        if (addr >= region->start && addr < region->end) {
            return region;
        }
    }
    
    return NULL;
}

NODISCARD int vmm_protect(void* addr, size_t size, uint32_t flags) {
    if (addr == NULL || size == 0) {
        return -1;
    }
    
    VmRegion* region = vmm_find_region(addr);
    if (region == NULL) {
        return -1;
    }
    
    // Update region flags
    region->flags = flags;
    
    // Update page table entries
    size_t num_pages = (size + VMM_PAGE_SIZE - 1) / VMM_PAGE_SIZE;
    for (size_t i = 0; i < num_pages; i++) {
        uintptr_t virt = (uintptr_t)addr + (i * VMM_PAGE_SIZE);
        size_t page_index = virt / VMM_PAGE_SIZE;
        
        if (page_index < page_table_size) {
            page_table[page_index].flags = flags;
        }
    }
    
    return 0;
}

NODISCARD bool vmm_is_mapped(void* addr) {
    if (addr == NULL) {
        return false;
    }
    
    size_t page_index = (uintptr_t)addr / VMM_PAGE_SIZE;
    if (page_index >= page_table_size) {
        return false;
    }
    
    return page_table[page_index].physical_addr != 0;
}

// ===================================================================
// Address Translation
// ===================================================================

NODISCARD void* vmm_virt_to_phys(void* virtual_addr) {
    if (virtual_addr == NULL) {
        return NULL;
    }
    
    size_t page_index = (uintptr_t)virtual_addr / VMM_PAGE_SIZE;
    if (page_index >= page_table_size) {
        return NULL;
    }
    
    uint64_t phys_page = page_table[page_index].physical_addr;
    if (phys_page == 0) {
        return NULL;
    }
    
    size_t offset = (uintptr_t)virtual_addr % VMM_PAGE_SIZE;
    return (void*)(phys_page + offset);
}

NODISCARD void* vmm_phys_to_virt(void* physical_addr) {
    if (physical_addr == NULL) {
        return NULL;
    }
    
    // Search page table for physical address
    uint64_t phys = (uint64_t)physical_addr & ~(VMM_PAGE_SIZE - 1);
    
    for (size_t i = 0; i < page_table_size; i++) {
        if (page_table[i].physical_addr == phys) {
            size_t offset = (uintptr_t)physical_addr % VMM_PAGE_SIZE;
            return (void*)((i * VMM_PAGE_SIZE) + offset);
        }
    }
    
    return NULL;
}

// ===================================================================
// Statistics
// ===================================================================

NODISCARD VmmStats vmm_get_stats(void) {
    VmmStats stats;
    stats.total_regions = ATOMIC_LOAD(&vmm_stats.total_regions);
    stats.active_regions = ATOMIC_LOAD(&vmm_stats.active_regions);
    stats.total_mapped = ATOMIC_LOAD(&vmm_stats.total_mapped);
    stats.numa_local_maps = ATOMIC_LOAD(&vmm_stats.numa_local_maps);
    stats.numa_remote_maps = ATOMIC_LOAD(&vmm_stats.numa_remote_maps);
    stats.huge_page_maps = ATOMIC_LOAD(&vmm_stats.huge_page_maps);
    return stats;
}

void vmm_print_stats(void) {
    VmmStats stats = vmm_get_stats();
    
    printf("\n=== VMM Statistics (Phase 2) ===\n");
    printf("Total Regions:       %zu\n", stats.total_regions);
    printf("Active Regions:      %zu\n", stats.active_regions);
    printf("Total Mapped:        %zu bytes\n", stats.total_mapped);
    printf("NUMA Local Maps:     %zu\n", stats.numa_local_maps);
    printf("NUMA Remote Maps:    %zu\n", stats.numa_remote_maps);
    printf("Huge Page Maps:      %zu\n", stats.huge_page_maps);
    printf("================================\n\n");
}
