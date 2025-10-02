
/**
 * @file huge_pages.c
 * @brief Huge page allocator implementation
 */

#define _GNU_SOURCE
#include "huge_pages.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <errno.h>

// Global state
static huge_page_stats_t g_stats = {0};
static huge_page_config_t g_config = {0};
static bool g_initialized = false;

int huge_page_init(const huge_page_config_t* config) {
    if (g_initialized) {
        return 0;
    }
    
    // Set default configuration
    g_config.enable_2mb = true;
    g_config.enable_1gb = true;
    g_config.enable_thp = true;
    g_config.promotion_threshold = 512 * 1024;  // 512KB
    g_config.demotion_threshold = 80;           // 80% memory pressure
    
    // Override with user config
    if (config) {
        g_config = *config;
    }
    
    // Check if huge pages are available
    if (access("/sys/kernel/mm/hugepages", F_OK) != 0) {
        fprintf(stderr, "Warning: Huge pages not available\n");
        g_config.enable_2mb = false;
        g_config.enable_1gb = false;
    }
    
    memset(&g_stats, 0, sizeof(g_stats));
    g_initialized = true;
    
    return 0;
}

void* huge_page_alloc(huge_page_type_t type) {
    if (!g_initialized) {
        huge_page_init(NULL);
    }
    
    size_t size;
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    
    switch (type) {
        case HUGE_PAGE_TYPE_2MB:
            if (!g_config.enable_2mb) {
                g_stats.failed_2mb_allocs++;
                return NULL;
            }
            size = HUGE_PAGE_2MB;
            flags |= MAP_HUGETLB | (21 << MAP_HUGE_SHIFT);  // 2^21 = 2MB
            g_stats.total_2mb_allocs++;
            break;
            
        case HUGE_PAGE_TYPE_1GB:
            if (!g_config.enable_1gb) {
                g_stats.failed_1gb_allocs++;
                return NULL;
            }
            size = HUGE_PAGE_1GB;
            flags |= MAP_HUGETLB | (30 << MAP_HUGE_SHIFT);  // 2^30 = 1GB
            g_stats.total_1gb_allocs++;
            break;
            
        default:
            return NULL;
    }
    
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, flags, -1, 0);
    
    if (ptr == MAP_FAILED) {
        if (type == HUGE_PAGE_TYPE_2MB) {
            g_stats.failed_2mb_allocs++;
        } else {
            g_stats.failed_1gb_allocs++;
        }
        return NULL;
    }
    
    // Update statistics
    if (type == HUGE_PAGE_TYPE_2MB) {
        g_stats.current_2mb_pages++;
        if (g_stats.current_2mb_pages > g_stats.peak_2mb_pages) {
            g_stats.peak_2mb_pages = g_stats.current_2mb_pages;
        }
    } else {
        g_stats.current_1gb_pages++;
        if (g_stats.current_1gb_pages > g_stats.peak_1gb_pages) {
            g_stats.peak_1gb_pages = g_stats.current_1gb_pages;
        }
    }
    
    return ptr;
}

void* huge_page_alloc_node(huge_page_type_t type, uint32_t node) {
    // For userspace, we can't easily bind to NUMA node with mmap
    // In kernel, would use alloc_pages_node() or similar
    // For now, just allocate and hope for the best
    return huge_page_alloc(type);
}

void huge_page_free(void* ptr, huge_page_type_t type) {
    if (!ptr || !g_initialized) {
        return;
    }
    
    size_t size;
    
    switch (type) {
        case HUGE_PAGE_TYPE_2MB:
            size = HUGE_PAGE_2MB;
            g_stats.total_2mb_frees++;
            g_stats.current_2mb_pages--;
            break;
            
        case HUGE_PAGE_TYPE_1GB:
            size = HUGE_PAGE_1GB;
            g_stats.total_1gb_frees++;
            g_stats.current_1gb_pages--;
            break;
            
        default:
            return;
    }
    
    munmap(ptr, size);
}

int huge_page_promote(void* addr, size_t size) {
    if (!g_initialized || !g_config.enable_thp) {
        return -1;
    }
    
    // Check alignment
    if ((uintptr_t)addr % HUGE_PAGE_2MB != 0) {
        return -1;
    }
    
    if (size % HUGE_PAGE_2MB != 0) {
        return -1;
    }
    
    // Use madvise to hint for huge pages
    if (madvise(addr, size, MADV_HUGEPAGE) < 0) {
        return -1;
    }
    
    g_stats.promotions++;
    return 0;
}

int huge_page_demote(void* addr, huge_page_type_t type) {
    if (!g_initialized) {
        return -1;
    }
    
    size_t size;
    
    switch (type) {
        case HUGE_PAGE_TYPE_2MB:
            size = HUGE_PAGE_2MB;
            break;
        case HUGE_PAGE_TYPE_1GB:
            size = HUGE_PAGE_1GB;
            break;
        default:
            return -1;
    }
    
    // Use madvise to hint for normal pages
    if (madvise(addr, size, MADV_NOHUGEPAGE) < 0) {
        return -1;
    }
    
    g_stats.demotions++;
    return 0;
}

bool huge_page_is_huge(void* addr, huge_page_type_t* type) {
    // In userspace, we can't easily determine this
    // In kernel, would check page table entries
    // For now, return false
    if (type) {
        *type = HUGE_PAGE_TYPE_NONE;
    }
    return false;
}

int huge_page_get_stats(huge_page_stats_t* stats) {
    if (!g_initialized || !stats) {
        return -1;
    }
    
    *stats = g_stats;
    return 0;
}

void huge_page_reset_stats(void) {
    if (!g_initialized) {
        return;
    }
    
    memset(&g_stats, 0, sizeof(g_stats));
}

void huge_page_print_stats(void) {
    if (!g_initialized) {
        return;
    }
    
    printf("Huge Page Statistics:\n");
    printf("  2MB Pages:\n");
    printf("    Total Allocations: %lu\n", g_stats.total_2mb_allocs);
    printf("    Total Frees: %lu\n", g_stats.total_2mb_frees);
    printf("    Failed Allocations: %lu\n", g_stats.failed_2mb_allocs);
    printf("    Current Pages: %lu\n", g_stats.current_2mb_pages);
    printf("    Peak Pages: %lu\n", g_stats.peak_2mb_pages);
    printf("\n");
    
    printf("  1GB Pages:\n");
    printf("    Total Allocations: %lu\n", g_stats.total_1gb_allocs);
    printf("    Total Frees: %lu\n", g_stats.total_1gb_frees);
    printf("    Failed Allocations: %lu\n", g_stats.failed_1gb_allocs);
    printf("    Current Pages: %lu\n", g_stats.current_1gb_pages);
    printf("    Peak Pages: %lu\n", g_stats.peak_1gb_pages);
    printf("\n");
    
    printf("  Transparent Huge Pages:\n");
    printf("    Promotions: %lu\n", g_stats.promotions);
    printf("    Demotions: %lu\n", g_stats.demotions);
}

void huge_page_destroy(void) {
    memset(&g_stats, 0, sizeof(g_stats));
    memset(&g_config, 0, sizeof(g_config));
    g_initialized = false;
}
