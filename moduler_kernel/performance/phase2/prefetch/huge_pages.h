
/**
 * @file huge_pages.h
 * @brief Huge page allocator (2MB/1GB pages)
 * 
 * Provides transparent huge page support with automatic promotion/demotion.
 * Reduces TLB pressure and page table overhead.
 */

#ifndef PHASE2_HUGE_PAGES_H
#define PHASE2_HUGE_PAGES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Huge page sizes
#define HUGE_PAGE_2MB (2 * 1024 * 1024)
#define HUGE_PAGE_1GB (1024 * 1024 * 1024)

// Huge page types
typedef enum {
    HUGE_PAGE_TYPE_2MB = 0,
    HUGE_PAGE_TYPE_1GB = 1,
    HUGE_PAGE_TYPE_NONE = 2
} huge_page_type_t;

/**
 * @brief Huge page statistics
 */
typedef struct {
    uint64_t total_2mb_allocs;
    uint64_t total_1gb_allocs;
    uint64_t total_2mb_frees;
    uint64_t total_1gb_frees;
    uint64_t failed_2mb_allocs;
    uint64_t failed_1gb_allocs;
    uint64_t promotions;        // 4KB -> 2MB
    uint64_t demotions;         // 2MB -> 4KB
    uint64_t current_2mb_pages;
    uint64_t current_1gb_pages;
    uint64_t peak_2mb_pages;
    uint64_t peak_1gb_pages;
} huge_page_stats_t;

/**
 * @brief Huge page configuration
 */
typedef struct {
    bool enable_2mb;            // Enable 2MB pages
    bool enable_1gb;            // Enable 1GB pages
    bool enable_thp;            // Enable transparent huge pages
    size_t promotion_threshold; // Size threshold for promotion
    size_t demotion_threshold;  // Memory pressure threshold for demotion
} huge_page_config_t;

/**
 * @brief Initialize huge page allocator
 * 
 * @param config Configuration (NULL = defaults)
 * @return 0 on success, -1 on failure
 */
int huge_page_init(const huge_page_config_t* config);

/**
 * @brief Allocate huge page
 * 
 * @param type Huge page type (2MB or 1GB)
 * @return Pointer to huge page, or NULL on failure
 */
void* huge_page_alloc(huge_page_type_t type);

/**
 * @brief Allocate huge page from specific NUMA node
 * 
 * @param type Huge page type
 * @param node NUMA node
 * @return Pointer to huge page, or NULL on failure
 */
void* huge_page_alloc_node(huge_page_type_t type, uint32_t node);

/**
 * @brief Free huge page
 * 
 * @param ptr Pointer to huge page
 * @param type Huge page type
 */
void huge_page_free(void* ptr, huge_page_type_t type);

/**
 * @brief Promote regular pages to huge page
 * 
 * Attempts to promote a range of 4KB pages to a 2MB huge page.
 * 
 * @param addr Address (must be 2MB aligned)
 * @param size Size (must be multiple of 2MB)
 * @return 0 on success, -1 on failure
 */
int huge_page_promote(void* addr, size_t size);

/**
 * @brief Demote huge page to regular pages
 * 
 * Splits a huge page into 4KB pages.
 * 
 * @param addr Address of huge page
 * @param type Huge page type
 * @return 0 on success, -1 on failure
 */
int huge_page_demote(void* addr, huge_page_type_t type);

/**
 * @brief Check if address is huge page
 * 
 * @param addr Address to check
 * @param type Output: huge page type
 * @return true if huge page, false otherwise
 */
bool huge_page_is_huge(void* addr, huge_page_type_t* type);

/**
 * @brief Get huge page statistics
 * 
 * @param stats Output statistics
 * @return 0 on success, -1 on failure
 */
int huge_page_get_stats(huge_page_stats_t* stats);

/**
 * @brief Reset huge page statistics
 */
void huge_page_reset_stats(void);

/**
 * @brief Print huge page statistics
 */
void huge_page_print_stats(void);

/**
 * @brief Destroy huge page allocator
 */
void huge_page_destroy(void);

#ifdef __cplusplus
}
#endif

#endif // PHASE2_HUGE_PAGES_H
