
// ===================================================================
// BDI Attention-Based Memory Manager
// Multi-objective allocation with learned priorities
// ===================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// Attention Memory Metadata
// ===================================================================

typedef struct {
    // Attention scores (0.0 to 1.0)
    float attention;        // Learned priority/importance
    float recency;          // How recently accessed (decays over time)
    float hotness;          // Access frequency/intensity
    float criticality;      // System-assigned importance
    
    // NUMA and locality information
    uint8_t numa_node;      // Preferred NUMA node
    uint8_t last_cpu;       // Last CPU that accessed this memory
    uint16_t numa_distance; // Distance from current CPU
    
    // Memory characteristics
    uint8_t flags;          // Memory flags (see BDI_PAGE_* below)
    uint8_t access_pattern; // Access pattern hint
    uint16_t ref_count;     // Reference count
    
    // Temporal information
    uint64_t alloc_time;    // When allocated (in ticks)
    uint64_t last_access;   // Last access time
    uint64_t access_count;  // Total number of accesses
    
    // Performance tracking
    uint32_t cache_misses;  // Estimated cache misses
    uint32_t tlb_misses;    // Estimated TLB misses
    float bandwidth_usage;  // Memory bandwidth utilization
    
} bdi_page_meta_t;

// Memory flags
#define BDI_PAGE_PINNED         (1 << 0)  // Cannot be swapped
#define BDI_PAGE_CRITICAL       (1 << 1)  // Critical system memory
#define BDI_PAGE_ENCRYPTED      (1 << 2)  // Memory is encrypted
#define BDI_PAGE_SHARED         (1 << 3)  // Shared between processes
#define BDI_PAGE_EXECUTABLE     (1 << 4)  // Contains executable code
#define BDI_PAGE_DEVICE         (1 << 5)  // Device memory (GPU, etc.)
#define BDI_PAGE_HUGEPAGE       (1 << 6)  // Large page (2MB/1GB)
#define BDI_PAGE_PREFAULTED     (1 << 7)  // Pre-faulted for performance

// Access pattern hints
typedef enum {
    BDI_ACCESS_RANDOM = 0,      // Random access pattern
    BDI_ACCESS_SEQUENTIAL,      // Sequential access
    BDI_ACCESS_TEMPORAL,        // High temporal locality
    BDI_ACCESS_SPATIAL,         // High spatial locality
    BDI_ACCESS_STREAMING,       // Streaming (use once)
    BDI_ACCESS_GRAPH_TRAVERSE,  // Graph traversal pattern
    BDI_ACCESS_MATRIX_MULT,     // Matrix multiplication pattern
    BDI_ACCESS_REDUCTION        // Reduction operation pattern
} bdi_access_pattern_t;

// ===================================================================
// Attention Memory Manager Configuration
// ===================================================================

typedef struct {
    // Learning parameters
    float attention_learning_rate;      // EMA learning rate for attention
    float recency_decay_rate;          // Decay rate for recency
    float hotness_learning_rate;       // Learning rate for hotness
    float regularization_factor;       // Regularization to prevent overfitting
    
    // Allocation weights
    float weight_attention;            // Weight for attention score
    float weight_recency;              // Weight for recency
    float weight_hotness;              // Weight for hotness  
    float weight_numa_locality;       // Weight for NUMA locality
    float weight_criticality;          // Weight for criticality
    
    // Memory management thresholds
    float eviction_threshold;          // Threshold for eviction decisions
    float promotion_threshold;         // Threshold for promotion to faster memory
    float demotion_threshold;          // Threshold for demotion to slower memory
    
    // Performance tuning
    uint32_t update_frequency;         // How often to update attention scores
    uint32_t gc_frequency;             // Garbage collection frequency
    bool enable_prefetching;           // Enable predictive prefetching
    bool enable_numa_balancing;        // Enable NUMA-aware balancing
    
} bdi_attention_config_t;

// ===================================================================
// Memory Pool Management
// ===================================================================

typedef enum {
    BDI_POOL_FAST = 0,      // Fast memory (L3 cache, HBM)
    BDI_POOL_NORMAL,        // Normal DRAM
    BDI_POOL_SLOW,          // Slow memory (swap, storage-class memory)
    BDI_POOL_DEVICE,        // Device memory (GPU, FPGA)
    BDI_POOL_COUNT
} bdi_memory_pool_t;

typedef struct {
    bdi_memory_pool_t pool_type;
    void* base_addr;                   // Base address of pool
    size_t total_size;                 // Total size of pool
    size_t available_size;             // Available size
    size_t allocated_size;             // Currently allocated
    
    // Performance characteristics
    uint32_t latency_ns;               // Access latency (nanoseconds)
    uint32_t bandwidth_gbps;           // Bandwidth (GB/s)
    float power_per_gb;                // Power consumption per GB
    
    // NUMA information
    uint32_t numa_node;                // NUMA node for this pool
    uint32_t cpu_affinity_mask;        // CPUs with best access to this pool
    
    // Pool statistics
    uint64_t alloc_count;              // Number of allocations
    uint64_t free_count;               // Number of frees
    uint64_t bytes_allocated;          // Total bytes allocated
    uint64_t bytes_freed;              // Total bytes freed
    
} bdi_memory_pool_info_t;

// ===================================================================
// Attention Memory Manager Interface
// ===================================================================

typedef struct bdi_attention_mm bdi_attention_mm_t;

// Initialize attention-based memory manager
bdi_attention_mm_t* bdi_attention_mm_create(const bdi_attention_config_t* config);
void bdi_attention_mm_destroy(bdi_attention_mm_t* mm);

// Memory allocation with attention hints
void* bdi_attention_alloc(bdi_attention_mm_t* mm, size_t size, uint32_t flags);
void* bdi_attention_alloc_with_hint(bdi_attention_mm_t* mm, size_t size, 
                                   uint32_t flags, float initial_attention,
                                   bdi_access_pattern_t pattern);
void bdi_attention_free(bdi_attention_mm_t* mm, void* ptr);

// Attention score management
bool bdi_set_attention_score(bdi_attention_mm_t* mm, void* ptr, float attention);
float bdi_get_attention_score(bdi_attention_mm_t* mm, void* ptr);
bool bdi_update_attention_hint(bdi_attention_mm_t* mm, void* ptr, float signal);

// Memory access tracking
void bdi_track_memory_access(bdi_attention_mm_t* mm, void* ptr, bool is_write);
void bdi_track_cache_miss(bdi_attention_mm_t* mm, void* ptr);
void bdi_track_tlb_miss(bdi_attention_mm_t* mm, void* ptr);

// NUMA-aware operations
void* bdi_attention_alloc_on_node(bdi_attention_mm_t* mm, size_t size, 
                                 uint32_t flags, uint32_t numa_node);
bool bdi_migrate_to_node(bdi_attention_mm_t* mm, void* ptr, uint32_t target_node);
uint32_t bdi_get_memory_node(bdi_attention_mm_t* mm, void* ptr);

// Memory pool management
bool bdi_register_memory_pool(bdi_attention_mm_t* mm, const bdi_memory_pool_info_t* pool);
bool bdi_promote_memory(bdi_attention_mm_t* mm, void* ptr, bdi_memory_pool_t target_pool);
bool bdi_demote_memory(bdi_attention_mm_t* mm, void* ptr, bdi_memory_pool_t target_pool);

// ===================================================================
// Attention Learning & Updates
// ===================================================================

// Update attention scores based on access patterns
void bdi_attention_tick(bdi_attention_mm_t* mm);

// Manual attention score updates
void bdi_boost_attention(bdi_attention_mm_t* mm, void* ptr, float boost);
void bdi_decay_attention(bdi_attention_mm_t* mm, void* ptr, float decay_rate);

// Batch attention updates for performance
void bdi_batch_attention_update_start(bdi_attention_mm_t* mm);
void bdi_batch_attention_update_add(bdi_attention_mm_t* mm, void* ptr, float signal);
void bdi_batch_attention_update_commit(bdi_attention_mm_t* mm);

// ===================================================================
// Memory Management Policies
// ===================================================================

// Eviction policy based on attention scores
typedef struct {
    float (*score_function)(const bdi_page_meta_t* meta);
    bool (*should_evict)(const bdi_page_meta_t* meta, float threshold);
    void (*on_eviction)(void* ptr, const bdi_page_meta_t* meta);
} bdi_eviction_policy_t;

// Set custom eviction policy
bool bdi_set_eviction_policy(bdi_attention_mm_t* mm, const bdi_eviction_policy_t* policy);

// Built-in eviction policies
extern const bdi_eviction_policy_t bdi_eviction_policy_lru;
extern const bdi_eviction_policy_t bdi_eviction_policy_attention_weighted;
extern const bdi_eviction_policy_t bdi_eviction_policy_multi_objective;

// ===================================================================
// Statistics & Monitoring
// ===================================================================

typedef struct {
    // Allocation statistics
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t bytes_allocated;
    uint64_t bytes_freed;
    uint64_t peak_memory_usage;
    
    // Attention statistics
    float avg_attention_score;
    float max_attention_score;
    float min_attention_score;
    uint64_t attention_updates;
    
    // Performance statistics
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t tlb_hits;
    uint64_t tlb_misses;
    uint64_t numa_migrations;
    
    // Pool statistics
    bdi_memory_pool_info_t pools[BDI_POOL_COUNT];
    
} bdi_attention_mm_stats_t;

// Get memory manager statistics
void bdi_get_attention_mm_stats(bdi_attention_mm_t* mm, bdi_attention_mm_stats_t* stats);
void bdi_reset_attention_mm_stats(bdi_attention_mm_t* mm);

// Print statistics in human-readable format
void bdi_print_attention_mm_stats(bdi_attention_mm_t* mm);

// ===================================================================
// Integration with BDI Graph System
// ===================================================================

// Set attention hint for graph node buffers
bool bdi_set_graph_buffer_attention(bdi_attention_mm_t* mm, void* buffer, 
                                   uint32_t node_id, float graph_importance);

// Update attention based on graph execution patterns
void bdi_update_graph_attention(bdi_attention_mm_t* mm, uint32_t node_id, 
                               float execution_time, float memory_pressure);

// Prefetch memory for upcoming graph operations
bool bdi_prefetch_graph_buffers(bdi_attention_mm_t* mm, uint32_t* node_ids, 
                               uint32_t node_count);

#ifdef __cplusplus
}
#endif
