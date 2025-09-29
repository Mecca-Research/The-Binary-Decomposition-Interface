
/**
 * @file x86_paging_mmu.c
 * @brief x86 Paging and MMU Management Implementation
 * 
 * Implementation of comprehensive x86 memory management unit support
 * providing virtual memory management, address translation, and page fault handling.
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#include "x86_paging_mmu.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// =============================================================================
// GLOBAL STATE
// =============================================================================

static bool g_x86_paging_initialized = false;
static x86_mmu_context_t g_mmu_context = {0};

// =============================================================================
// PRIVATE FUNCTION DECLARATIONS
// =============================================================================

static uint32_t x86_get_physical_address(void *virtual_addr);
static void *x86_allocate_physical_page(void);
static void x86_free_physical_page(void *page);
static int x86_ensure_page_table(x86_page_directory_t *page_dir, uint32_t virtual_addr);

// =============================================================================
// PUBLIC FUNCTION IMPLEMENTATIONS
// =============================================================================

int x86_paging_mmu_initialize(void)
{
    if (g_x86_paging_initialized) {
        return 0; // Already initialized
    }
    
    // Initialize MMU context
    memset(&g_mmu_context, 0, sizeof(x86_mmu_context_t));
    
    // Allocate page table pointer array
    g_mmu_context.page_tables = calloc(X86_PDE_COUNT, sizeof(x86_page_table_t *));
    if (g_mmu_context.page_tables == NULL) {
        return -1;
    }
    
    // Allocate physical address array for page tables
    g_mmu_context.page_table_phys = calloc(X86_PDE_COUNT, sizeof(uint32_t));
    if (g_mmu_context.page_table_phys == NULL) {
        free(g_mmu_context.page_tables);
        return -1;
    }
    
    g_mmu_context.allocated_tables = 0;
    g_mmu_context.paging_enabled = false;
    
    g_x86_paging_initialized = true;
    return 0;
}

int x86_paging_mmu_shutdown(void)
{
    if (!g_x86_paging_initialized) {
        return -1; // Not initialized
    }
    
    // Disable paging if enabled
    if (g_mmu_context.paging_enabled) {
        x86_disable_paging();
    }
    
    // Free all page tables
    if (g_mmu_context.page_tables != NULL) {
        for (size_t i = 0; i < X86_PDE_COUNT; i++) {
            if (g_mmu_context.page_tables[i] != NULL) {
                x86_destroy_page_table(g_mmu_context.page_tables[i]);
            }
        }
        free(g_mmu_context.page_tables);
    }
    
    // Free physical address array
    if (g_mmu_context.page_table_phys != NULL) {
        free(g_mmu_context.page_table_phys);
    }
    
    // Free page directory
    if (g_mmu_context.page_directory != NULL) {
        x86_destroy_page_directory(g_mmu_context.page_directory);
    }
    
    // Reset context
    memset(&g_mmu_context, 0, sizeof(x86_mmu_context_t));
    
    g_x86_paging_initialized = false;
    return 0;
}

x86_page_directory_t *x86_create_page_directory(void)
{
    if (!g_x86_paging_initialized) {
        return NULL;
    }
    
    // Allocate page directory (must be page-aligned)
    x86_page_directory_t *page_dir = aligned_alloc(X86_PAGE_SIZE, sizeof(x86_page_directory_t));
    if (page_dir == NULL) {
        return NULL;
    }
    
    // Initialize all entries to not present
    memset(page_dir, 0, sizeof(x86_page_directory_t));
    
    return page_dir;
}

void x86_destroy_page_directory(x86_page_directory_t *page_dir)
{
    if (page_dir == NULL) {
        return;
    }
    
    // Free all associated page tables
    for (int i = 0; i < X86_PDE_COUNT; i++) {
        if (page_dir->entries[i].fields.present) {
            // Get page table physical address
            uint32_t pt_phys = page_dir->entries[i].fields.page_table_addr << 12;
            
            // Find corresponding page table in our tracking arrays
            for (size_t j = 0; j < X86_PDE_COUNT; j++) {
                if (g_mmu_context.page_table_phys[j] == pt_phys) {
                    if (g_mmu_context.page_tables[j] != NULL) {
                        x86_destroy_page_table(g_mmu_context.page_tables[j]);
                        g_mmu_context.page_tables[j] = NULL;
                        g_mmu_context.page_table_phys[j] = 0;
                        g_mmu_context.allocated_tables--;
                    }
                    break;
                }
            }
        }
    }
    
    free(page_dir);
}

x86_page_table_t *x86_create_page_table(void)
{
    if (!g_x86_paging_initialized) {
        return NULL;
    }
    
    // Allocate page table (must be page-aligned)
    x86_page_table_t *page_table = aligned_alloc(X86_PAGE_SIZE, sizeof(x86_page_table_t));
    if (page_table == NULL) {
        return NULL;
    }
    
    // Initialize all entries to not present
    memset(page_table, 0, sizeof(x86_page_table_t));
    
    return page_table;
}

void x86_destroy_page_table(x86_page_table_t *page_table)
{
    if (page_table != NULL) {
        free(page_table);
    }
}

int x86_map_page(
    x86_page_directory_t *page_dir,
    uint32_t virtual_addr,
    uint32_t physical_addr,
    uint32_t flags)
{
    if (!g_x86_paging_initialized || page_dir == NULL) {
        return -1;
    }
    
    // Ensure addresses are page-aligned
    virtual_addr = x86_page_align(virtual_addr);
    physical_addr = x86_page_align(physical_addr);
    
    // Ensure page table exists for this virtual address
    if (x86_ensure_page_table(page_dir, virtual_addr) != 0) {
        return -1;
    }
    
    // Get page table entry
    x86_pte_t *pte = x86_get_pte(page_dir, virtual_addr);
    if (pte == NULL) {
        return -1;
    }
    
    // Set up page table entry
    pte->raw = 0;
    pte->fields.present = (flags & X86_PAGE_PRESENT) ? 1 : 0;
    pte->fields.writable = (flags & X86_PAGE_WRITABLE) ? 1 : 0;
    pte->fields.user = (flags & X86_PAGE_USER) ? 1 : 0;
    pte->fields.write_through = (flags & X86_PAGE_WRITE_THROUGH) ? 1 : 0;
    pte->fields.cache_disable = (flags & X86_PAGE_CACHE_DISABLE) ? 1 : 0;
    pte->fields.global = (flags & X86_PAGE_GLOBAL) ? 1 : 0;
    pte->fields.page_addr = physical_addr >> 12;
    
    // Flush TLB for this page
    x86_flush_tlb_page(virtual_addr);
    
    return 0;
}

int x86_unmap_page(x86_page_directory_t *page_dir, uint32_t virtual_addr)
{
    if (!g_x86_paging_initialized || page_dir == NULL) {
        return -1;
    }
    
    // Get page table entry
    x86_pte_t *pte = x86_get_pte(page_dir, virtual_addr);
    if (pte == NULL || !pte->fields.present) {
        return -1; // Page not mapped
    }
    
    // Clear page table entry
    pte->raw = 0;
    
    // Flush TLB for this page
    x86_flush_tlb_page(virtual_addr);
    
    return 0;
}

int x86_map_memory_region(x86_page_directory_t *page_dir, const x86_memory_mapping_t *mapping)
{
    if (!g_x86_paging_initialized || page_dir == NULL || mapping == NULL) {
        return -1;
    }
    
    uint32_t virtual_addr = x86_page_align(mapping->virtual_addr);
    uint32_t physical_addr = x86_page_align(mapping->physical_addr);
    size_t size = mapping->size;
    uint32_t flags = mapping->flags;
    
    // Convert protection to page flags
    flags |= x86_protection_to_page_flags(mapping->protection);
    
    // Map each page in the region
    size_t pages = (size + X86_PAGE_SIZE - 1) / X86_PAGE_SIZE;
    for (size_t i = 0; i < pages; i++) {
        if (x86_map_page(page_dir, virtual_addr, physical_addr, flags) != 0) {
            // Rollback on error
            for (size_t j = 0; j < i; j++) {
                x86_unmap_page(page_dir, mapping->virtual_addr + (j * X86_PAGE_SIZE));
            }
            return -1;
        }
        
        virtual_addr += X86_PAGE_SIZE;
        physical_addr += X86_PAGE_SIZE;
    }
    
    return 0;
}

int x86_unmap_memory_region(x86_page_directory_t *page_dir, uint32_t virtual_addr, size_t size)
{
    if (!g_x86_paging_initialized || page_dir == NULL) {
        return -1;
    }
    
    virtual_addr = x86_page_align(virtual_addr);
    size_t pages = (size + X86_PAGE_SIZE - 1) / X86_PAGE_SIZE;
    
    // Unmap each page in the region
    for (size_t i = 0; i < pages; i++) {
        x86_unmap_page(page_dir, virtual_addr + (i * X86_PAGE_SIZE));
    }
    
    return 0;
}

int x86_translate_address(
    x86_page_directory_t *page_dir,
    uint32_t virtual_addr,
    uint32_t *physical_addr)
{
    if (!g_x86_paging_initialized || page_dir == NULL || physical_addr == NULL) {
        return -1;
    }
    
    // Get page table entry
    x86_pte_t *pte = x86_get_pte(page_dir, virtual_addr);
    if (pte == NULL || !pte->fields.present) {
        return -1; // Page not present
    }
    
    // Calculate physical address
    uint32_t page_base = pte->fields.page_addr << 12;
    uint32_t offset = x86_get_page_offset(virtual_addr);
    *physical_addr = page_base + offset;
    
    return 0;
}

int x86_enable_paging(x86_page_directory_t *page_dir)
{
    if (!g_x86_paging_initialized || page_dir == NULL) {
        return -1;
    }
    
    // Get physical address of page directory
    uint32_t page_dir_phys = x86_get_physical_address(page_dir);
    if (page_dir_phys == 0) {
        return -1;
    }
    
    // Update MMU context
    g_mmu_context.page_directory = page_dir;
    g_mmu_context.page_directory_phys = page_dir_phys;
    g_mmu_context.cr3_value = page_dir_phys;
    
    // Enable paging (would use inline assembly in real implementation)
    // This is a placeholder for the actual CR0 and CR3 manipulation
    g_mmu_context.paging_enabled = true;
    
    return 0;
}

int x86_disable_paging(void)
{
    if (!g_x86_paging_initialized) {
        return -1;
    }
    
    // Disable paging (would use inline assembly in real implementation)
    // This is a placeholder for the actual CR0 manipulation
    g_mmu_context.paging_enabled = false;
    
    return 0;
}

int x86_switch_page_directory(x86_page_directory_t *page_dir)
{
    if (!g_x86_paging_initialized || page_dir == NULL) {
        return -1;
    }
    
    // Get physical address of page directory
    uint32_t page_dir_phys = x86_get_physical_address(page_dir);
    if (page_dir_phys == 0) {
        return -1;
    }
    
    // Update MMU context
    g_mmu_context.page_directory = page_dir;
    g_mmu_context.page_directory_phys = page_dir_phys;
    g_mmu_context.cr3_value = page_dir_phys;
    
    // Load CR3 register (would use inline assembly in real implementation)
    // This is a placeholder for the actual CR3 load
    
    // Flush TLB
    x86_flush_tlb_all();
    
    return 0;
}

x86_page_directory_t *x86_get_current_page_directory(void)
{
    if (!g_x86_paging_initialized) {
        return NULL;
    }
    
    return g_mmu_context.page_directory;
}

void x86_flush_tlb_page(uint32_t virtual_addr)
{
    // In real implementation, would use inline assembly:
    // __asm__ volatile ("invlpg (%0)" : : "r" (virtual_addr) : "memory");
    (void)virtual_addr; // Placeholder
}

void x86_flush_tlb_all(void)
{
    // In real implementation, would reload CR3:
    // __asm__ volatile ("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "eax", "memory");
}

int x86_handle_page_fault(const x86_page_fault_info_t *fault_info)
{
    if (!g_x86_paging_initialized || fault_info == NULL) {
        return -1;
    }
    
    // Basic page fault handling - in a real implementation this would be much more complex
    printf("Page fault at address 0x%08X, error code: 0x%08X\n",
           fault_info->fault_address, fault_info->error_code);
    
    if (!fault_info->present) {
        printf("  - Page not present\n");
    }
    if (fault_info->write_fault) {
        printf("  - Write fault\n");
    }
    if (fault_info->user_fault) {
        printf("  - User mode fault\n");
    }
    
    // Return error - real implementation would attempt to handle the fault
    return -1;
}

x86_pde_t *x86_get_pde(x86_page_directory_t *page_dir, uint32_t virtual_addr)
{
    if (page_dir == NULL) {
        return NULL;
    }
    
    uint32_t pd_index = x86_get_pd_index(virtual_addr);
    return &page_dir->entries[pd_index];
}

x86_pte_t *x86_get_pte(x86_page_directory_t *page_dir, uint32_t virtual_addr)
{
    if (page_dir == NULL) {
        return NULL;
    }
    
    // Get page directory entry
    x86_pde_t *pde = x86_get_pde(page_dir, virtual_addr);
    if (pde == NULL || !pde->fields.present) {
        return NULL;
    }
    
    // Find page table in our tracking arrays
    uint32_t pt_phys = pde->fields.page_table_addr << 12;
    for (size_t i = 0; i < X86_PDE_COUNT; i++) {
        if (g_mmu_context.page_table_phys[i] == pt_phys) {
            x86_page_table_t *page_table = g_mmu_context.page_tables[i];
            if (page_table != NULL) {
                uint32_t pt_index = x86_get_pt_index(virtual_addr);
                return &page_table->entries[pt_index];
            }
            break;
        }
    }
    
    return NULL;
}

bool x86_is_page_present(x86_page_directory_t *page_dir, uint32_t virtual_addr)
{
    x86_pte_t *pte = x86_get_pte(page_dir, virtual_addr);
    return (pte != NULL && pte->fields.present);
}

int x86_set_page_protection(
    x86_page_directory_t *page_dir,
    uint32_t virtual_addr,
    x86_memory_protection_t protection)
{
    if (!g_x86_paging_initialized || page_dir == NULL) {
        return -1;
    }
    
    x86_pte_t *pte = x86_get_pte(page_dir, virtual_addr);
    if (pte == NULL || !pte->fields.present) {
        return -1;
    }
    
    // Update protection flags
    pte->fields.writable = (protection & X86_PROT_WRITE) ? 1 : 0;
    pte->fields.user = (protection & X86_PROT_USER) ? 1 : 0;
    
    // Flush TLB for this page
    x86_flush_tlb_page(virtual_addr);
    
    return 0;
}

x86_memory_protection_t x86_get_page_protection(
    x86_page_directory_t *page_dir,
    uint32_t virtual_addr)
{
    if (!g_x86_paging_initialized || page_dir == NULL) {
        return X86_PROT_NONE;
    }
    
    x86_pte_t *pte = x86_get_pte(page_dir, virtual_addr);
    if (pte == NULL || !pte->fields.present) {
        return X86_PROT_NONE;
    }
    
    x86_memory_protection_t protection = X86_PROT_READ; // Always readable if present
    
    if (pte->fields.writable) {
        protection |= X86_PROT_WRITE;
    }
    
    if (pte->fields.user) {
        protection |= X86_PROT_USER;
    }
    
    // Note: x86 doesn't have explicit execute disable in basic paging
    protection |= X86_PROT_EXEC;
    
    return protection;
}

x86_mmu_context_t *x86_get_mmu_context(void)
{
    if (!g_x86_paging_initialized) {
        return NULL;
    }
    
    return &g_mmu_context;
}

// =============================================================================
// PRIVATE FUNCTION IMPLEMENTATIONS
// =============================================================================

static uint32_t x86_get_physical_address(void *virtual_addr)
{
    // In a real implementation, this would convert virtual to physical address
    // For now, we assume identity mapping or use a simple conversion
    return (uint32_t)(uintptr_t)virtual_addr;
}

static void *x86_allocate_physical_page(void)
{
    // In a real implementation, this would allocate from physical memory manager
    return aligned_alloc(X86_PAGE_SIZE, X86_PAGE_SIZE);
}

static void x86_free_physical_page(void *page)
{
    if (page != NULL) {
        free(page);
    }
}

static int x86_ensure_page_table(x86_page_directory_t *page_dir, uint32_t virtual_addr)
{
    if (page_dir == NULL) {
        return -1;
    }
    
    uint32_t pd_index = x86_get_pd_index(virtual_addr);
    x86_pde_t *pde = &page_dir->entries[pd_index];
    
    // Check if page table already exists
    if (pde->fields.present) {
        return 0; // Already exists
    }
    
    // Create new page table
    x86_page_table_t *page_table = x86_create_page_table();
    if (page_table == NULL) {
        return -1;
    }
    
    // Get physical address of page table
    uint32_t pt_phys = x86_get_physical_address(page_table);
    if (pt_phys == 0) {
        x86_destroy_page_table(page_table);
        return -1;
    }
    
    // Find free slot in tracking arrays
    size_t free_slot = SIZE_MAX;
    for (size_t i = 0; i < X86_PDE_COUNT; i++) {
        if (g_mmu_context.page_tables[i] == NULL) {
            free_slot = i;
            break;
        }
    }
    
    if (free_slot == SIZE_MAX) {
        x86_destroy_page_table(page_table);
        return -1; // No free slots
    }
    
    // Store page table in tracking arrays
    g_mmu_context.page_tables[free_slot] = page_table;
    g_mmu_context.page_table_phys[free_slot] = pt_phys;
    g_mmu_context.allocated_tables++;
    
    // Set up page directory entry
    pde->raw = 0;
    pde->fields.present = 1;
    pde->fields.writable = 1;
    pde->fields.user = 1;
    pde->fields.page_table_addr = pt_phys >> 12;
    
    return 0;
}
