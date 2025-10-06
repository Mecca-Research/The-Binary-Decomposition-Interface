
/**
 * @file vmm.c
 * @brief Virtual Memory Manager Implementation for BDI Kernel
 * 
 * @author BDI Kernel Team
 * @date 2024
 * @standard C23
 */

#include "vmm.h"
#include "pmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Global State
// ============================================================================

static reclaim_state_t g_reclaim_state = {0};
static tlb_cache_t g_tlb_cache = {0};
static bool g_vmm_initialized = false;

// Statistics
static atomic_uint_fast64_t g_total_page_faults = 0;
static atomic_uint_fast64_t g_major_faults = 0;
static atomic_uint_fast64_t g_minor_faults = 0;

// ============================================================================
// Internal Helper Functions
// ============================================================================

/**
 * @brief Get page table index for level
 */
static inline uint32_t get_pt_index(uint64_t vaddr, uint32_t level) {
    return (vaddr >> (12 + 9 * level)) & 0x1FF;
}

/**
 * @brief Check if address is canonical
 */
static inline bool is_canonical(uint64_t addr) {
    return (addr <= VMM_USER_SPACE_END) || (addr >= VMM_KERNEL_SPACE_START);
}

/**
 * @brief Convert protection flags to PTE flags
 */
static inline uint64_t prot_to_pte_flags(uint32_t prot) {
    uint64_t flags = PTE_PRESENT;
    
    if (prot & PROT_WRITE) {
        flags |= PTE_WRITABLE;
    }
    
    if (!(prot & PROT_EXEC)) {
        flags |= PTE_NX;
    }
    
    return flags;
}

// ============================================================================
// TLB Implementation
// ============================================================================

int vmm_tlb_init(void) {
    memset(&g_tlb_cache, 0, sizeof(tlb_cache_t));
    atomic_store(&g_tlb_cache.next_victim, 0);
    
    printf("VMM: TLB initialized with 64 entries\n");
    return 0;
}

bool vmm_tlb_lookup(uint64_t vaddr, uint64_t *paddr) {
    uint64_t vpfn = vaddr >> PAGE_SHIFT;
    
    for (uint32_t i = 0; i < 64; i++) {
        tlb_entry_t *entry = &g_tlb_cache.entries[i];
        
        if (entry->valid && (entry->vaddr >> PAGE_SHIFT) == vpfn) {
            *paddr = entry->paddr | (vaddr & (PAGE_SIZE - 1));
            atomic_fetch_add(&g_tlb_cache.hits, 1);
            return true;
        }
    }
    
    atomic_fetch_add(&g_tlb_cache.misses, 1);
    return false;
}

void vmm_tlb_insert(uint64_t vaddr, uint64_t paddr, uint64_t flags) {
    uint32_t victim = atomic_fetch_add(&g_tlb_cache.next_victim, 1) % 64;
    
    tlb_entry_t *entry = &g_tlb_cache.entries[victim];
    entry->vaddr = vaddr & ~(PAGE_SIZE - 1);
    entry->paddr = paddr & ~(PAGE_SIZE - 1);
    entry->flags = flags;
    entry->valid = true;
}

void vmm_tlb_flush_entry(uint64_t vaddr) {
    uint64_t vpfn = vaddr >> PAGE_SHIFT;
    
    for (uint32_t i = 0; i < 64; i++) {
        tlb_entry_t *entry = &g_tlb_cache.entries[i];
        
        if (entry->valid && (entry->vaddr >> PAGE_SHIFT) == vpfn) {
            entry->valid = false;
            break;
        }
    }
    
    atomic_fetch_add(&g_tlb_cache.flushes, 1);
}

void vmm_tlb_flush_all(void) {
    for (uint32_t i = 0; i < 64; i++) {
        g_tlb_cache.entries[i].valid = false;
    }
    
    atomic_fetch_add(&g_tlb_cache.flushes, 1);
}

void vmm_tlb_get_stats(uint64_t *hits, uint64_t *misses) {
    if (hits != nullptr) {
        *hits = atomic_load(&g_tlb_cache.hits);
    }
    if (misses != nullptr) {
        *misses = atomic_load(&g_tlb_cache.misses);
    }
}

// ============================================================================
// Page Table Management
// ============================================================================

page_table_t* vmm_alloc_page_table(void) {
    page_table_t *pt = calloc(1, sizeof(page_table_t));
    if (pt == nullptr) {
        return nullptr;
    }
    
    atomic_store(&pt->refcount, 1);
    return pt;
}

void vmm_free_page_table(page_table_t *pt) {
    if (pt == nullptr) {
        return;
    }
    
    if (atomic_fetch_sub(&pt->refcount, 1) > 1) {
        return;
    }
    
    free(pt);
}

pte_t* vmm_walk_page_tables(mm_struct_t *mm, uint64_t vaddr, bool create) {
    if (mm == nullptr || mm->pgd == nullptr) {
        return nullptr;
    }
    
    page_table_t *current = mm->pgd;
    
    // Walk through page table levels (4-level paging)
    for (int level = 3; level >= 0; level--) {
        uint32_t index = get_pt_index(vaddr, level);
        pte_t *pte = &current->entries[index];
        
        if (level == 0) {
            return pte;  // Reached final level
        }
        
        // Check if next level exists
        if (!(pte->value & PTE_PRESENT)) {
            if (!create) {
                return nullptr;
            }
            
            // Allocate next level
            page_table_t *next = vmm_alloc_page_table();
            if (next == nullptr) {
                return nullptr;
            }
            
            pte->value = ((uint64_t)next & ~0xFFF) | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
        }
        
        current = (page_table_t*)(pte->value & ~0xFFF);
    }
    
    return nullptr;
}

int vmm_virt_to_phys(mm_struct_t *mm, uint64_t vaddr, uint64_t *paddr) {
    if (mm == nullptr || paddr == nullptr) {
        return -1;
    }
    
    // Try TLB first
    if (vmm_tlb_lookup(vaddr, paddr)) {
        return 0;
    }
    
    // Walk page tables
    pte_t *pte = vmm_walk_page_tables(mm, vaddr, false);
    if (pte == nullptr || !(pte->value & PTE_PRESENT)) {
        return -1;
    }
    
    *paddr = (pte->value & ~0xFFF) | (vaddr & 0xFFF);
    
    // Update TLB
    vmm_tlb_insert(vaddr, *paddr, pte->value);
    
    return 0;
}

// ============================================================================
// Memory Reclamation Implementation
// ============================================================================

int vmm_reclaim_init(void) {
    memset(&g_reclaim_state, 0, sizeof(reclaim_state_t));
    
    for (uint32_t i = 0; i < NR_LRU_LISTS; i++) {
        g_reclaim_state.lists[i].head = nullptr;
        g_reclaim_state.lists[i].tail = nullptr;
        atomic_store(&g_reclaim_state.lists[i].count, 0);
        atomic_flag_clear(&g_reclaim_state.lists[i].lock);
    }
    
    // Set thresholds (example values)
    g_reclaim_state.min_free_pages = 1024;
    g_reclaim_state.low_free_pages = 2048;
    g_reclaim_state.high_free_pages = 4096;
    
    printf("VMM: Memory reclamation initialized\n");
    return 0;
}

void vmm_lru_add(page_frame_t *page, lru_list_t lru) {
    if (page == nullptr || lru >= NR_LRU_LISTS) {
        return;
    }
    
    lru_list_head_t *list = &g_reclaim_state.lists[lru];
    
    // Lock list
    while (atomic_flag_test_and_set(&list->lock)) {
        // Spin wait
    }
    
    // Add to tail
    page->next = nullptr;
    page->prev = list->tail;
    
    if (list->tail != nullptr) {
        list->tail->next = page;
    } else {
        list->head = page;
    }
    
    list->tail = page;
    atomic_fetch_add(&list->count, 1);
    atomic_fetch_or(&page->flags, PG_LRU);
    
    atomic_flag_clear(&list->lock);
}

void vmm_lru_remove(page_frame_t *page) {
    if (page == nullptr || !(atomic_load(&page->flags) & PG_LRU)) {
        return;
    }
    
    // Find which list the page is in
    for (uint32_t i = 0; i < NR_LRU_LISTS; i++) {
        lru_list_head_t *list = &g_reclaim_state.lists[i];
        
        while (atomic_flag_test_and_set(&list->lock)) {
            // Spin wait
        }
        
        // Check if page is in this list (simplified)
        if (page->prev != nullptr) {
            page->prev->next = page->next;
        } else if (list->head == page) {
            list->head = page->next;
        }
        
        if (page->next != nullptr) {
            page->next->prev = page->prev;
        } else if (list->tail == page) {
            list->tail = page->prev;
        }
        
        atomic_fetch_sub(&list->count, 1);
        atomic_flag_clear(&list->lock);
    }
    
    atomic_fetch_and(&page->flags, ~PG_LRU);
    page->next = nullptr;
    page->prev = nullptr;
}

void vmm_activate_page(page_frame_t *page) {
    if (page == nullptr) {
        return;
    }
    
    vmm_lru_remove(page);
    
    // Determine if anonymous or file-backed
    bool is_anon = true;  // Simplified
    
    if (is_anon) {
        vmm_lru_add(page, LRU_ACTIVE_ANON);
    } else {
        vmm_lru_add(page, LRU_ACTIVE_FILE);
    }
    
    atomic_fetch_or(&page->flags, PG_ACTIVE);
    atomic_fetch_add(&g_reclaim_state.pages_activated, 1);
}

void vmm_deactivate_page(page_frame_t *page) {
    if (page == nullptr) {
        return;
    }
    
    vmm_lru_remove(page);
    
    bool is_anon = true;  // Simplified
    
    if (is_anon) {
        vmm_lru_add(page, LRU_INACTIVE_ANON);
    } else {
        vmm_lru_add(page, LRU_INACTIVE_FILE);
    }
    
    atomic_fetch_and(&page->flags, ~PG_ACTIVE);
    atomic_fetch_add(&g_reclaim_state.pages_deactivated, 1);
}

uint64_t vmm_reclaim_pages(uint64_t nr_pages) {
    uint64_t reclaimed = 0;
    
    printf("VMM: Attempting to reclaim %llu pages\n", nr_pages);
    
    // Try inactive lists first
    lru_list_t lists[] = {LRU_INACTIVE_FILE, LRU_INACTIVE_ANON};
    
    for (uint32_t i = 0; i < 2 && reclaimed < nr_pages; i++) {
        lru_list_head_t *list = &g_reclaim_state.lists[lists[i]];
        
        while (atomic_flag_test_and_set(&list->lock)) {
            // Spin wait
        }
        
        page_frame_t *page = list->head;
        while (page != nullptr && reclaimed < nr_pages) {
            page_frame_t *next = page->next;
            
            // Check if page can be reclaimed
            if (atomic_load(&page->refcount) == 0) {
                vmm_lru_remove(page);
                // In real implementation, would write back dirty pages
                reclaimed++;
                atomic_fetch_add(&g_reclaim_state.pages_reclaimed, 1);
            }
            
            page = next;
            atomic_fetch_add(&g_reclaim_state.pages_scanned, 1);
        }
        
        atomic_flag_clear(&list->lock);
    }
    
    printf("VMM: Reclaimed %llu pages\n", reclaimed);
    return reclaimed;
}

uint64_t vmm_shrink_caches(void) {
    // Simplified cache shrinking
    uint64_t freed = vmm_reclaim_pages(1024);
    
    printf("VMM: Shrunk caches, freed %llu pages\n", freed);
    return freed;
}

// ============================================================================
// Address Space Management
// ============================================================================

int vmm_init(void) {
    if (g_vmm_initialized) {
        return 0;
    }
    
    printf("VMM: Initializing virtual memory manager...\n");
    
    if (vmm_tlb_init() != 0) {
        return -1;
    }
    
    if (vmm_reclaim_init() != 0) {
        return -1;
    }
    
    g_vmm_initialized = true;
    
    printf("VMM: Initialization complete\n");
    return 0;
}

void vmm_shutdown(void) {
    if (!g_vmm_initialized) {
        return;
    }
    
    printf("VMM: Shutting down...\n");
    
    g_vmm_initialized = false;
    
    printf("VMM: Shutdown complete\n");
}

mm_struct_t* vmm_create_address_space(void) {
    mm_struct_t *mm = calloc(1, sizeof(mm_struct_t));
    if (mm == nullptr) {
        return nullptr;
    }
    
    mm->pgd = vmm_alloc_page_table();
    if (mm->pgd == nullptr) {
        free(mm);
        return nullptr;
    }
    
    mm->vma_list = nullptr;
    mm->total_vm = 0;
    mm->locked_vm = 0;
    atomic_store(&mm->mm_users, 1);
    atomic_flag_clear(&mm->lock);
    
    printf("VMM: Created new address space\n");
    return mm;
}

void vmm_destroy_address_space(mm_struct_t *mm) {
    if (mm == nullptr) {
        return;
    }
    
    if (atomic_fetch_sub(&mm->mm_users, 1) > 1) {
        return;
    }
    
    // Free VMAs
    vm_area_t *vma = mm->vma_list;
    while (vma != nullptr) {
        vm_area_t *next = vma->next;
        free(vma);
        vma = next;
    }
    
    // Free page tables
    vmm_free_page_table(mm->pgd);
    
    free(mm);
    
    printf("VMM: Destroyed address space\n");
}

int vmm_map_page(mm_struct_t *mm, uint64_t vaddr, page_frame_t *page, uint32_t prot) {
    if (mm == nullptr || page == nullptr || !is_canonical(vaddr)) {
        return -1;
    }
    
    pte_t *pte = vmm_walk_page_tables(mm, vaddr, true);
    if (pte == nullptr) {
        return -1;
    }
    
    uint64_t pfn = pmm_page_to_pfn(page);
    uint64_t flags = prot_to_pte_flags(prot);
    
    pte->value = (pfn << PAGE_SHIFT) | flags;
    
    // Invalidate TLB
    vmm_tlb_flush_entry(vaddr);
    
    return 0;
}

int vmm_unmap_page(mm_struct_t *mm, uint64_t vaddr) {
    if (mm == nullptr || !is_canonical(vaddr)) {
        return -1;
    }
    
    pte_t *pte = vmm_walk_page_tables(mm, vaddr, false);
    if (pte == nullptr) {
        return -1;
    }
    
    pte->value = 0;
    
    vmm_tlb_flush_entry(vaddr);
    
    return 0;
}

uint64_t vmm_mmap(mm_struct_t *mm, uint64_t addr, size_t length, 
                  uint32_t prot, uint32_t flags) {
    if (mm == nullptr || length == 0) {
        return 0;
    }
    
    // Align length to page boundary
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Find suitable address if not fixed
    if (addr == 0 || !(flags & MAP_FIXED)) {
        addr = 0x10000000;  // Simplified address selection
    }
    
    // Create VMA
    vm_area_t *vma = calloc(1, sizeof(vm_area_t));
    if (vma == nullptr) {
        return 0;
    }
    
    vma->start = addr;
    vma->end = addr + length;
    vma->flags = flags;
    vma->prot = prot;
    vma->next = mm->vma_list;
    mm->vma_list = vma;
    
    mm->total_vm += length;
    
    // Populate pages if requested
    if (flags & MAP_POPULATE) {
        for (uint64_t va = addr; va < addr + length; va += PAGE_SIZE) {
            page_frame_t *page = pmm_alloc_page(GFP_KERNEL);
            if (page != nullptr) {
                vmm_map_page(mm, va, page, prot);
            }
        }
    }
    
    printf("VMM: Mapped region 0x%llx-0x%llx (%zu bytes)\n", 
           addr, addr + length, length);
    
    return addr;
}

int vmm_munmap(mm_struct_t *mm, uint64_t addr, size_t length) {
    if (mm == nullptr || length == 0) {
        return -1;
    }
    
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Find and remove VMA
    vm_area_t **prev = &mm->vma_list;
    vm_area_t *vma = mm->vma_list;
    
    while (vma != nullptr) {
        if (vma->start == addr && vma->end == addr + length) {
            *prev = vma->next;
            
            // Unmap pages
            for (uint64_t va = addr; va < addr + length; va += PAGE_SIZE) {
                vmm_unmap_page(mm, va);
            }
            
            mm->total_vm -= length;
            free(vma);
            
            printf("VMM: Unmapped region 0x%llx-0x%llx\n", addr, addr + length);
            return 0;
        }
        
        prev = &vma->next;
        vma = vma->next;
    }
    
    return -1;
}

int vmm_mprotect(mm_struct_t *mm, uint64_t addr, size_t length, uint32_t prot) {
    if (mm == nullptr || length == 0) {
        return -1;
    }
    
    length = (length + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    // Update page protections
    for (uint64_t va = addr; va < addr + length; va += PAGE_SIZE) {
        pte_t *pte = vmm_walk_page_tables(mm, va, false);
        if (pte != nullptr && (pte->value & PTE_PRESENT)) {
            uint64_t pfn = pte->value >> PAGE_SHIFT;
            pte->value = (pfn << PAGE_SHIFT) | prot_to_pte_flags(prot);
            vmm_tlb_flush_entry(va);
        }
    }
    
    printf("VMM: Changed protection for 0x%llx-0x%llx\n", addr, addr + length);
    return 0;
}

// ============================================================================
// Page Fault Handling
// ============================================================================

int vmm_handle_page_fault(mm_struct_t *mm, const fault_info_t *fault) {
    if (mm == nullptr || fault == nullptr) {
        return -1;
    }
    
    atomic_fetch_add(&g_total_page_faults, 1);
    atomic_fetch_add(&mm->page_faults, 1);
    
    printf("VMM: Page fault at 0x%llx (write=%d, user=%d)\n",
           fault->address, fault->write, fault->user);
    
    // Check if address is in valid VMA
    vm_area_t *vma = mm->vma_list;
    while (vma != nullptr) {
        if (fault->address >= vma->start && fault->address < vma->end) {
            break;
        }
        vma = vma->next;
    }
    
    if (vma == nullptr) {
        printf("VMM: Invalid address - no VMA found\n");
        return -1;
    }
    
    atomic_fetch_add(&vma->fault_count, 1);
    
    if (!fault->present) {
        // Page not present - demand paging
        atomic_fetch_add(&g_major_faults, 1);
        atomic_fetch_add(&mm->major_faults, 1);
        return vmm_handle_demand_page(mm, fault->address);
    } else if (fault->write) {
        // Write to read-only page - COW
        atomic_fetch_add(&g_minor_faults, 1);
        atomic_fetch_add(&mm->minor_faults, 1);
        return vmm_handle_cow(mm, fault->address);
    }
    
    return 0;
}

int vmm_handle_demand_page(mm_struct_t *mm, uint64_t vaddr) {
    printf("VMM: Handling demand page for 0x%llx\n", vaddr);
    
    // Allocate physical page
    page_frame_t *page = pmm_alloc_page(GFP_KERNEL | GFP_ZERO);
    if (page == nullptr) {
        printf("VMM: Failed to allocate page\n");
        return -1;
    }
    
    // Map page
    if (vmm_map_page(mm, vaddr & ~(PAGE_SIZE - 1), page, PROT_READ | PROT_WRITE) != 0) {
        pmm_free_page(page);
        return -1;
    }
    
    // Add to LRU
    vmm_lru_add(page, LRU_ACTIVE_ANON);
    
    printf("VMM: Demand page allocated and mapped\n");
    return 0;
}

int vmm_handle_cow(mm_struct_t *mm, uint64_t vaddr) {
    printf("VMM: Handling copy-on-write for 0x%llx\n", vaddr);
    
    // Get current page
    pte_t *pte = vmm_walk_page_tables(mm, vaddr, false);
    if (pte == nullptr) {
        return -1;
    }
    
    uint64_t old_pfn = pte->value >> PAGE_SHIFT;
    page_frame_t *old_page = pmm_pfn_to_page(old_pfn);
    
    // Allocate new page
    page_frame_t *new_page = pmm_alloc_page(GFP_KERNEL);
    if (new_page == nullptr) {
        return -1;
    }
    
    // Copy data
    if (old_page != nullptr && old_page->virtual_addr != nullptr && 
        new_page->virtual_addr != nullptr) {
        memcpy(new_page->virtual_addr, old_page->virtual_addr, PAGE_SIZE);
    }
    
    // Update mapping
    uint64_t new_pfn = pmm_page_to_pfn(new_page);
    pte->value = (new_pfn << PAGE_SHIFT) | PTE_PRESENT | PTE_WRITABLE | PTE_USER;
    
    vmm_tlb_flush_entry(vaddr);
    
    printf("VMM: Copy-on-write completed\n");
    return 0;
}

// ============================================================================
// Diagnostics
// ============================================================================

void vmm_print_stats(void) {
    printf("\n=== VMM Statistics ===\n");
    printf("Page Faults:\n");
    printf("  Total:  %llu\n", atomic_load(&g_total_page_faults));
    printf("  Major:  %llu\n", atomic_load(&g_major_faults));
    printf("  Minor:  %llu\n", atomic_load(&g_minor_faults));
    
    uint64_t tlb_hits, tlb_misses;
    vmm_tlb_get_stats(&tlb_hits, &tlb_misses);
    printf("\nTLB:\n");
    printf("  Hits:   %llu\n", tlb_hits);
    printf("  Misses: %llu\n", tlb_misses);
    if (tlb_hits + tlb_misses > 0) {
        printf("  Hit Rate: %.2f%%\n", 
               (double)tlb_hits * 100.0 / (tlb_hits + tlb_misses));
    }
    
    printf("\nReclamation:\n");
    printf("  Scanned:   %llu\n", atomic_load(&g_reclaim_state.pages_scanned));
    printf("  Reclaimed: %llu\n", atomic_load(&g_reclaim_state.pages_reclaimed));
    printf("  Activated: %llu\n", atomic_load(&g_reclaim_state.pages_activated));
    
    printf("\nLRU Lists:\n");
    const char *lru_names[] = {
        "Inactive Anon", "Active Anon", 
        "Inactive File", "Active File", "Unevictable"
    };
    for (uint32_t i = 0; i < NR_LRU_LISTS; i++) {
        printf("  %s: %llu pages\n", 
               lru_names[i], 
               atomic_load(&g_reclaim_state.lists[i].count));
    }
    printf("======================\n\n");
}

void vmm_dump_address_space(mm_struct_t *mm) {
    if (mm == nullptr) {
        return;
    }
    
    printf("\n=== Address Space Dump ===\n");
    printf("Total VM: %llu bytes\n", mm->total_vm);
    printf("Locked VM: %llu bytes\n", mm->locked_vm);
    printf("Users: %d\n", atomic_load(&mm->mm_users));
    
    printf("\nVMAs:\n");
    vm_area_t *vma = mm->vma_list;
    while (vma != nullptr) {
        printf("  0x%016llx-0x%016llx ", vma->start, vma->end);
        printf("prot=%c%c%c ",
               (vma->prot & PROT_READ) ? 'r' : '-',
               (vma->prot & PROT_WRITE) ? 'w' : '-',
               (vma->prot & PROT_EXEC) ? 'x' : '-');
        printf("faults=%llu\n", atomic_load(&vma->fault_count));
        vma = vma->next;
    }
    printf("==========================\n\n");
}

bool vmm_validate(void) {
    if (!g_vmm_initialized) {
        return false;
    }
    
    printf("VMM: Validation passed\n");
    return true;
}
