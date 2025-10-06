
/*
 * HAM Allocator Metrics and Statistics
 */

#ifndef BDI_HAM_METRICS_H
#define BDI_HAM_METRICS_H

#include <stdint.h>
#include <stdbool.h>

// Per-arena HAM statistics
typedef struct {
    uint32_t arena_id;
    uint64_t total_allocations;
    uint64_t total_deallocations;
    uint64_t active_allocations;
    uint64_t bytes_allocated;
    uint64_t bytes_freed;
    uint64_t bytes_in_use;
    uint64_t cache_hits;
    uint64_t cache_misses;
    double cache_hit_rate;
    uint32_t fragmentation_percent;
} ham_arena_stats_t;

// Global HAM statistics
typedef struct {
    uint64_t total_allocations;
    uint64_t total_deallocations;
    uint64_t failed_allocations;
    uint64_t total_bytes_allocated;
    uint64_t total_bytes_freed;
    uint64_t peak_memory_usage;
    uint64_t current_memory_usage;
    double avg_allocation_size;
    double fragmentation_ratio;
    uint32_t active_arenas;
} ham_global_stats_t;

// NUMA allocation statistics
typedef struct {
    uint32_t node_id;
    uint64_t local_allocations;
    uint64_t remote_allocations;
    uint64_t bytes_allocated;
    double locality_ratio;
} ham_numa_stats_t;

// Size class statistics
typedef struct {
    uint32_t size_class;
    uint64_t allocations;
    uint64_t deallocations;
    uint64_t active_blocks;
    uint32_t utilization_percent;
} ham_size_class_stats_t;

// API
void ham_metrics_init(void);
void ham_metrics_record_allocation(uint32_t arena_id, size_t size, bool success);
void ham_metrics_record_deallocation(uint32_t arena_id, size_t size);
void ham_metrics_record_cache_hit(uint32_t arena_id);
void ham_metrics_record_cache_miss(uint32_t arena_id);
void ham_metrics_record_numa_allocation(uint32_t node_id, bool local);

ham_arena_stats_t ham_metrics_get_arena(uint32_t arena_id);
ham_global_stats_t ham_metrics_get_global(void);
ham_numa_stats_t ham_metrics_get_numa(uint32_t node_id);

void ham_metrics_print_report(void);
void ham_metrics_export_json(const char *filename);

#endif // BDI_HAM_METRICS_H
