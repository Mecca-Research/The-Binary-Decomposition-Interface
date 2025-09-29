
/**
 * @file x86_atomic_ops.h
 * @brief x86 Atomic Operations and Memory Fences with Proper Ordering
 * 
 * Phase 2 Master Memory Manager - Advanced x86 Systems
 * Complete atomic operations and memory fences with proper ordering semantics
 */

#ifndef X86_ATOMIC_OPS_H
#define X86_ATOMIC_OPS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Memory ordering constants
typedef enum {
    MEMORY_ORDER_RELAXED = 0,
    MEMORY_ORDER_CONSUME,
    MEMORY_ORDER_ACQUIRE,
    MEMORY_ORDER_RELEASE,
    MEMORY_ORDER_ACQ_REL,
    MEMORY_ORDER_SEQ_CST
} memory_order_t;

// Atomic data types
typedef struct { volatile uint8_t value; } atomic_uint8_t;
typedef struct { volatile uint16_t value; } atomic_uint16_t;
typedef struct { volatile uint32_t value; } atomic_uint32_t;
typedef struct { volatile uint64_t value; } atomic_uint64_t;
typedef struct { volatile uintptr_t value; } atomic_uintptr_t;
typedef struct { volatile bool value; } atomic_bool_t;

// Spinlock structure
typedef struct {
    atomic_uint32_t lock;
    uint32_t owner_cpu;
    uint64_t acquire_time;
    const char* file;
    int line;
} spinlock_t;

// Read-Write lock structure
typedef struct {
    atomic_uint32_t readers;
    atomic_uint32_t writers;
    atomic_uint32_t write_waiters;
    spinlock_t write_lock;
} rwlock_t;

// Lock-free queue node
typedef struct atomic_queue_node {
    volatile struct atomic_queue_node* next;
    void* data;
} atomic_queue_node_t;

// Lock-free queue
typedef struct {
    volatile atomic_queue_node_t* head;
    volatile atomic_queue_node_t* tail;
    atomic_uint64_t enqueue_count;
    atomic_uint64_t dequeue_count;
} atomic_queue_t;

// Memory fence operations
void x86_memory_fence_full(void);
void x86_memory_fence_load(void);
void x86_memory_fence_store(void);
void x86_memory_fence_acquire(void);
void x86_memory_fence_release(void);
void x86_compiler_barrier(void);

// 8-bit atomic operations
uint8_t x86_atomic_load_uint8(const atomic_uint8_t* obj, memory_order_t order);
void x86_atomic_store_uint8(atomic_uint8_t* obj, uint8_t value, memory_order_t order);
uint8_t x86_atomic_exchange_uint8(atomic_uint8_t* obj, uint8_t value, memory_order_t order);
bool x86_atomic_compare_exchange_uint8(atomic_uint8_t* obj, uint8_t* expected, 
                                      uint8_t desired, memory_order_t order);
uint8_t x86_atomic_fetch_add_uint8(atomic_uint8_t* obj, uint8_t value, memory_order_t order);
uint8_t x86_atomic_fetch_sub_uint8(atomic_uint8_t* obj, uint8_t value, memory_order_t order);
uint8_t x86_atomic_fetch_and_uint8(atomic_uint8_t* obj, uint8_t value, memory_order_t order);
uint8_t x86_atomic_fetch_or_uint8(atomic_uint8_t* obj, uint8_t value, memory_order_t order);
uint8_t x86_atomic_fetch_xor_uint8(atomic_uint8_t* obj, uint8_t value, memory_order_t order);

// 16-bit atomic operations
uint16_t x86_atomic_load_uint16(const atomic_uint16_t* obj, memory_order_t order);
void x86_atomic_store_uint16(atomic_uint16_t* obj, uint16_t value, memory_order_t order);
uint16_t x86_atomic_exchange_uint16(atomic_uint16_t* obj, uint16_t value, memory_order_t order);
bool x86_atomic_compare_exchange_uint16(atomic_uint16_t* obj, uint16_t* expected, 
                                       uint16_t desired, memory_order_t order);
uint16_t x86_atomic_fetch_add_uint16(atomic_uint16_t* obj, uint16_t value, memory_order_t order);
uint16_t x86_atomic_fetch_sub_uint16(atomic_uint16_t* obj, uint16_t value, memory_order_t order);
uint16_t x86_atomic_fetch_and_uint16(atomic_uint16_t* obj, uint16_t value, memory_order_t order);
uint16_t x86_atomic_fetch_or_uint16(atomic_uint16_t* obj, uint16_t value, memory_order_t order);
uint16_t x86_atomic_fetch_xor_uint16(atomic_uint16_t* obj, uint16_t value, memory_order_t order);

// 32-bit atomic operations
uint32_t x86_atomic_load_uint32(const atomic_uint32_t* obj, memory_order_t order);
void x86_atomic_store_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order);
uint32_t x86_atomic_exchange_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order);
bool x86_atomic_compare_exchange_uint32(atomic_uint32_t* obj, uint32_t* expected, 
                                       uint32_t desired, memory_order_t order);
uint32_t x86_atomic_fetch_add_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order);
uint32_t x86_atomic_fetch_sub_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order);
uint32_t x86_atomic_fetch_and_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order);
uint32_t x86_atomic_fetch_or_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order);
uint32_t x86_atomic_fetch_xor_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order);

// 64-bit atomic operations
uint64_t x86_atomic_load_uint64(const atomic_uint64_t* obj, memory_order_t order);
void x86_atomic_store_uint64(atomic_uint64_t* obj, uint64_t value, memory_order_t order);
uint64_t x86_atomic_exchange_uint64(atomic_uint64_t* obj, uint64_t value, memory_order_t order);
bool x86_atomic_compare_exchange_uint64(atomic_uint64_t* obj, uint64_t* expected, 
                                       uint64_t desired, memory_order_t order);
uint64_t x86_atomic_fetch_add_uint64(atomic_uint64_t* obj, uint64_t value, memory_order_t order);
uint64_t x86_atomic_fetch_sub_uint64(atomic_uint64_t* obj, uint64_t value, memory_order_t order);
uint64_t x86_atomic_fetch_and_uint64(atomic_uint64_t* obj, uint64_t value, memory_order_t order);
uint64_t x86_atomic_fetch_or_uint64(atomic_uint64_t* obj, uint64_t value, memory_order_t order);
uint64_t x86_atomic_fetch_xor_uint64(atomic_uint64_t* obj, uint64_t value, memory_order_t order);

// Pointer atomic operations
void* x86_atomic_load_ptr(const atomic_uintptr_t* obj, memory_order_t order);
void x86_atomic_store_ptr(atomic_uintptr_t* obj, void* value, memory_order_t order);
void* x86_atomic_exchange_ptr(atomic_uintptr_t* obj, void* value, memory_order_t order);
bool x86_atomic_compare_exchange_ptr(atomic_uintptr_t* obj, void** expected, 
                                    void* desired, memory_order_t order);

// Boolean atomic operations
bool x86_atomic_load_bool(const atomic_bool_t* obj, memory_order_t order);
void x86_atomic_store_bool(atomic_bool_t* obj, bool value, memory_order_t order);
bool x86_atomic_exchange_bool(atomic_bool_t* obj, bool value, memory_order_t order);
bool x86_atomic_compare_exchange_bool(atomic_bool_t* obj, bool* expected, 
                                     bool desired, memory_order_t order);

// Double-width compare-and-swap (CMPXCHG16B)
bool x86_atomic_compare_exchange_128(volatile uint64_t* obj, uint64_t* expected_low, 
                                    uint64_t* expected_high, uint64_t desired_low, 
                                    uint64_t desired_high);

// Spinlock operations
void x86_spinlock_init(spinlock_t* lock);
void x86_spinlock_lock(spinlock_t* lock);
bool x86_spinlock_trylock(spinlock_t* lock);
void x86_spinlock_unlock(spinlock_t* lock);
bool x86_spinlock_is_locked(const spinlock_t* lock);

// Spinlock with debugging
#define x86_spinlock_lock_debug(lock) \
    x86_spinlock_lock_with_debug(lock, __FILE__, __LINE__)
void x86_spinlock_lock_with_debug(spinlock_t* lock, const char* file, int line);

// Read-Write lock operations
void x86_rwlock_init(rwlock_t* rwlock);
void x86_rwlock_read_lock(rwlock_t* rwlock);
bool x86_rwlock_read_trylock(rwlock_t* rwlock);
void x86_rwlock_read_unlock(rwlock_t* rwlock);
void x86_rwlock_write_lock(rwlock_t* rwlock);
bool x86_rwlock_write_trylock(rwlock_t* rwlock);
void x86_rwlock_write_unlock(rwlock_t* rwlock);

// Lock-free queue operations
void x86_atomic_queue_init(atomic_queue_t* queue);
void x86_atomic_queue_destroy(atomic_queue_t* queue);
bool x86_atomic_queue_enqueue(atomic_queue_t* queue, void* data);
bool x86_atomic_queue_dequeue(atomic_queue_t* queue, void** data);
bool x86_atomic_queue_is_empty(const atomic_queue_t* queue);
uint64_t x86_atomic_queue_size_estimate(const atomic_queue_t* queue);

// Memory ordering utilities
const char* x86_memory_order_to_string(memory_order_t order);
bool x86_is_memory_order_valid(memory_order_t order);
memory_order_t x86_memory_order_for_load(memory_order_t order);
memory_order_t x86_memory_order_for_store(memory_order_t order);

// CPU pause and backoff
void x86_cpu_pause(void);
void x86_cpu_relax(void);
void x86_backoff_delay(uint32_t iteration);
void x86_exponential_backoff(uint32_t* backoff_state);

// Cache line operations
#define X86_CACHE_LINE_SIZE 64
void x86_prefetch_read(const void* addr);
void x86_prefetch_write(void* addr);
void x86_clflush(const void* addr);
void x86_clflushopt(const void* addr);
void x86_clwb(const void* addr);

// Performance counters for atomic operations
typedef struct {
    uint64_t successful_cas;
    uint64_t failed_cas;
    uint64_t lock_acquisitions;
    uint64_t lock_contentions;
    uint64_t total_wait_cycles;
    uint64_t max_wait_cycles;
} atomic_perf_stats_t;

void x86_atomic_perf_init(atomic_perf_stats_t* stats);
void x86_atomic_perf_record_cas(atomic_perf_stats_t* stats, bool success);
void x86_atomic_perf_record_lock(atomic_perf_stats_t* stats, uint64_t wait_cycles);
void x86_atomic_perf_print(const atomic_perf_stats_t* stats);

// Hazard pointers for memory reclamation
#define MAX_HAZARD_POINTERS 16

typedef struct hazard_pointer {
    volatile void* pointer;
    struct hazard_pointer* next;
} hazard_pointer_t;

typedef struct {
    hazard_pointer_t hazards[MAX_HAZARD_POINTERS];
    volatile hazard_pointer_t* free_list;
    atomic_uint32_t active_count;
} hazard_pointer_domain_t;

void x86_hazard_pointer_init(hazard_pointer_domain_t* domain);
hazard_pointer_t* x86_hazard_pointer_acquire(hazard_pointer_domain_t* domain);
void x86_hazard_pointer_release(hazard_pointer_domain_t* domain, hazard_pointer_t* hp);
void x86_hazard_pointer_set(hazard_pointer_t* hp, const void* pointer);
void x86_hazard_pointer_clear(hazard_pointer_t* hp);
bool x86_hazard_pointer_is_protected(hazard_pointer_domain_t* domain, const void* pointer);

// Atomic reference counting
typedef struct {
    atomic_uint32_t count;
    void (*destructor)(void* obj);
} atomic_refcount_t;

void x86_atomic_refcount_init(atomic_refcount_t* refcount, void (*destructor)(void*));
void x86_atomic_refcount_get(atomic_refcount_t* refcount);
bool x86_atomic_refcount_put(atomic_refcount_t* refcount, void* obj);
uint32_t x86_atomic_refcount_read(const atomic_refcount_t* refcount);

// Wait-free algorithms
typedef struct {
    atomic_uintptr_t data[16]; // Circular buffer
    atomic_uint64_t head;
    atomic_uint64_t tail;
} wait_free_ring_buffer_t;

void x86_wait_free_ring_init(wait_free_ring_buffer_t* ring);
bool x86_wait_free_ring_push(wait_free_ring_buffer_t* ring, void* data);
bool x86_wait_free_ring_pop(wait_free_ring_buffer_t* ring, void** data);
bool x86_wait_free_ring_is_empty(const wait_free_ring_buffer_t* ring);
bool x86_wait_free_ring_is_full(const wait_free_ring_buffer_t* ring);

// Debugging and diagnostics
void x86_atomic_dump_memory_state(const void* addr, size_t size);
void x86_atomic_check_alignment(const void* addr, size_t alignment);
bool x86_atomic_is_lock_free_size(size_t size);
void x86_atomic_test_memory_ordering(void);

// Utility macros
#define X86_ATOMIC_INIT(value) { .value = (value) }
#define X86_ATOMIC_VAR_INIT(value) X86_ATOMIC_INIT(value)

// Compiler-specific optimizations
#ifdef __GNUC__
#define X86_ATOMIC_ALWAYS_INLINE __attribute__((always_inline)) inline
#define X86_ATOMIC_NOINLINE __attribute__((noinline))
#define X86_ATOMIC_LIKELY(x) __builtin_expect(!!(x), 1)
#define X86_ATOMIC_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define X86_ATOMIC_ALWAYS_INLINE inline
#define X86_ATOMIC_NOINLINE
#define X86_ATOMIC_LIKELY(x) (x)
#define X86_ATOMIC_UNLIKELY(x) (x)
#endif

#ifdef __cplusplus
}
#endif

#endif // X86_ATOMIC_OPS_H
