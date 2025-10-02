
// ===================================================================
// BDI Kernel - Memory Management Subsystem (Phase 2)
// C23 Enhanced Memory Allocator with NUMA Optimization
// ===================================================================
#ifndef BDI_MEMORY_H
#define BDI_MEMORY_H

#include "c23_compat.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// ===================================================================
// Memory Constants (Compile-time constants)
// ===================================================================

// Page sizes
#define PAGE_SIZE 4096
#define CACHE_LINE_SIZE 64
#define HUGE_PAGE_2MB (2 * 1024 * 1024)
#define HUGE_PAGE_1GB (1024 * 1024 * 1024)

// Memory alignment
#define MIN_ALIGNMENT 8
#define MAX_ALIGNMENT 4096

// Arena configuration
#define ARENA_SIZE (1024 * 1024)  // 1MB per CPU
#define MAX_ARENAS 256            // Max CPUs
#define ARENA_ALIGNMENT 64        // Cache-line aligned

// NUMA configuration
#define MAX_NUMA_NODES 8
#define INVALID_NUMA_NODE (-1)

// Memory flags
#define MEM_FLAG_ZERO 0x01      // Zero memory on allocation
#define MEM_FLAG_HUGE 0x02      // Use huge pages
#define MEM_FLAG_NUMA 0x04      // NUMA-aware allocation
#define MEM_FLAG_LOCKED 0x08    // Lock pages in memory

// ===================================================================
// Memory Structures (C23 Enhanced)
// ===================================================================

// Memory block descriptor (cache-line sized)
typedef struct MemoryBlock {
    void* base;                    // Base address
    size_t size;                   // Block size in bytes
    int numa_node;                 // NUMA node affinity
    uint32_t flags;                // Memory flags
    _Atomic uint32_t ref_count;    // Reference count
    uint8_t padding[36];           // Pad to 64 bytes
} MemoryBlock;

// Validate structure size at compile time
_Static_assert(sizeof(MemoryBlock) == 64, "MemoryBlock must be cache-line sized");
_Static_assert(_Alignof(MemoryBlock) >= 8, "MemoryBlock alignment requirement");

// Per-CPU arena structure (cache-line sized)
typedef struct CpuArena {
    void* base;                    // Arena base address (8 bytes)
    size_t size;                   // Total arena size (8 bytes)
    size_t used;                   // Used bytes (8 bytes)
    int numa_node;                 // NUMA node affinity (4 bytes)
    int cpu_id;                    // CPU ID (4 bytes)
    _Atomic size_t allocs;         // Allocation count (8 bytes)
    _Atomic size_t frees;          // Free count (8 bytes)
    _Atomic size_t bytes_allocated;// Total bytes allocated (8 bytes)
    uint8_t padding[8];            // Pad to 64 bytes (8 bytes)
} CpuArena;

_Static_assert(sizeof(CpuArena) == 64, "CpuArena must be cache-line sized");
_Static_assert(_Alignof(CpuArena) >= 8, "CpuArena alignment requirement");

// Memory statistics
typedef struct MemoryStats {
    _Atomic size_t total_allocated;
    _Atomic size_t total_freed;
    _Atomic size_t current_usage;
    _Atomic size_t peak_usage;
    _Atomic size_t numa_local_allocs;
    _Atomic size_t numa_remote_allocs;
    _Atomic size_t huge_page_allocs;
    _Atomic size_t arena_allocs;
} MemoryStats;

// ===================================================================
// Core Memory Functions (C23 [[nodiscard]])
// ===================================================================

// Memory subsystem initialization
NODISCARD int memory_init(void);
void memory_shutdown(void);

// Core allocation functions
NODISCARD void* alloc_memory(size_t size, size_t alignment);
NODISCARD void* alloc_memory_flags(size_t size, size_t alignment, uint32_t flags);
NODISCARD int free_memory(void* ptr, size_t size);

// NUMA-aware allocation
NODISCARD void* alloc_memory_numa(size_t size, size_t alignment, int node);
NODISCARD void* alloc_memory_local(size_t size, size_t alignment);

// Memory statistics
NODISCARD MemoryStats memory_get_stats(void);
void memory_print_stats(void);

// ===================================================================
// NUMA Functions (Phase 2)
// ===================================================================

NODISCARD int numa_init(void);
NODISCARD int numa_num_nodes(void);
NODISCARD int numa_current_node(void);
NODISCARD int numa_cpu_to_node(int cpu);
NODISCARD void* numa_alloc_onnode(size_t size, int node);
NODISCARD void* numa_alloc_local(size_t size);
NODISCARD int numa_free(void* ptr, size_t size);

// ===================================================================
// Per-CPU Arena Functions (Phase 2)
// ===================================================================

NODISCARD int arena_init(void);
void arena_shutdown(void);
NODISCARD void* arena_alloc(size_t size);
NODISCARD int arena_free(void* ptr);
NODISCARD CpuArena* arena_get_current(void);

// ===================================================================
// Huge Page Functions (Phase 2)
// ===================================================================

NODISCARD void* alloc_huge_page(size_t size, int numa_node);
NODISCARD int free_huge_page(void* ptr, size_t size);
NODISCARD bool is_huge_page_aligned(void* ptr, size_t page_size);

// ===================================================================
// Type-Safe Allocation Macros (C23 typeof)
// ===================================================================

// Allocate single object
#define ALLOC(type) \
    ((type*)alloc_memory(sizeof(type), _Alignof(type)))

// Allocate array of objects
#define ALLOC_ARRAY(type, count) \
    ((type*)alloc_memory(sizeof(type) * (count), _Alignof(type)))

// Allocate on specific NUMA node
#define ALLOC_NUMA(type, node) \
    ((type*)alloc_memory_numa(sizeof(type), _Alignof(type), node))

// Allocate array on specific NUMA node
#define ALLOC_ARRAY_NUMA(type, count, node) \
    ((type*)alloc_memory_numa(sizeof(type) * (count), _Alignof(type), node))

// Free single object
#define FREE(ptr, type) \
    free_memory((ptr), sizeof(type))

// Free array of objects
#define FREE_ARRAY(ptr, type, count) \
    free_memory((ptr), sizeof(type) * (count))

// Allocate with flags
#define ALLOC_FLAGS(type, flags) \
    ((type*)alloc_memory_flags(sizeof(type), _Alignof(type), flags))

// ===================================================================
// Compile-Time Validations (Phase 2)
// ===================================================================

// Page size validations
_Static_assert(PAGE_SIZE == 4096, "Page size must be 4KB");
_Static_assert(HUGE_PAGE_2MB == 2 * 1024 * 1024, "2MB huge page size");
_Static_assert(HUGE_PAGE_1GB == 1024 * 1024 * 1024, "1GB huge page size");

// Alignment validations
_Static_assert(HUGE_PAGE_2MB % PAGE_SIZE == 0, "2MB must be 4KB aligned");
_Static_assert(HUGE_PAGE_1GB % HUGE_PAGE_2MB == 0, "1GB must be 2MB aligned");
_Static_assert(CACHE_LINE_SIZE == 64, "Cache line must be 64 bytes");

// Arena validations
_Static_assert(ARENA_SIZE >= PAGE_SIZE, "Arena must be at least one page");
_Static_assert(ARENA_SIZE % PAGE_SIZE == 0, "Arena must be page-aligned");
_Static_assert(MAX_ARENAS > 0 && MAX_ARENAS <= 1024, "Arena count limits");

// Alignment validations
_Static_assert(MIN_ALIGNMENT >= sizeof(void*), "Minimum alignment");
_Static_assert(MAX_ALIGNMENT <= PAGE_SIZE, "Maximum alignment");

#endif // BDI_MEMORY_H
