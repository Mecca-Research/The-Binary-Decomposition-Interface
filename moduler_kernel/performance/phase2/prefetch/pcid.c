
/**
 * @file pcid.c
 * @brief PCID/ASID management implementation
 */

#include "pcid.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// PCID allocation entry
typedef struct {
    uint64_t context_id;
    uint64_t last_used;
    bool allocated;
} pcid_entry_t;

// Global state
static pcid_entry_t g_pcid_table[PCID_SPACE_SIZE] = {0};
static pcid_stats_t g_stats = {0};
static pcid_config_t g_config = {0};
static bool g_initialized = false;
static uint64_t g_timestamp = 0;

/**
 * @brief Check CPUID for PCID support
 */
static bool check_pcid_support(void) {
    // In userspace, we can't check CPUID easily
    // In kernel, would use cpuid instruction
    // For now, assume supported on x86-64
    return true;
}

/**
 * @brief Check CPUID for INVPCID support
 */
static bool check_invpcid_support(void) {
    // In userspace, we can't check CPUID easily
    // In kernel, would check CPUID leaf 7, subleaf 0, EBX bit 10
    // For now, assume supported on modern x86-64
    return true;
}

int pcid_init(const pcid_config_t* config) {
    if (g_initialized) {
        return 0;
    }
    
    // Set default configuration
    g_config.enable_pcid = true;
    g_config.enable_invpcid = true;
    g_config.eviction_threshold = PCID_SPACE_SIZE * 3 / 4;  // 75%
    
    // Override with user config
    if (config) {
        g_config = *config;
    }
    
    // Check hardware support
    if (g_config.enable_pcid && !check_pcid_support()) {
        fprintf(stderr, "Warning: PCID not supported, disabling\n");
        g_config.enable_pcid = false;
    }
    
    if (g_config.enable_invpcid && !check_invpcid_support()) {
        fprintf(stderr, "Warning: INVPCID not supported, disabling\n");
        g_config.enable_invpcid = false;
    }
    
    // Initialize PCID table
    memset(g_pcid_table, 0, sizeof(g_pcid_table));
    
    // Reserve PCID 0 for kernel
    g_pcid_table[PCID_KERNEL].allocated = true;
    g_pcid_table[PCID_KERNEL].context_id = 0;
    
    memset(&g_stats, 0, sizeof(g_stats));
    g_timestamp = 0;
    g_initialized = true;
    
    return 0;
}

uint16_t pcid_alloc(uint64_t context_id) {
    if (!g_initialized || !g_config.enable_pcid) {
        return PCID_INVALID;
    }
    
    g_timestamp++;
    
    // Check if already allocated
    for (uint32_t i = 1; i < PCID_SPACE_SIZE; i++) {
        if (g_pcid_table[i].allocated && g_pcid_table[i].context_id == context_id) {
            g_pcid_table[i].last_used = g_timestamp;
            return i;
        }
    }
    
    // Find free PCID
    for (uint32_t i = 1; i < PCID_SPACE_SIZE; i++) {
        if (!g_pcid_table[i].allocated) {
            g_pcid_table[i].allocated = true;
            g_pcid_table[i].context_id = context_id;
            g_pcid_table[i].last_used = g_timestamp;
            
            g_stats.total_allocations++;
            g_stats.current_allocated++;
            
            if (g_stats.current_allocated > g_stats.peak_allocated) {
                g_stats.peak_allocated = g_stats.current_allocated;
            }
            
            return i;
        }
    }
    
    // No free PCID, evict LRU
    if (g_stats.current_allocated >= g_config.eviction_threshold) {
        uint32_t lru_pcid = 1;
        uint64_t lru_time = g_pcid_table[1].last_used;
        
        for (uint32_t i = 2; i < PCID_SPACE_SIZE; i++) {
            if (g_pcid_table[i].allocated && g_pcid_table[i].last_used < lru_time) {
                lru_time = g_pcid_table[i].last_used;
                lru_pcid = i;
            }
        }
        
        // Evict LRU PCID
        pcid_invalidate_all(lru_pcid);
        g_pcid_table[lru_pcid].allocated = true;
        g_pcid_table[lru_pcid].context_id = context_id;
        g_pcid_table[lru_pcid].last_used = g_timestamp;
        
        g_stats.evictions++;
        
        return lru_pcid;
    }
    
    return PCID_INVALID;
}

void pcid_free(uint16_t pcid) {
    if (!g_initialized || pcid >= PCID_SPACE_SIZE || pcid == PCID_KERNEL) {
        return;
    }
    
    if (g_pcid_table[pcid].allocated) {
        pcid_invalidate_all(pcid);
        g_pcid_table[pcid].allocated = false;
        g_pcid_table[pcid].context_id = 0;
        
        g_stats.total_frees++;
        g_stats.current_allocated--;
    }
}

uint16_t pcid_get(uint64_t context_id) {
    if (!g_initialized) {
        return PCID_INVALID;
    }
    
    for (uint32_t i = 1; i < PCID_SPACE_SIZE; i++) {
        if (g_pcid_table[i].allocated && g_pcid_table[i].context_id == context_id) {
            return i;
        }
    }
    
    return PCID_INVALID;
}

void pcid_invalidate(uint16_t pcid, uintptr_t addr) {
    if (!g_initialized || pcid >= PCID_SPACE_SIZE) {
        return;
    }
    
    if (g_config.enable_invpcid) {
        // In kernel, would use INVPCID instruction
        // Type 0: Invalidate individual address
        // invpcid_flush_one(pcid, addr);
        g_stats.selective_invalidations++;
        g_stats.tlb_flushes_avoided++;
    } else {
        // Fallback: full TLB flush
        // In kernel, would reload CR3
        g_stats.full_invalidations++;
    }
}

void pcid_invalidate_all(uint16_t pcid) {
    if (!g_initialized || pcid >= PCID_SPACE_SIZE) {
        return;
    }
    
    if (g_config.enable_invpcid) {
        // In kernel, would use INVPCID instruction
        // Type 1: Invalidate all entries for PCID
        // invpcid_flush_single_context(pcid);
        g_stats.selective_invalidations++;
        g_stats.tlb_flushes_avoided++;
    } else {
        // Fallback: full TLB flush
        g_stats.full_invalidations++;
    }
}

void pcid_flush_all(void) {
    if (!g_initialized) {
        return;
    }
    
    if (g_config.enable_invpcid) {
        // In kernel, would use INVPCID instruction
        // Type 2: Invalidate all entries (including global)
        // invpcid_flush_all();
    } else {
        // Reload CR3
    }
    
    g_stats.full_invalidations++;
}

bool pcid_is_supported(void) {
    return g_initialized && g_config.enable_pcid;
}

bool pcid_is_invpcid_supported(void) {
    return g_initialized && g_config.enable_invpcid;
}

int pcid_get_stats(pcid_stats_t* stats) {
    if (!g_initialized || !stats) {
        return -1;
    }
    
    *stats = g_stats;
    return 0;
}

void pcid_reset_stats(void) {
    if (!g_initialized) {
        return;
    }
    
    memset(&g_stats, 0, sizeof(g_stats));
}

void pcid_print_stats(void) {
    if (!g_initialized) {
        return;
    }
    
    printf("PCID Statistics:\n");
    printf("  Total Allocations: %lu\n", g_stats.total_allocations);
    printf("  Total Frees: %lu\n", g_stats.total_frees);
    printf("  Evictions: %lu\n", g_stats.evictions);
    printf("  Current Allocated: %u / %d\n", g_stats.current_allocated, PCID_SPACE_SIZE);
    printf("  Peak Allocated: %u\n", g_stats.peak_allocated);
    printf("\n");
    
    printf("  TLB Operations:\n");
    printf("    Flushes Avoided: %lu\n", g_stats.tlb_flushes_avoided);
    printf("    Selective Invalidations: %lu\n", g_stats.selective_invalidations);
    printf("    Full Invalidations: %lu\n", g_stats.full_invalidations);
    
    if (g_stats.selective_invalidations + g_stats.full_invalidations > 0) {
        double selective_pct = 100.0 * g_stats.selective_invalidations / 
                               (g_stats.selective_invalidations + g_stats.full_invalidations);
        printf("    Selective Rate: %.1f%%\n", selective_pct);
    }
}

void pcid_destroy(void) {
    memset(g_pcid_table, 0, sizeof(g_pcid_table));
    memset(&g_stats, 0, sizeof(g_stats));
    memset(&g_config, 0, sizeof(g_config));
    g_initialized = false;
}
