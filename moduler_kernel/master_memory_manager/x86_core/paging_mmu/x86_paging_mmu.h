
/**
 * @file x86_paging_mmu.h
 * @brief x86 Paging and MMU Management Interface
 * 
 * Provides comprehensive x86 memory management unit support including:
 * - Linear address translation through page tables
 * - Page directory and page table management
 * - Memory protection and privilege levels
 * - Virtual memory management
 * - Page fault handling support
 * 
 * Based on technical foundation from Assembly Language for x86 Processors 7th Edition
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#ifndef X86_PAGING_MMU_H
#define X86_PAGING_MMU_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// PAGING CONSTANTS AND DEFINITIONS
// =============================================================================

#define X86_PAGE_SIZE           4096        ///< Standard page size (4KB)
#define X86_PAGE_SHIFT          12          ///< Page size shift (2^12 = 4096)
#define X86_PAGE_MASK           0xFFF       ///< Page offset mask
#define X86_PAGE_ALIGN_MASK     0xFFFFF000  ///< Page alignment mask

#define X86_PDE_COUNT           1024        ///< Page directory entries
#define X86_PTE_COUNT           1024        ///< Page table entries per table
#define X86_PAGES_PER_TABLE     1024        ///< Pages per page table

#define X86_PD_INDEX_SHIFT      22          ///< Page directory index shift
#define X86_PT_INDEX_SHIFT      12          ///< Page table index shift
#define X86_PD_INDEX_MASK       0x3FF       ///< Page directory index mask
#define X86_PT_INDEX_MASK       0x3FF       ///< Page table index mask

/**
 * @brief Page directory/table entry flags
 */
typedef enum {
    X86_PAGE_PRESENT        = 0x001,    ///< Page is present in memory
    X86_PAGE_WRITABLE       = 0x002,    ///< Page is writable
    X86_PAGE_USER           = 0x004,    ///< Page accessible from user mode
    X86_PAGE_WRITE_THROUGH  = 0x008,    ///< Write-through caching
    X86_PAGE_CACHE_DISABLE  = 0x010,    ///< Cache disabled
    X86_PAGE_ACCESSED       = 0x020,    ///< Page has been accessed
    X86_PAGE_DIRTY          = 0x040,    ///< Page has been written to
    X86_PAGE_SIZE_FLAG      = 0x080,    ///< Page size (4MB pages in PDE)
    X86_PAGE_GLOBAL         = 0x100,    ///< Global page (not flushed on CR3 load)
    X86_PAGE_AVAILABLE_1    = 0x200,    ///< Available for OS use
    X86_PAGE_AVAILABLE_2    = 0x400,    ///< Available for OS use
    X86_PAGE_AVAILABLE_3    = 0x800     ///< Available for OS use
} x86_page_flags_t;

/**
 * @brief Memory protection levels
 */
typedef enum {
    X86_PROT_NONE = 0,          ///< No access
    X86_PROT_READ = 1,          ///< Read access
    X86_PROT_WRITE = 2,         ///< Write access
    X86_PROT_EXEC = 4,          ///< Execute access
    X86_PROT_USER = 8           ///< User mode access
} x86_memory_protection_t;

/**
 * @brief Page directory entry structure
 */
typedef union {
    uint32_t raw;
    struct {
        uint32_t present        : 1;    ///< Present bit
        uint32_t writable       : 1;    ///< Read/Write bit
        uint32_t user           : 1;    ///< User/Supervisor bit
        uint32_t write_through  : 1;    ///< Page-level write-through
        uint32_t cache_disable  : 1;    ///< Page-level cache disable
        uint32_t accessed       : 1;    ///< Accessed bit
        uint32_t reserved       : 1;    ///< Reserved (must be 0)
        uint32_t page_size      : 1;    ///< Page size (0=4KB, 1=4MB)
        uint32_t ignored        : 1;    ///< Ignored
        uint32_t available      : 3;    ///< Available for OS use
        uint32_t page_table_addr: 20;   ///< Page table base address
    } fields;
} x86_pde_t;

/**
 * @brief Page table entry structure
 */
typedef union {
    uint32_t raw;
    struct {
        uint32_t present        : 1;    ///< Present bit
        uint32_t writable       : 1;    ///< Read/Write bit
        uint32_t user           : 1;    ///< User/Supervisor bit
        uint32_t write_through  : 1;    ///< Page-level write-through
        uint32_t cache_disable  : 1;    ///< Page-level cache disable
        uint32_t accessed       : 1;    ///< Accessed bit
        uint32_t dirty          : 1;    ///< Dirty bit
        uint32_t pat            : 1;    ///< Page Attribute Table
        uint32_t global         : 1;    ///< Global bit
        uint32_t available      : 3;    ///< Available for OS use
        uint32_t page_addr      : 20;   ///< Physical page address
    } fields;
} x86_pte_t;

/**
 * @brief Linear address breakdown
 */
typedef union {
    uint32_t raw;
    struct {
        uint32_t offset         : 12;   ///< Page offset
        uint32_t page_table_idx : 10;   ///< Page table index
        uint32_t page_dir_idx   : 10;   ///< Page directory index
    } fields;
} x86_linear_addr_t;

/**
 * @brief Page directory structure
 */
typedef struct {
    x86_pde_t entries[X86_PDE_COUNT];   ///< Page directory entries
} __attribute__((aligned(X86_PAGE_SIZE))) x86_page_directory_t;

/**
 * @brief Page table structure
 */
typedef struct {
    x86_pte_t entries[X86_PTE_COUNT];   ///< Page table entries
} __attribute__((aligned(X86_PAGE_SIZE))) x86_page_table_t;

/**
 * @brief Memory mapping descriptor
 */
typedef struct {
    uint32_t virtual_addr;      ///< Virtual address
    uint32_t physical_addr;     ///< Physical address
    size_t size;                ///< Size in bytes
    uint32_t flags;             ///< Page flags
    x86_memory_protection_t protection; ///< Memory protection
} x86_memory_mapping_t;

/**
 * @brief MMU context structure
 */
typedef struct {
    x86_page_directory_t *page_directory;  ///< Current page directory
    uint32_t page_directory_phys;           ///< Physical address of page directory
    x86_page_table_t **page_tables;        ///< Array of page table pointers
    uint32_t *page_table_phys;              ///< Physical addresses of page tables
    size_t allocated_tables;                ///< Number of allocated page tables
    uint32_t cr3_value;                     ///< Current CR3 register value
    bool paging_enabled;                    ///< True if paging is enabled
} x86_mmu_context_t;

/**
 * @brief Page fault information
 */
typedef struct {
    uint32_t fault_address;     ///< Address that caused the fault
    uint32_t error_code;        ///< Page fault error code
    bool present;               ///< Page was present
    bool write_fault;           ///< Fault was due to write
    bool user_fault;            ///< Fault occurred in user mode
    bool reserved_fault;        ///< Reserved bit violation
    bool instruction_fetch;     ///< Fault during instruction fetch
} x86_page_fault_info_t;

// =============================================================================
// FUNCTION DECLARATIONS
// =============================================================================

/**
 * @brief Initialize x86 paging and MMU system
 * @return Status code (0 = success, negative = error)
 */
int x86_paging_mmu_initialize(void);

/**
 * @brief Shutdown x86 paging and MMU system
 * @return Status code (0 = success, negative = error)
 */
int x86_paging_mmu_shutdown(void);

/**
 * @brief Create new page directory
 * @return Pointer to page directory or NULL on error
 */
x86_page_directory_t *x86_create_page_directory(void);

/**
 * @brief Destroy page directory and associated page tables
 * @param page_dir Pointer to page directory to destroy
 */
void x86_destroy_page_directory(x86_page_directory_t *page_dir);

/**
 * @brief Create new page table
 * @return Pointer to page table or NULL on error
 */
x86_page_table_t *x86_create_page_table(void);

/**
 * @brief Destroy page table
 * @param page_table Pointer to page table to destroy
 */
void x86_destroy_page_table(x86_page_table_t *page_table);

/**
 * @brief Map virtual address to physical address
 * @param page_dir Page directory to use
 * @param virtual_addr Virtual address to map
 * @param physical_addr Physical address to map to
 * @param flags Page flags
 * @return Status code (0 = success, negative = error)
 */
int x86_map_page(
    x86_page_directory_t *page_dir,
    uint32_t virtual_addr,
    uint32_t physical_addr,
    uint32_t flags
);

/**
 * @brief Unmap virtual address
 * @param page_dir Page directory to use
 * @param virtual_addr Virtual address to unmap
 * @return Status code (0 = success, negative = error)
 */
int x86_unmap_page(x86_page_directory_t *page_dir, uint32_t virtual_addr);

/**
 * @brief Map memory region
 * @param page_dir Page directory to use
 * @param mapping Memory mapping descriptor
 * @return Status code (0 = success, negative = error)
 */
int x86_map_memory_region(x86_page_directory_t *page_dir, const x86_memory_mapping_t *mapping);

/**
 * @brief Unmap memory region
 * @param page_dir Page directory to use
 * @param virtual_addr Start of virtual address range
 * @param size Size of region to unmap
 * @return Status code (0 = success, negative = error)
 */
int x86_unmap_memory_region(x86_page_directory_t *page_dir, uint32_t virtual_addr, size_t size);

/**
 * @brief Translate virtual address to physical address
 * @param page_dir Page directory to use
 * @param virtual_addr Virtual address to translate
 * @param physical_addr Pointer to store physical address
 * @return Status code (0 = success, negative = error)
 */
int x86_translate_address(
    x86_page_directory_t *page_dir,
    uint32_t virtual_addr,
    uint32_t *physical_addr
);

/**
 * @brief Enable paging
 * @param page_dir Page directory to use
 * @return Status code (0 = success, negative = error)
 */
int x86_enable_paging(x86_page_directory_t *page_dir);

/**
 * @brief Disable paging
 * @return Status code (0 = success, negative = error)
 */
int x86_disable_paging(void);

/**
 * @brief Switch page directory (load CR3)
 * @param page_dir Page directory to switch to
 * @return Status code (0 = success, negative = error)
 */
int x86_switch_page_directory(x86_page_directory_t *page_dir);

/**
 * @brief Get current page directory
 * @return Pointer to current page directory or NULL
 */
x86_page_directory_t *x86_get_current_page_directory(void);

/**
 * @brief Flush TLB for specific page
 * @param virtual_addr Virtual address to flush
 */
void x86_flush_tlb_page(uint32_t virtual_addr);

/**
 * @brief Flush entire TLB
 */
void x86_flush_tlb_all(void);

/**
 * @brief Handle page fault
 * @param fault_info Page fault information
 * @return Status code (0 = handled, negative = error)
 */
int x86_handle_page_fault(const x86_page_fault_info_t *fault_info);

/**
 * @brief Get page directory entry
 * @param page_dir Page directory
 * @param virtual_addr Virtual address
 * @return Pointer to PDE or NULL if invalid
 */
x86_pde_t *x86_get_pde(x86_page_directory_t *page_dir, uint32_t virtual_addr);

/**
 * @brief Get page table entry
 * @param page_dir Page directory
 * @param virtual_addr Virtual address
 * @return Pointer to PTE or NULL if invalid
 */
x86_pte_t *x86_get_pte(x86_page_directory_t *page_dir, uint32_t virtual_addr);

/**
 * @brief Check if page is present
 * @param page_dir Page directory
 * @param virtual_addr Virtual address to check
 * @return True if page is present
 */
bool x86_is_page_present(x86_page_directory_t *page_dir, uint32_t virtual_addr);

/**
 * @brief Set page protection
 * @param page_dir Page directory
 * @param virtual_addr Virtual address
 * @param protection Memory protection flags
 * @return Status code (0 = success, negative = error)
 */
int x86_set_page_protection(
    x86_page_directory_t *page_dir,
    uint32_t virtual_addr,
    x86_memory_protection_t protection
);

/**
 * @brief Get page protection
 * @param page_dir Page directory
 * @param virtual_addr Virtual address
 * @return Memory protection flags
 */
x86_memory_protection_t x86_get_page_protection(
    x86_page_directory_t *page_dir,
    uint32_t virtual_addr
);

/**
 * @brief Get MMU context
 * @return Pointer to current MMU context
 */
x86_mmu_context_t *x86_get_mmu_context(void);

// =============================================================================
// INLINE HELPER FUNCTIONS
// =============================================================================

/**
 * @brief Extract page directory index from virtual address
 * @param virtual_addr Virtual address
 * @return Page directory index
 */
static inline uint32_t x86_get_pd_index(uint32_t virtual_addr)
{
    return (virtual_addr >> X86_PD_INDEX_SHIFT) & X86_PD_INDEX_MASK;
}

/**
 * @brief Extract page table index from virtual address
 * @param virtual_addr Virtual address
 * @return Page table index
 */
static inline uint32_t x86_get_pt_index(uint32_t virtual_addr)
{
    return (virtual_addr >> X86_PT_INDEX_SHIFT) & X86_PT_INDEX_MASK;
}

/**
 * @brief Extract page offset from virtual address
 * @param virtual_addr Virtual address
 * @return Page offset
 */
static inline uint32_t x86_get_page_offset(uint32_t virtual_addr)
{
    return virtual_addr & X86_PAGE_MASK;
}

/**
 * @brief Align address to page boundary
 * @param addr Address to align
 * @return Page-aligned address
 */
static inline uint32_t x86_page_align(uint32_t addr)
{
    return addr & X86_PAGE_ALIGN_MASK;
}

/**
 * @brief Check if address is page-aligned
 * @param addr Address to check
 * @return True if page-aligned
 */
static inline bool x86_is_page_aligned(uint32_t addr)
{
    return (addr & X86_PAGE_MASK) == 0;
}

/**
 * @brief Convert protection flags to page flags
 * @param protection Memory protection flags
 * @return Page flags
 */
static inline uint32_t x86_protection_to_page_flags(x86_memory_protection_t protection)
{
    uint32_t flags = X86_PAGE_PRESENT;
    
    if (protection & X86_PROT_WRITE) {
        flags |= X86_PAGE_WRITABLE;
    }
    
    if (protection & X86_PROT_USER) {
        flags |= X86_PAGE_USER;
    }
    
    return flags;
}

#ifdef __cplusplus
}
#endif

#endif // X86_PAGING_MMU_H
