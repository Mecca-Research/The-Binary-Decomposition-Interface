
// ===================================================================
// BDI Attention-Based Memory Manager Implementation
// Multi-objective allocation with learned priorities
// ===================================================================

#include "attention_mm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

// ===================================================================
// Internal Data Structures
// ===================================================================

#define BDI_MM_MAX_PAGES 65536
#define BDI_MM_PAGE_SIZE 4096

typedef struct bdi_page_entry {
    void* addr;                    // Page address
    size_t size;                   // Allocation size
    bdi_page_meta_t meta;          // Attention metadata
    struct bdi_page_entry* next;   // Next in hash chain
    bool in_use;                   // Whether page is allocated
} bdi_page_entry_t;

struct bdi_attention_mm {
    bdi_attention_config_t config;
    
    // Page tracking
    bdi_page_entry_t pages[BDI_MM_MAX_PAGES];
    bdi_page_entry_t* hash_table[1024];  // Hash table for fast lookup
    uint32_t page_count;
    uint32_t allocated_pages;
    
    // Memory pools
    bdi_memory_pool_info_t pools[BDI_POOL_COUNT];
    uint32_t active_pools;
    
    // Statistics
    bdi_attention_mm_stats_t stats;
    
    // Update tracking
    uint64_t update_counter;
    uint64_t last_gc_time;
    
    // Batch update state
    bool batch_update_active;
    struct {
        void* ptr;
        float signal;
    } batch_updates[256];
    uint32_t batch_count;
};

// ===================================================================
// Hash Table Management
// ===================================================================

static uint32_t hash_ptr(void* ptr) {
    uintptr_t addr = (uintptr_t)ptr;
    return (uint32_t)((addr >> 12) ^ (addr >> 20)) & 1023;
}

static bdi_page_entry_t* find_page_entry(bdi_attention_mm_t* mm, void* ptr) {
    uint32_t hash = hash_ptr(ptr);
    bdi_page_entry_t* entry = mm->hash_table[hash];
    
    while (entry) {
        if (entry->addr <= ptr && 
            (uint8_t*)ptr < (uint8_t*)entry->addr + entry->size) {
            return entry;
        }
        entry = entry->next;
    }
    
    return NULL;
}

static void add_page_entry(bdi_attention_mm_t* mm, bdi_page_entry_t* entry) {
    uint32_t hash = hash_ptr(entry->addr);
    entry->next = mm->hash_table[hash];
    mm->hash_table[hash] = entry;
}

static void remove_page_entry(bdi_attention_mm_t* mm, bdi_page_entry_t* entry) {
    uint32_t hash = hash_ptr(entry->addr);
    bdi_page_entry_t** current = &mm->hash_table[hash];
    
    while (*current) {
        if (*current == entry) {
            *current = entry->next;
            entry->next = NULL;
            break;
        }
        current = &(*current)->next;
    }
}

// ===================================================================
// Attention Score Management
// ===================================================================

static void update_attention_score(bdi_page_meta_t* meta, float signal, 
                                  const bdi_attention_config_t* config) {
    // EMA update for hotness
    meta->hotness = (1.0f - config->hotness_learning_rate) * meta->hotness + 
                    config->hotness_learning_rate * signal;
    
    // Update attention with regularization
    float attention_update = config->attention_learning_rate * 
                           (meta->hotness - config->regularization_factor * meta->attention);
    meta->attention = fmaxf(0.0f, fminf(1.0f, meta->attention + attention_update));
    
    // Decay recency
    meta->recency *= config->recency_decay_rate;
    
    // Update access tracking
    meta->access_count++;
    meta->last_access = 0; // Would use actual timestamp
}

static float compute_eviction_score(const bdi_page_meta_t* meta, 
                                   const bdi_attention_config_t* config) {
    float score = config->weight_attention * meta->attention +
                  config->weight_recency * meta->recency +
                  config->weight_hotness * meta->hotness +
                  config->weight_criticality * ((meta->flags & BDI_PAGE_CRITICAL) ? 1.0f : 0.0f);
    
    // NUMA locality penalty
    if (meta->numa_distance > 0) {
        score -= config->weight_numa_locality * (meta->numa_distance / 255.0f);
    }
    
    return score;
}

// ===================================================================
// Memory Pool Management
// ===================================================================

static void init_default_pools(bdi_attention_mm_t* mm) {
    // Fast pool (simulated)
    mm->pools[BDI_POOL_FAST] = (bdi_memory_pool_info_t){
        .pool_type = BDI_POOL_FAST,
        .base_addr = NULL,
        .total_size = 1024 * 1024 * 1024, // 1GB
        .available_size = 1024 * 1024 * 1024,
        .allocated_size = 0,
        .latency_ns = 50,
        .bandwidth_gbps = 800,
        .power_per_gb = 2.0f,
        .numa_node = 0,
        .cpu_affinity_mask = 0xFF
    };
    
    // Normal pool
    mm->pools[BDI_POOL_NORMAL] = (bdi_memory_pool_info_t){
        .pool_type = BDI_POOL_NORMAL,
        .base_addr = NULL,
        .total_size = 16ULL * 1024 * 1024 * 1024, // 16GB
        .available_size = 16ULL * 1024 * 1024 * 1024,
        .allocated_size = 0,
        .latency_ns = 100,
        .bandwidth_gbps = 200,
        .power_per_gb = 1.0f,
        .numa_node = 0,
        .cpu_affinity_mask = 0xFF
    };
    
    // Slow pool
    mm->pools[BDI_POOL_SLOW] = (bdi_memory_pool_info_t){
        .pool_type = BDI_POOL_SLOW,
        .base_addr = NULL,
        .total_size = 64ULL * 1024 * 1024 * 1024, // 64GB
        .available_size = 64ULL * 1024 * 1024 * 1024,
        .allocated_size = 0,
        .latency_ns = 1000,
        .bandwidth_gbps = 50,
        .power_per_gb = 0.5f,
        .numa_node = 0,
        .cpu_affinity_mask = 0xFF
    };
    
    mm->active_pools = 3;
}

// ===================================================================
// Memory Manager Creation and Destruction
// ===================================================================

bdi_attention_mm_t* bdi_attention_mm_create(const bdi_attention_config_t* config) {
    bdi_attention_mm_t* mm = calloc(1, sizeof(bdi_attention_mm_t));
    if (!mm) return NULL;
    
    // Copy configuration
    if (config) {
        mm->config = *config;
    } else {
        // Default configuration
        mm->config = (bdi_attention_config_t){
            .attention_learning_rate = 0.02f,
            .recency_decay_rate = 0.95f,
            .hotness_learning_rate = 0.1f,
            .regularization_factor = 0.01f,
            .weight_attention = 0.6f,
            .weight_recency = 0.3f,
            .weight_hotness = 0.1f,
            .weight_numa_locality = 0.2f,
            .weight_criticality = 0.4f,
            .eviction_threshold = 0.3f,
            .promotion_threshold = 0.7f,
            .demotion_threshold = 0.2f,
            .update_frequency = 1000,
            .gc_frequency = 10000,
            .enable_prefetching = true,
            .enable_numa_balancing = true
        };
    }
    
    // Initialize memory pools
    init_default_pools(mm);
    
    // Initialize statistics
    memset(&mm->stats, 0, sizeof(mm->stats));
    
    return mm;
}

void bdi_attention_mm_destroy(bdi_attention_mm_t* mm) {
    if (!mm) return;
    
    // Free all allocated pages
    for (uint32_t i = 0; i < mm->page_count; i++) {
        if (mm->pages[i].in_use && mm->pages[i].addr) {
            free(mm->pages[i].addr);
        }
    }
    
    free(mm);
}

// ===================================================================
// Memory Allocation
// ===================================================================

void* bdi_attention_alloc(bdi_attention_mm_t* mm, size_t size, uint32_t flags) {
    return bdi_attention_alloc_with_hint(mm, size, flags, 0.5f, BDI_ACCESS_RANDOM);
}

void* bdi_attention_alloc_with_hint(bdi_attention_mm_t* mm, size_t size, 
                                   uint32_t flags, float initial_attention,
                                   bdi_access_pattern_t pattern) {
    if (!mm || size == 0) return NULL;
    
    // Find free page entry
    bdi_page_entry_t* entry = NULL;
    for (uint32_t i = 0; i < BDI_MM_MAX_PAGES; i++) {
        if (!mm->pages[i].in_use) {
            entry = &mm->pages[i];
            break;
        }
    }
    
    if (!entry) {
        // Try to evict a page
        // For now, just fail
        return NULL;
    }
    
    // Allocate memory (using system malloc for now)
    void* addr = malloc(size);
    if (!addr) return NULL;
    
    // Initialize page entry
    entry->addr = addr;
    entry->size = size;
    entry->in_use = true;
    
    // Initialize metadata
    bdi_page_meta_t* meta = &entry->meta;
    memset(meta, 0, sizeof(*meta));
    meta->attention = fmaxf(0.0f, fminf(1.0f, initial_attention));
    meta->recency = 1.0f;
    meta->hotness = 0.1f;
    meta->criticality = (flags & BDI_PAGE_CRITICAL) ? 1.0f : 0.0f;
    meta->flags = (uint8_t)flags;
    meta->access_pattern = (uint8_t)pattern;
    meta->alloc_time = mm->update_counter;
    meta->last_access = mm->update_counter;
    meta->ref_count = 1;
    
    // Add to hash table
    add_page_entry(mm, entry);
    
    // Update statistics
    mm->page_count++;
    mm->allocated_pages++;
    mm->stats.total_allocations++;
    mm->stats.bytes_allocated += size;
    
    // Update pool statistics
    bdi_memory_pool_info_t* pool = &mm->pools[BDI_POOL_NORMAL];
    pool->alloc_count++;
    pool->bytes_allocated += size;
    pool->allocated_size += size;
    pool->available_size -= size;
    
    return addr;
}

void bdi_attention_free(bdi_attention_mm_t* mm, void* ptr) {
    if (!mm || !ptr) return;
    
    bdi_page_entry_t* entry = find_page_entry(mm, ptr);
    if (!entry || !entry->in_use) return;
    
    // Remove from hash table
    remove_page_entry(mm, entry);
    
    // Update statistics
    mm->stats.total_frees++;
    mm->stats.bytes_freed += entry->size;
    
    // Update pool statistics
    bdi_memory_pool_info_t* pool = &mm->pools[BDI_POOL_NORMAL];
    pool->free_count++;
    pool->bytes_freed += entry->size;
    pool->allocated_size -= entry->size;
    pool->available_size += entry->size;
    
    // Free memory
    free(entry->addr);
    
    // Clear entry
    memset(entry, 0, sizeof(*entry));
    mm->allocated_pages--;
}

// ===================================================================
// Attention Score Management
// ===================================================================

bool bdi_set_attention_score(bdi_attention_mm_t* mm, void* ptr, float attention) {
    if (!mm || !ptr) return false;
    
    bdi_page_entry_t* entry = find_page_entry(mm, ptr);
    if (!entry || !entry->in_use) return false;
    
    entry->meta.attention = fmaxf(0.0f, fminf(1.0f, attention));
    return true;
}

float bdi_get_attention_score(bdi_attention_mm_t* mm, void* ptr) {
    if (!mm || !ptr) return 0.0f;
    
    bdi_page_entry_t* entry = find_page_entry(mm, ptr);
    if (!entry || !entry->in_use) return 0.0f;
    
    return entry->meta.attention;
}

bool bdi_update_attention_hint(bdi_attention_mm_t* mm, void* ptr, float signal) {
    if (!mm || !ptr) return false;
    
    bdi_page_entry_t* entry = find_page_entry(mm, ptr);
    if (!entry || !entry->in_use) return false;
    
    update_attention_score(&entry->meta, signal, &mm->config);
    mm->stats.attention_updates++;
    
    return true;
}

// ===================================================================
// Memory Access Tracking
// ===================================================================

void bdi_track_memory_access(bdi_attention_mm_t* mm, void* ptr, bool is_write) {
    if (!mm || !ptr) return;
    
    bdi_page_entry_t* entry = find_page_entry(mm, ptr);
    if (!entry || !entry->in_use) return;
    
    // Update recency
    entry->meta.recency = 1.0f;
    entry->meta.last_access = mm->update_counter;
    entry->meta.access_count++;
    
    // Update attention based on access
    float signal = is_write ? 1.2f : 1.0f;
    update_attention_score(&entry->meta, signal, &mm->config);
    
    mm->stats.cache_hits++; // Assume hit for now
}

void bdi_track_cache_miss(bdi_attention_mm_t* mm, void* ptr) {
    if (!mm || !ptr) return;
    
    bdi_page_entry_t* entry = find_page_entry(mm, ptr);
    if (!entry || !entry->in_use) return;
    
    entry->meta.cache_misses++;
    mm->stats.cache_misses++;
    
    // Cache miss indicates lower attention
    update_attention_score(&entry->meta, 0.5f, &mm->config);
}

void bdi_track_tlb_miss(bdi_attention_mm_t* mm, void* ptr) {
    if (!mm || !ptr) return;
    
    bdi_page_entry_t* entry = find_page_entry(mm, ptr);
    if (!entry || !entry->in_use) return;
    
    entry->meta.tlb_misses++;
    mm->stats.tlb_misses++;
}

// ===================================================================
// Attention Learning and Updates
// ===================================================================

void bdi_attention_tick(bdi_attention_mm_t* mm) {
    if (!mm) return;
    
    mm->update_counter++;
    
    // Update attention scores for all pages
    for (uint32_t i = 0; i < mm->page_count; i++) {
        if (!mm->pages[i].in_use) continue;
        
        bdi_page_meta_t* meta = &mm->pages[i].meta;
        
        // Decay recency over time
        meta->recency *= mm->config.recency_decay_rate;
        
        // Decay hotness if no recent access
        if (mm->update_counter - meta->last_access > mm->config.update_frequency) {
            meta->hotness *= 0.99f;
        }
        
        // Update attention based on current state
        float signal = meta->hotness * meta->recency;
        update_attention_score(meta, signal, &mm->config);
    }
    
    // Garbage collection
    if (mm->update_counter - mm->last_gc_time > mm->config.gc_frequency) {
        // Simple GC: find pages with very low attention for potential eviction
        for (uint32_t i = 0; i < mm->page_count; i++) {
            if (!mm->pages[i].in_use) continue;
            
            float score = compute_eviction_score(&mm->pages[i].meta, &mm->config);
            if (score < mm->config.eviction_threshold) {
                // Mark for potential eviction
                mm->pages[i].meta.flags |= 0x80; // Eviction candidate flag
            }
        }
        
        mm->last_gc_time = mm->update_counter;
    }
}

// ===================================================================
// Batch Updates
// ===================================================================

void bdi_batch_attention_update_start(bdi_attention_mm_t* mm) {
    if (!mm) return;
    mm->batch_update_active = true;
    mm->batch_count = 0;
}

void bdi_batch_attention_update_add(bdi_attention_mm_t* mm, void* ptr, float signal) {
    if (!mm || !mm->batch_update_active || mm->batch_count >= 256) return;
    
    mm->batch_updates[mm->batch_count].ptr = ptr;
    mm->batch_updates[mm->batch_count].signal = signal;
    mm->batch_count++;
}

void bdi_batch_attention_update_commit(bdi_attention_mm_t* mm) {
    if (!mm || !mm->batch_update_active) return;
    
    for (uint32_t i = 0; i < mm->batch_count; i++) {
        bdi_update_attention_hint(mm, mm->batch_updates[i].ptr, mm->batch_updates[i].signal);
    }
    
    mm->batch_update_active = false;
    mm->batch_count = 0;
}

// ===================================================================
// Statistics and Monitoring
// ===================================================================

void bdi_get_attention_mm_stats(bdi_attention_mm_t* mm, bdi_attention_mm_stats_t* stats) {
    if (!mm || !stats) return;
    
    *stats = mm->stats;
    
    // Update pool statistics
    for (int i = 0; i < BDI_POOL_COUNT; i++) {
        stats->pools[i] = mm->pools[i];
    }
    
    // Compute attention statistics
    float total_attention = 0.0f;
    float max_attention = 0.0f;
    float min_attention = 1.0f;
    uint32_t active_pages = 0;
    
    for (uint32_t i = 0; i < mm->page_count; i++) {
        if (!mm->pages[i].in_use) continue;
        
        float attention = mm->pages[i].meta.attention;
        total_attention += attention;
        max_attention = fmaxf(max_attention, attention);
        min_attention = fminf(min_attention, attention);
        active_pages++;
    }
    
    if (active_pages > 0) {
        stats->avg_attention_score = total_attention / active_pages;
        stats->max_attention_score = max_attention;
        stats->min_attention_score = min_attention;
    }
    
    stats->peak_memory_usage = mm->allocated_pages * BDI_MM_PAGE_SIZE;
}

void bdi_print_attention_mm_stats(bdi_attention_mm_t* mm) {
    if (!mm) return;
    
    bdi_attention_mm_stats_t stats;
    bdi_get_attention_mm_stats(mm, &stats);
    
    printf("\n=== BDI Attention Memory Manager Statistics ===\n");
    printf("Allocation Statistics:\n");
    printf("  Total allocations: %lu\n", stats.total_allocations);
    printf("  Total frees: %lu\n", stats.total_frees);
    printf("  Bytes allocated: %lu\n", stats.bytes_allocated);
    printf("  Bytes freed: %lu\n", stats.bytes_freed);
    printf("  Peak memory usage: %lu bytes\n", stats.peak_memory_usage);
    
    printf("\nAttention Statistics:\n");
    printf("  Average attention: %.3f\n", stats.avg_attention_score);
    printf("  Max attention: %.3f\n", stats.max_attention_score);
    printf("  Min attention: %.3f\n", stats.min_attention_score);
    printf("  Attention updates: %lu\n", stats.attention_updates);
    
    printf("\nPerformance Statistics:\n");
    printf("  Cache hits: %lu\n", stats.cache_hits);
    printf("  Cache misses: %lu\n", stats.cache_misses);
    printf("  TLB hits: %lu\n", stats.tlb_hits);
    printf("  TLB misses: %lu\n", stats.tlb_misses);
    printf("  NUMA migrations: %lu\n", stats.numa_migrations);
    
    printf("\nMemory Pool Statistics:\n");
    for (int i = 0; i < BDI_POOL_COUNT; i++) {
        if (stats.pools[i].total_size > 0) {
            printf("  Pool %d: %lu/%lu MB allocated\n", i,
                   stats.pools[i].allocated_size / (1024*1024),
                   stats.pools[i].total_size / (1024*1024));
        }
    }
    printf("===============================================\n");
}

// ===================================================================
// Stub implementations for advanced features
// ===================================================================

void* bdi_attention_alloc_on_node(bdi_attention_mm_t* mm, size_t size, 
                                 uint32_t flags, uint32_t numa_node) {
    // For now, ignore NUMA node and use regular allocation
    return bdi_attention_alloc(mm, size, flags);
}

bool bdi_migrate_to_node(bdi_attention_mm_t* mm, void* ptr, uint32_t target_node) {
    // NUMA migration not implemented yet
    return false;
}

uint32_t bdi_get_memory_node(bdi_attention_mm_t* mm, void* ptr) {
    // Return default node
    return 0;
}

bool bdi_register_memory_pool(bdi_attention_mm_t* mm, const bdi_memory_pool_info_t* pool) {
    // Pool registration not fully implemented
    return false;
}

bool bdi_promote_memory(bdi_attention_mm_t* mm, void* ptr, bdi_memory_pool_t target_pool) {
    // Memory promotion not implemented yet
    return false;
}

bool bdi_demote_memory(bdi_attention_mm_t* mm, void* ptr, bdi_memory_pool_t target_pool) {
    // Memory demotion not implemented yet
    return false;
}
