
/**
 * @file vmm.h
 * @brief Virtual Memory Manager for BDI Kernel
 * @details Implements virtual address space management, page tables, and memory mapping.
 *          Provides page fault handling, memory reclamation, and TLB management.
 * 
 * Phase 1: Memory & HAM Readiness
 * - Virtual address space management
 * - Page table management
 * - Page fault handling
 * - Memory reclamation (LRU, active/inactive lists)
 * - TLB management
 * - Memory mapping and unmapping
 * 
 * @author BDI Kernel Team
 * @date 2024
 * @standard C23
 */

#ifndef BDI_KERNEL_VMM_H
#define BDI_KERNEL_VMM_H

#include "../c23_compat.h"
#include "memory.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

// ============================================================================
// VMM Configuration
// ============================================================================

#define VMM_PAGE_TABLE_LEVELS   4
#define VMM_ENTRIES_PER_TABLE   512
#define VMM_USER_SPACE_END      0x00007FFFFFFFFFFF
#define VMM_KERNEL_SPACE_START  0xFFFF800000000000

// Page table entry flags
#define PTE_PRESENT     (1ULL << 0)
#define PTE_WRITABLE    (1ULL << 1)
#define PTE_USER        (1ULL << 2)
#define PTE_WRITETHROUGH (1ULL << 3)
#define PTE_NOCACHE     (1ULL << 4)
#define PTE_ACCESSED    (1ULL << 5)
#define PTE_DIRTY       (1ULL << 6)
#define PTE_HUGE        (1ULL << 7)
#define PTE_GLOBAL      (1ULL << 8)
#define PTE_NX          (1ULL << 63)

// Memory protection flags
#define PROT_NONE       0x0
#define PROT_READ       0x1
#define PROT_WRITE      0x2
#define PROT_EXEC       0x4

// Memory mapping flags
#define MAP_SHARED      0x01
#define MAP_PRIVATE     0x02
#define MAP_FIXED       0x10
#define MAP_ANONYMOUS   0x20
#define MAP_POPULATE    0x40
#define MAP_LOCKED      0x80

// ============================================================================
// Page Table Structures
// ============================================================================

/**
 * @brief Page table entry
 */
typedef struct {
    uint64_t value;
} pte_t;

/**
 * @brief Page table
 */
typedef struct {
    pte_t entries[VMM_ENTRIES_PER_TABLE];
    atomic_int refcount;
} page_table_t;

/**
 * @brief Virtual memory area
 */
typedef struct vm_area {
    uint64_t start;
    uint64_t end;
    uint32_t flags;
    uint32_t prot;
    struct vm_area *next;
    
    // Statistics
    atomic_uint_fast64_t fault_count;
    atomic_uint_fast64_t access_count;
} vm_area_t;

/**
 * @brief Address space descriptor
 */
typedef struct {
    page_table_t *pgd;              // Page global directory
    vm_area_t *vma_list;            // List of VMAs
    uint64_t total_vm;              // Total virtual memory
    uint64_t locked_vm;             // Locked memory
    atomic_int mm_users;            // Number of users
    atomic_flag lock;               // Address space lock
    
    // Statistics
    atomic_uint_fast64_t page_faults;
    atomic_uint_fast64_t major_faults;
    atomic_uint_fast64_t minor_faults;
} mm_struct_t;

// ============================================================================
// Page Fault Handling
// ============================================================================

/**
 * @brief Page fault error codes
 */
typedef enum {
    FAULT_ERROR_NONE = 0,
    FAULT_ERROR_NOPAGE,
    FAULT_ERROR_PROT,
    FAULT_ERROR_WRITE,
    FAULT_ERROR_USER,
    FAULT_ERROR_EXEC
} fault_error_t;

/**
 * @brief Page fault information
 */
typedef struct {
    uint64_t address;
    uint32_t error_code;
    bool write;
    bool user;
    bool exec;
    bool present;
} fault_info_t;

// ============================================================================
// Memory Reclamation
// ============================================================================

/**
 * @brief LRU list types
 */
typedef enum {
    LRU_INACTIVE_ANON,
    LRU_ACTIVE_ANON,
    LRU_INACTIVE_FILE,
    LRU_ACTIVE_FILE,
    LRU_UNEVICTABLE,
    NR_LRU_LISTS
} lru_list_t;

/**
 * @brief LRU list
 */
typedef struct {
    page_frame_t *head;
    page_frame_t *tail;
    atomic_uint_fast64_t count;
    atomic_flag lock;
} lru_list_head_t;

/**
 * @brief Memory reclamation state
 */
typedef struct {
    lru_list_head_t lists[NR_LRU_LISTS];
    
    // Reclamation statistics
    atomic_uint_fast64_t pages_scanned;
    atomic_uint_fast64_t pages_reclaimed;
    atomic_uint_fast64_t pages_activated;
    atomic_uint_fast64_t pages_deactivated;
    
    // Thresholds
    uint64_t min_free_pages;
    uint64_t low_free_pages;
    uint64_t high_free_pages;
} reclaim_state_t;

// ============================================================================
// TLB Management
// ============================================================================

/**
 * @brief TLB entry
 */
typedef struct {
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t flags;
    bool valid;
} tlb_entry_t;

/**
 * @brief TLB cache
 */
typedef struct {
    tlb_entry_t entries[64];
    atomic_uint_fast32_t next_victim;
    
    // Statistics
    atomic_uint_fast64_t hits;
    atomic_uint_fast64_t misses;
    atomic_uint_fast64_t flushes;
} tlb_cache_t;

// ============================================================================
// VMM Interface
// ============================================================================

/**
 * @brief Initialize virtual memory manager
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_init(void);

/**
 * @brief Shutdown virtual memory manager
 */
void vmm_shutdown(void);

/**
 * @brief Create new address space
 * @return Pointer to mm_struct or nullptr on failure
 */
NODISCARD mm_struct_t* vmm_create_address_space(void);

/**
 * @brief Destroy address space
 * @param mm Address space descriptor
 */
void vmm_destroy_address_space(mm_struct_t *mm);

/**
 * @brief Map virtual address to physical page
 * @param mm Address space descriptor
 * @param vaddr Virtual address
 * @param page Physical page frame
 * @param prot Protection flags
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_map_page(mm_struct_t *mm, uint64_t vaddr, page_frame_t *page, uint32_t prot);

/**
 * @brief Unmap virtual address
 * @param mm Address space descriptor
 * @param vaddr Virtual address
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_unmap_page(mm_struct_t *mm, uint64_t vaddr);

/**
 * @brief Map memory region
 * @param mm Address space descriptor
 * @param addr Desired address (or 0 for any)
 * @param length Length in bytes
 * @param prot Protection flags
 * @param flags Mapping flags
 * @return Mapped address or 0 on failure
 */
NODISCARD uint64_t vmm_mmap(mm_struct_t *mm, uint64_t addr, size_t length, 
                            uint32_t prot, uint32_t flags);

/**
 * @brief Unmap memory region
 * @param mm Address space descriptor
 * @param addr Address to unmap
 * @param length Length in bytes
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_munmap(mm_struct_t *mm, uint64_t addr, size_t length);

/**
 * @brief Change memory protection
 * @param mm Address space descriptor
 * @param addr Address
 * @param length Length in bytes
 * @param prot New protection flags
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_mprotect(mm_struct_t *mm, uint64_t addr, size_t length, uint32_t prot);

// ============================================================================
// Page Fault Handling Interface
// ============================================================================

/**
 * @brief Handle page fault
 * @param mm Address space descriptor
 * @param fault Fault information
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_handle_page_fault(mm_struct_t *mm, const fault_info_t *fault);

/**
 * @brief Handle demand paging
 * @param mm Address space descriptor
 * @param vaddr Virtual address
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_handle_demand_page(mm_struct_t *mm, uint64_t vaddr);

/**
 * @brief Handle copy-on-write
 * @param mm Address space descriptor
 * @param vaddr Virtual address
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_handle_cow(mm_struct_t *mm, uint64_t vaddr);

// ============================================================================
// Memory Reclamation Interface
// ============================================================================

/**
 * @brief Initialize memory reclamation
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_reclaim_init(void);

/**
 * @brief Add page to LRU list
 * @param page Page frame
 * @param lru LRU list type
 */
void vmm_lru_add(page_frame_t *page, lru_list_t lru);

/**
 * @brief Remove page from LRU list
 * @param page Page frame
 */
void vmm_lru_remove(page_frame_t *page);

/**
 * @brief Activate page (move to active list)
 * @param page Page frame
 */
void vmm_activate_page(page_frame_t *page);

/**
 * @brief Deactivate page (move to inactive list)
 * @param page Page frame
 */
void vmm_deactivate_page(page_frame_t *page);

/**
 * @brief Reclaim pages
 * @param nr_pages Number of pages to reclaim
 * @return Number of pages actually reclaimed
 */
NODISCARD uint64_t vmm_reclaim_pages(uint64_t nr_pages);

/**
 * @brief Shrink caches
 * @return Number of pages freed
 */
NODISCARD uint64_t vmm_shrink_caches(void);

// ============================================================================
// TLB Management Interface
// ============================================================================

/**
 * @brief Initialize TLB
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_tlb_init(void);

/**
 * @brief Lookup TLB entry
 * @param vaddr Virtual address
 * @param paddr Output physical address
 * @return true if hit, false if miss
 */
NODISCARD bool vmm_tlb_lookup(uint64_t vaddr, uint64_t *paddr);

/**
 * @brief Insert TLB entry
 * @param vaddr Virtual address
 * @param paddr Physical address
 * @param flags Entry flags
 */
void vmm_tlb_insert(uint64_t vaddr, uint64_t paddr, uint64_t flags);

/**
 * @brief Flush TLB entry
 * @param vaddr Virtual address
 */
void vmm_tlb_flush_entry(uint64_t vaddr);

/**
 * @brief Flush entire TLB
 */
void vmm_tlb_flush_all(void);

/**
 * @brief Get TLB statistics
 * @param hits Output hit count
 * @param misses Output miss count
 */
void vmm_tlb_get_stats(uint64_t *hits, uint64_t *misses);

// ============================================================================
// Page Table Management Interface
// ============================================================================

/**
 * @brief Allocate page table
 * @return Pointer to page table or nullptr on failure
 */
NODISCARD page_table_t* vmm_alloc_page_table(void);

/**
 * @brief Free page table
 * @param pt Page table pointer
 */
void vmm_free_page_table(page_table_t *pt);

/**
 * @brief Walk page tables
 * @param mm Address space descriptor
 * @param vaddr Virtual address
 * @param create Create missing tables if true
 * @return Pointer to PTE or nullptr
 */
NODISCARD pte_t* vmm_walk_page_tables(mm_struct_t *mm, uint64_t vaddr, bool create);

/**
 * @brief Get physical address from virtual
 * @param mm Address space descriptor
 * @param vaddr Virtual address
 * @param paddr Output physical address
 * @return 0 on success, negative error code on failure
 */
NODISCARD int vmm_virt_to_phys(mm_struct_t *mm, uint64_t vaddr, uint64_t *paddr);

// ============================================================================
// Diagnostic Interface
// ============================================================================

/**
 * @brief Print VMM statistics
 */
void vmm_print_stats(void);

/**
 * @brief Dump address space
 * @param mm Address space descriptor
 */
void vmm_dump_address_space(mm_struct_t *mm);

/**
 * @brief Validate VMM state
 * @return true if valid, false if corruption detected
 */
NODISCARD bool vmm_validate(void);

// ============================================================================
// Compile-time Assertions
// ============================================================================

static_assert(sizeof(pte_t) == 8, "PTE must be 8 bytes");
static_assert(VMM_ENTRIES_PER_TABLE == 512, "Page table must have 512 entries");

#endif // BDI_KERNEL_VMM_H
