
// ===================================================================
// BDI Kernel - Physical Memory Manager Implementation (Phase 2)
// C23 Enhanced with NUMA-Aware Physical Memory Allocation
// ===================================================================
#include "pmm.h"
#include "memory.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// ===================================================================
// Global State
// ===================================================================

static bool pmm_initialized = false;
static PmmStats pmm_stats = {0};

// Per-NUMA-node page lists
static PageDescriptor* numa_page_lists[PMM_MAX_NUMA_NODES];
static _Atomic size_t numa_page_counts[PMM_MAX_NUMA_NODES];
static NumaNodeInfo numa_info[PMM_MAX_NUMA_NODES];

// ===================================================================
// PMM Initialization
// ===================================================================

NODISCARD int pmm_init(void) {
    if (pmm_initialized) {
        return 0;
    }
    
    printf("PMM: Initializing physical memory manager\n");
    
    // Get number of NUMA nodes
    int num_nodes = numa_num_nodes();
    if (num_nodes <= 0 || num_nodes > PMM_MAX_NUMA_NODES) {
        printf("PMM: Invalid NUMA node count %d\n", num_nodes);
        return -1;
    }
    
    // Initialize per-NUMA-node page lists
    for (int i = 0; i < num_nodes; i++) {
        // Allocate page descriptor list on NUMA node
        size_t list_size = sizeof(PageDescriptor) * PMM_MAX_PAGES;
        numa_page_lists[i] = (PageDescriptor*)numa_alloc_onnode(list_size, i);
        
        if (numa_page_lists[i] == NULL) {
            printf("PMM: Failed to allocate page list for NUMA node %d\n", i);
            return -1;
        }
        
        // Initialize page descriptors
        memset(numa_page_lists[i], 0, list_size);
        ATOMIC_STORE(&numa_page_counts[i], 0);
        
        // Initialize NUMA node info
        numa_info[i].total_pages = PMM_MAX_PAGES;
        numa_info[i].free_pages = PMM_MAX_PAGES;
        numa_info[i].used_pages = 0;
        numa_info[i].base_addr = (uint64_t)i * (1ULL << 32);  // 4GB per node
        numa_info[i].end_addr = numa_info[i].base_addr + (1ULL << 32);
        
        printf("PMM: NUMA node %d initialized (%zu pages)\n", i, PMM_MAX_PAGES);
    }
    
    // Initialize statistics
    ATOMIC_STORE(&pmm_stats.total_pages, PMM_MAX_PAGES * num_nodes);
    ATOMIC_STORE(&pmm_stats.free_pages, PMM_MAX_PAGES * num_nodes);
    ATOMIC_STORE(&pmm_stats.used_pages, 0);
    ATOMIC_STORE(&pmm_stats.huge_pages, 0);
    ATOMIC_STORE(&pmm_stats.numa_local_allocs, 0);
    ATOMIC_STORE(&pmm_stats.numa_remote_allocs, 0);
    
    pmm_initialized = true;
    printf("PMM: Initialization complete\n");
    return 0;
}

void pmm_shutdown(void) {
    if (!pmm_initialized) {
        return;
    }
    
    int num_nodes = numa_num_nodes();
    for (int i = 0; i < num_nodes; i++) {
        if (numa_page_lists[i] != NULL) {
            numa_free(numa_page_lists[i], sizeof(PageDescriptor) * PMM_MAX_PAGES);
            numa_page_lists[i] = NULL;
        }
    }
    
    pmm_initialized = false;
}

// ===================================================================
// Page Allocation
// ===================================================================

NODISCARD void* pmm_alloc_page(int numa_node) {
    if (!pmm_initialized) {
        return NULL;
    }
    
    int num_nodes = numa_num_nodes();
    if (numa_node < 0 || numa_node >= num_nodes) {
        return NULL;
    }
    
    // Check if pages are available on this node
    size_t count = ATOMIC_LOAD(&numa_page_counts[numa_node]);
    if (count >= PMM_MAX_PAGES) {
        // Try other nodes
        for (int i = 0; i < num_nodes; i++) {
            if (i != numa_node) {
                count = ATOMIC_LOAD(&numa_page_counts[i]);
                if (count < PMM_MAX_PAGES) {
                    numa_node = i;
                    ATOMIC_FETCH_ADD(&pmm_stats.numa_remote_allocs, 1);
                    break;
                }
            }
        }
        
        if (count >= PMM_MAX_PAGES) {
            return NULL;  // Out of memory
        }
    } else {
        ATOMIC_FETCH_ADD(&pmm_stats.numa_local_allocs, 1);
    }
    
    // Allocate page from node
    PageDescriptor* list = numa_page_lists[numa_node];
    size_t index = ATOMIC_FETCH_ADD(&numa_page_counts[numa_node], 1);
    
    if (index >= PMM_MAX_PAGES) {
        ATOMIC_FETCH_SUB(&numa_page_counts[numa_node], 1);
        return NULL;
    }
    
    // Initialize page descriptor
    PageDescriptor* page = &list[index];
    page->physical_addr = numa_info[numa_node].base_addr + (index * PMM_PAGE_SIZE);
    page->flags = PMM_PAGE_USED;
    page->numa_node = (uint16_t)numa_node;
    page->ref_count = 1;
    
    // Update statistics
    ATOMIC_FETCH_ADD(&pmm_stats.used_pages, 1);
    ATOMIC_FETCH_SUB(&pmm_stats.free_pages, 1);
    numa_info[numa_node].used_pages++;
    numa_info[numa_node].free_pages--;
    
    return (void*)page->physical_addr;
}

NODISCARD void* pmm_alloc_pages(size_t count, int numa_node) {
    if (count == 0) {
        return NULL;
    }
    
    if (count == 1) {
        return pmm_alloc_page(numa_node);
    }
    
    // For multiple pages, allocate individual pages and track them
    // Use stack array for small counts, heap for large
    #define MAX_STACK_PAGES 64
    void* stack_pages[MAX_STACK_PAGES];
    void** pages;
    
    if (count <= MAX_STACK_PAGES) {
        pages = stack_pages;
    } else {
        pages = (void**)malloc(count * sizeof(void*));
        if (pages == NULL) {
            return NULL;
        }
    }
    
    // Allocate all pages
    for (size_t i = 0; i < count; i++) {
        pages[i] = pmm_alloc_page(numa_node);
        if (pages[i] == NULL) {
            // Allocation failed, free previously allocated pages one by one
            for (size_t j = 0; j < i; j++) {
                pmm_free_page(pages[j]);
            }
            if (count > MAX_STACK_PAGES) {
                free(pages);
            }
            return NULL;
        }
    }
    
    // All pages allocated successfully
    void* first_page = pages[0];
    if (count > MAX_STACK_PAGES) {
        free(pages);
    }
    return first_page;
}

NODISCARD void* pmm_alloc_huge_page(size_t size, int numa_node) {
    if (size == 0) {
        return NULL;
    }
    
    // Determine number of pages needed
    size_t page_count;
    if (size <= PMM_HUGE_PAGE_2MB) {
        page_count = PMM_HUGE_PAGE_2MB / PMM_PAGE_SIZE;  // 512 pages
    } else {
        page_count = PMM_HUGE_PAGE_1GB / PMM_PAGE_SIZE;  // 262144 pages
    }
    
    void* pages = pmm_alloc_pages(page_count, numa_node);
    if (pages == NULL) {
        return NULL;
    }
    
    ATOMIC_FETCH_ADD(&pmm_stats.huge_pages, 1);
    return pages;
}

// ===================================================================
// Page Deallocation
// ===================================================================

NODISCARD int pmm_free_page(void* page) {
    if (!pmm_initialized || page == NULL) {
        return -1;
    }
    
    // Find page descriptor
    PageDescriptor* desc = pmm_get_page_descriptor(page);
    if (desc == NULL) {
        return -1;
    }
    
    // Decrement reference count
    if (desc->ref_count > 0) {
        desc->ref_count--;
    }
    
    // Free page if reference count is zero
    if (desc->ref_count == 0) {
        desc->flags = PMM_PAGE_FREE;
        
        // Update statistics
        ATOMIC_FETCH_SUB(&pmm_stats.used_pages, 1);
        ATOMIC_FETCH_ADD(&pmm_stats.free_pages, 1);
        
        int node = desc->numa_node;
        numa_info[node].used_pages--;
        numa_info[node].free_pages++;
    }
    
    return 0;
}

NODISCARD int pmm_free_pages(void* pages, size_t count) {
    if (pages == NULL || count == 0) {
        return -1;
    }
    
    // Free each page
    for (size_t i = 0; i < count; i++) {
        void* page = (void*)((uintptr_t)pages + (i * PMM_PAGE_SIZE));
        if (pmm_free_page(page) != 0) {
            return -1;
        }
    }
    
    return 0;
}

NODISCARD int pmm_free_huge_page(void* page, size_t size) {
    if (page == NULL || size == 0) {
        return -1;
    }
    
    // Determine number of pages
    size_t page_count;
    if (size <= PMM_HUGE_PAGE_2MB) {
        page_count = PMM_HUGE_PAGE_2MB / PMM_PAGE_SIZE;
    } else {
        page_count = PMM_HUGE_PAGE_1GB / PMM_PAGE_SIZE;
    }
    
    int result = pmm_free_pages(page, page_count);
    if (result == 0) {
        ATOMIC_FETCH_SUB(&pmm_stats.huge_pages, 1);
    }
    
    return result;
}

// ===================================================================
// Page Information
// ===================================================================

NODISCARD PageDescriptor* pmm_get_page_descriptor(void* page) {
    if (page == NULL) {
        return NULL;
    }
    
    uint64_t addr = (uint64_t)page;
    
    // Find which NUMA node this page belongs to
    int num_nodes = numa_num_nodes();
    for (int i = 0; i < num_nodes; i++) {
        if (addr >= numa_info[i].base_addr && addr < numa_info[i].end_addr) {
            // Calculate page index
            size_t index = (addr - numa_info[i].base_addr) / PMM_PAGE_SIZE;
            if (index < PMM_MAX_PAGES) {
                return &numa_page_lists[i][index];
            }
        }
    }
    
    return NULL;
}

NODISCARD bool pmm_is_page_free(void* page) {
    PageDescriptor* desc = pmm_get_page_descriptor(page);
    if (desc == NULL) {
        return false;
    }
    
    return (desc->flags & PMM_PAGE_FREE) != 0;
}

NODISCARD int pmm_get_page_numa_node(void* page) {
    PageDescriptor* desc = pmm_get_page_descriptor(page);
    if (desc == NULL) {
        return -1;
    }
    
    return desc->numa_node;
}

// ===================================================================
// NUMA Node Information
// ===================================================================

NODISCARD NumaNodeInfo pmm_get_numa_info(int node) {
    if (node < 0 || node >= numa_num_nodes()) {
        NumaNodeInfo empty = {0};
        return empty;
    }
    
    return numa_info[node];
}

NODISCARD int pmm_get_best_numa_node(void) {
    int num_nodes = numa_num_nodes();
    int best_node = 0;
    size_t max_free = 0;
    
    // Find node with most free pages
    for (int i = 0; i < num_nodes; i++) {
        if (numa_info[i].free_pages > max_free) {
            max_free = numa_info[i].free_pages;
            best_node = i;
        }
    }
    
    return best_node;
}

// ===================================================================
// Statistics
// ===================================================================

NODISCARD PmmStats pmm_get_stats(void) {
    PmmStats stats;
    stats.total_pages = ATOMIC_LOAD(&pmm_stats.total_pages);
    stats.free_pages = ATOMIC_LOAD(&pmm_stats.free_pages);
    stats.used_pages = ATOMIC_LOAD(&pmm_stats.used_pages);
    stats.huge_pages = ATOMIC_LOAD(&pmm_stats.huge_pages);
    stats.numa_local_allocs = ATOMIC_LOAD(&pmm_stats.numa_local_allocs);
    stats.numa_remote_allocs = ATOMIC_LOAD(&pmm_stats.numa_remote_allocs);
    return stats;
}

void pmm_print_stats(void) {
    PmmStats stats = pmm_get_stats();
    
    printf("\n=== PMM Statistics (Phase 2) ===\n");
    printf("Total Pages:         %zu\n", stats.total_pages);
    printf("Free Pages:          %zu\n", stats.free_pages);
    printf("Used Pages:          %zu\n", stats.used_pages);
    printf("Huge Pages:          %zu\n", stats.huge_pages);
    printf("NUMA Local Allocs:   %zu\n", stats.numa_local_allocs);
    printf("NUMA Remote Allocs:  %zu\n", stats.numa_remote_allocs);
    
    // Print per-NUMA-node statistics
    int num_nodes = numa_num_nodes();
    printf("\n=== Per-NUMA-Node Statistics ===\n");
    for (int i = 0; i < num_nodes; i++) {
        NumaNodeInfo info = pmm_get_numa_info(i);
        printf("NUMA Node %d: %zu/%zu pages free\n",
               i, info.free_pages, info.total_pages);
    }
    printf("================================\n\n");
}
