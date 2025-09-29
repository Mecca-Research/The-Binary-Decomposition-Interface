
/**
 * @file x86_atomic_ops.c
 * @brief x86 Atomic Operations and Memory Fences Implementation
 * 
 * Phase 2 Master Memory Manager - Advanced x86 Systems
 * Complete atomic operations and memory fences with proper ordering semantics
 */

#include "x86_atomic_ops.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global performance statistics
static atomic_perf_stats_t global_perf_stats = {0};

// Memory ordering names for debugging
static const char* memory_order_names[] = {
    "relaxed", "consume", "acquire", "release", "acq_rel", "seq_cst"
};

/**
 * @brief Full memory fence (MFENCE)
 */
void x86_memory_fence_full(void) {
    __asm__ volatile("mfence" ::: "memory");
}

/**
 * @brief Load fence (LFENCE)
 */
void x86_memory_fence_load(void) {
    __asm__ volatile("lfence" ::: "memory");
}

/**
 * @brief Store fence (SFENCE)
 */
void x86_memory_fence_store(void) {
    __asm__ volatile("sfence" ::: "memory");
}

/**
 * @brief Acquire fence
 */
void x86_memory_fence_acquire(void) {
    // On x86, acquire semantics are provided by the strong memory model
    // Only need compiler barrier for most cases
    __asm__ volatile("" ::: "memory");
}

/**
 * @brief Release fence
 */
void x86_memory_fence_release(void) {
    // On x86, release semantics are provided by the strong memory model
    // Only need compiler barrier for most cases
    __asm__ volatile("" ::: "memory");
}

/**
 * @brief Compiler barrier
 */
void x86_compiler_barrier(void) {
    __asm__ volatile("" ::: "memory");
}

/**
 * @brief Apply memory ordering fence
 */
static inline void apply_memory_order_fence(memory_order_t order) {
    switch (order) {
        case MEMORY_ORDER_RELAXED:
            // No fence needed
            break;
        case MEMORY_ORDER_CONSUME:
        case MEMORY_ORDER_ACQUIRE:
            x86_memory_fence_acquire();
            break;
        case MEMORY_ORDER_RELEASE:
            x86_memory_fence_release();
            break;
        case MEMORY_ORDER_ACQ_REL:
            x86_memory_fence_acquire();
            x86_memory_fence_release();
            break;
        case MEMORY_ORDER_SEQ_CST:
            x86_memory_fence_full();
            break;
    }
}

/**
 * @brief 32-bit atomic load
 */
uint32_t x86_atomic_load_uint32(const atomic_uint32_t* obj, memory_order_t order) {
    uint32_t result = obj->value;
    apply_memory_order_fence(order);
    return result;
}

/**
 * @brief 32-bit atomic store
 */
void x86_atomic_store_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order) {
    apply_memory_order_fence(order);
    obj->value = value;
    if (order == MEMORY_ORDER_SEQ_CST) {
        x86_memory_fence_full();
    }
}

/**
 * @brief 32-bit atomic exchange
 */
uint32_t x86_atomic_exchange_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order) {
    uint32_t result;
    __asm__ volatile(
        "xchgl %0, %1"
        : "=r"(result), "+m"(obj->value)
        : "0"(value)
        : "memory"
    );
    apply_memory_order_fence(order);
    return result;
}

/**
 * @brief 32-bit atomic compare-and-swap
 */
bool x86_atomic_compare_exchange_uint32(atomic_uint32_t* obj, uint32_t* expected, 
                                       uint32_t desired, memory_order_t order) {
    uint32_t prev = *expected;
    bool success;
    
    __asm__ volatile(
        "lock cmpxchgl %2, %1\n"
        "sete %0"
        : "=q"(success), "+m"(obj->value), "+a"(prev)
        : "r"(desired)
        : "memory"
    );
    
    *expected = prev;
    apply_memory_order_fence(order);
    
    // Update performance statistics
    x86_atomic_perf_record_cas(&global_perf_stats, success);
    
    return success;
}

/**
 * @brief 32-bit atomic fetch-and-add
 */
uint32_t x86_atomic_fetch_add_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order) {
    uint32_t result;
    __asm__ volatile(
        "lock xaddl %0, %1"
        : "=r"(result), "+m"(obj->value)
        : "0"(value)
        : "memory"
    );
    apply_memory_order_fence(order);
    return result;
}

/**
 * @brief 32-bit atomic fetch-and-sub
 */
uint32_t x86_atomic_fetch_sub_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order) {
    return x86_atomic_fetch_add_uint32(obj, -value, order);
}

/**
 * @brief 32-bit atomic fetch-and-or
 */
uint32_t x86_atomic_fetch_or_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order) {
    uint32_t old_val, new_val;
    do {
        old_val = obj->value;
        new_val = old_val | value;
    } while (!x86_atomic_compare_exchange_uint32(obj, &old_val, new_val, order));
    return old_val;
}

/**
 * @brief 32-bit atomic fetch-and-and
 */
uint32_t x86_atomic_fetch_and_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order) {
    uint32_t old_val, new_val;
    do {
        old_val = obj->value;
        new_val = old_val & value;
    } while (!x86_atomic_compare_exchange_uint32(obj, &old_val, new_val, order));
    return old_val;
}

/**
 * @brief 32-bit atomic fetch-and-xor
 */
uint32_t x86_atomic_fetch_xor_uint32(atomic_uint32_t* obj, uint32_t value, memory_order_t order) {
    uint32_t old_val, new_val;
    do {
        old_val = obj->value;
        new_val = old_val ^ value;
    } while (!x86_atomic_compare_exchange_uint32(obj, &old_val, new_val, order));
    return old_val;
}

/**
 * @brief 64-bit atomic load
 */
uint64_t x86_atomic_load_uint64(const atomic_uint64_t* obj, memory_order_t order) {
#ifdef __x86_64__
    uint64_t result = obj->value;
    apply_memory_order_fence(order);
    return result;
#else
    // On 32-bit x86, use CMPXCHG8B for atomic 64-bit load
    uint64_t result;
    __asm__ volatile(
        "movl %%ebx, %%eax\n"
        "movl %%ecx, %%edx\n"
        "lock cmpxchg8b %1"
        : "=&A"(result)
        : "m"(obj->value), "b"(0), "c"(0)
        : "memory"
    );
    apply_memory_order_fence(order);
    return result;
#endif
}

/**
 * @brief 64-bit atomic store
 */
void x86_atomic_store_uint64(atomic_uint64_t* obj, uint64_t value, memory_order_t order) {
#ifdef __x86_64__
    apply_memory_order_fence(order);
    obj->value = value;
    if (order == MEMORY_ORDER_SEQ_CST) {
        x86_memory_fence_full();
    }
#else
    // On 32-bit x86, use CMPXCHG8B for atomic 64-bit store
    uint64_t expected;
    do {
        expected = obj->value;
    } while (!x86_atomic_compare_exchange_uint64(obj, &expected, value, order));
#endif
}

/**
 * @brief 64-bit atomic compare-and-swap
 */
bool x86_atomic_compare_exchange_uint64(atomic_uint64_t* obj, uint64_t* expected, 
                                       uint64_t desired, memory_order_t order) {
#ifdef __x86_64__
    uint64_t prev = *expected;
    bool success;
    
    __asm__ volatile(
        "lock cmpxchgq %2, %1\n"
        "sete %0"
        : "=q"(success), "+m"(obj->value), "+a"(prev)
        : "r"(desired)
        : "memory"
    );
    
    *expected = prev;
    apply_memory_order_fence(order);
    
    x86_atomic_perf_record_cas(&global_perf_stats, success);
    return success;
#else
    // On 32-bit x86, use CMPXCHG8B
    uint32_t expected_low = (uint32_t)*expected;
    uint32_t expected_high = (uint32_t)(*expected >> 32);
    uint32_t desired_low = (uint32_t)desired;
    uint32_t desired_high = (uint32_t)(desired >> 32);
    bool success;
    
    __asm__ volatile(
        "lock cmpxchg8b %1\n"
        "sete %0"
        : "=q"(success), "+m"(obj->value), "+a"(expected_low), "+d"(expected_high)
        : "b"(desired_low), "c"(desired_high)
        : "memory"
    );
    
    *expected = ((uint64_t)expected_high << 32) | expected_low;
    apply_memory_order_fence(order);
    
    x86_atomic_perf_record_cas(&global_perf_stats, success);
    return success;
#endif
}

/**
 * @brief 128-bit atomic compare-and-swap (CMPXCHG16B)
 */
bool x86_atomic_compare_exchange_128(volatile uint64_t* obj, uint64_t* expected_low, 
                                    uint64_t* expected_high, uint64_t desired_low, 
                                    uint64_t desired_high) {
#ifdef __x86_64__
    bool success;
    uint64_t exp_low = *expected_low;
    uint64_t exp_high = *expected_high;
    
    __asm__ volatile(
        "lock cmpxchg16b %1\n"
        "sete %0"
        : "=q"(success), "+m"(*obj), "+a"(exp_low), "+d"(exp_high)
        : "b"(desired_low), "c"(desired_high)
        : "memory"
    );
    
    *expected_low = exp_low;
    *expected_high = exp_high;
    
    return success;
#else
    // Not supported on 32-bit x86
    return false;
#endif
}

/**
 * @brief Initialize spinlock
 */
void x86_spinlock_init(spinlock_t* lock) {
    if (!lock) return;
    
    lock->lock.value = 0;
    lock->owner_cpu = 0;
    lock->acquire_time = 0;
    lock->file = NULL;
    lock->line = 0;
}

/**
 * @brief Acquire spinlock
 */
void x86_spinlock_lock(spinlock_t* lock) {
    if (!lock) return;
    
    uint64_t start_cycles = __builtin_ia32_rdtsc();
    uint32_t backoff = 1;
    
    while (1) {
        // Try to acquire the lock
        uint32_t expected = 0;
        if (x86_atomic_compare_exchange_uint32(&lock->lock, &expected, 1, MEMORY_ORDER_ACQUIRE)) {
            lock->acquire_time = __builtin_ia32_rdtsc();
            break;
        }
        
        // Backoff strategy
        x86_exponential_backoff(&backoff);
        
        // Spin on read to reduce cache coherency traffic
        while (x86_atomic_load_uint32(&lock->lock, MEMORY_ORDER_RELAXED) != 0) {
            x86_cpu_pause();
        }
    }
    
    uint64_t wait_cycles = __builtin_ia32_rdtsc() - start_cycles;
    x86_atomic_perf_record_lock(&global_perf_stats, wait_cycles);
}

/**
 * @brief Try to acquire spinlock
 */
bool x86_spinlock_trylock(spinlock_t* lock) {
    if (!lock) return false;
    
    uint32_t expected = 0;
    if (x86_atomic_compare_exchange_uint32(&lock->lock, &expected, 1, MEMORY_ORDER_ACQUIRE)) {
        lock->acquire_time = __builtin_ia32_rdtsc();
        return true;
    }
    
    return false;
}

/**
 * @brief Release spinlock
 */
void x86_spinlock_unlock(spinlock_t* lock) {
    if (!lock) return;
    
    x86_atomic_store_uint32(&lock->lock, 0, MEMORY_ORDER_RELEASE);
}

/**
 * @brief Check if spinlock is locked
 */
bool x86_spinlock_is_locked(const spinlock_t* lock) {
    if (!lock) return false;
    
    return x86_atomic_load_uint32(&lock->lock, MEMORY_ORDER_RELAXED) != 0;
}

/**
 * @brief Initialize lock-free queue
 */
void x86_atomic_queue_init(atomic_queue_t* queue) {
    if (!queue) return;
    
    // Create dummy node
    atomic_queue_node_t* dummy = malloc(sizeof(atomic_queue_node_t));
    dummy->next = NULL;
    dummy->data = NULL;
    
    queue->head = dummy;
    queue->tail = dummy;
    queue->enqueue_count.value = 0;
    queue->dequeue_count.value = 0;
}

/**
 * @brief Enqueue item to lock-free queue
 */
bool x86_atomic_queue_enqueue(atomic_queue_t* queue, void* data) {
    if (!queue || !data) return false;
    
    // Allocate new node
    atomic_queue_node_t* new_node = malloc(sizeof(atomic_queue_node_t));
    if (!new_node) return false;
    
    new_node->data = data;
    new_node->next = NULL;
    
    while (1) {
        atomic_queue_node_t* tail = (atomic_queue_node_t*)queue->tail;
        atomic_queue_node_t* next = (atomic_queue_node_t*)tail->next;
        
        if (tail == queue->tail) { // Tail hasn't changed
            if (next == NULL) {
                // Try to link new node at the end of the list
                if (__sync_bool_compare_and_swap(&tail->next, next, new_node)) {
                    break; // Successfully enqueued
                }
            } else {
                // Try to swing tail to the next node
                __sync_bool_compare_and_swap(&queue->tail, tail, next);
            }
        }
    }
    
    // Try to swing tail to the new node
    __sync_bool_compare_and_swap(&queue->tail, queue->tail, new_node);
    
    x86_atomic_fetch_add_uint64(&queue->enqueue_count, 1, MEMORY_ORDER_RELAXED);
    return true;
}

/**
 * @brief Dequeue item from lock-free queue
 */
bool x86_atomic_queue_dequeue(atomic_queue_t* queue, void** data) {
    if (!queue || !data) return false;
    
    while (1) {
        atomic_queue_node_t* head = (atomic_queue_node_t*)queue->head;
        atomic_queue_node_t* tail = (atomic_queue_node_t*)queue->tail;
        atomic_queue_node_t* next = (atomic_queue_node_t*)head->next;
        
        if (head == queue->head) { // Head hasn't changed
            if (head == tail) {
                if (next == NULL) {
                    return false; // Queue is empty
                }
                // Try to swing tail to the next node
                __sync_bool_compare_and_swap(&queue->tail, tail, next);
            } else {
                if (next == NULL) {
                    continue; // Inconsistent state, retry
                }
                
                // Read data before CAS
                *data = next->data;
                
                // Try to swing head to the next node
                if (__sync_bool_compare_and_swap(&queue->head, head, next)) {
                    free(head); // Free old dummy node
                    x86_atomic_fetch_add_uint64(&queue->dequeue_count, 1, MEMORY_ORDER_RELAXED);
                    return true;
                }
            }
        }
    }
}

/**
 * @brief CPU pause instruction
 */
void x86_cpu_pause(void) {
    __asm__ volatile("pause" ::: "memory");
}

/**
 * @brief CPU relax (alias for pause)
 */
void x86_cpu_relax(void) {
    x86_cpu_pause();
}

/**
 * @brief Exponential backoff
 */
void x86_exponential_backoff(uint32_t* backoff_state) {
    uint32_t delay = *backoff_state;
    
    for (uint32_t i = 0; i < delay; i++) {
        x86_cpu_pause();
    }
    
    // Exponential increase with cap
    if (delay < 1024) {
        *backoff_state = delay * 2;
    }
}

/**
 * @brief Prefetch for read
 */
void x86_prefetch_read(const void* addr) {
    __builtin_prefetch(addr, 0, 3); // Read, high temporal locality
}

/**
 * @brief Prefetch for write
 */
void x86_prefetch_write(void* addr) {
    __builtin_prefetch(addr, 1, 3); // Write, high temporal locality
}

/**
 * @brief Cache line flush
 */
void x86_clflush(const void* addr) {
    __asm__ volatile("clflush %0" : : "m"(*(const char*)addr) : "memory");
}

/**
 * @brief Initialize performance statistics
 */
void x86_atomic_perf_init(atomic_perf_stats_t* stats) {
    if (!stats) return;
    
    memset(stats, 0, sizeof(*stats));
}

/**
 * @brief Record CAS performance
 */
void x86_atomic_perf_record_cas(atomic_perf_stats_t* stats, bool success) {
    if (!stats) return;
    
    if (success) {
        x86_atomic_fetch_add_uint64((atomic_uint64_t*)&stats->successful_cas, 1, MEMORY_ORDER_RELAXED);
    } else {
        x86_atomic_fetch_add_uint64((atomic_uint64_t*)&stats->failed_cas, 1, MEMORY_ORDER_RELAXED);
    }
}

/**
 * @brief Record lock performance
 */
void x86_atomic_perf_record_lock(atomic_perf_stats_t* stats, uint64_t wait_cycles) {
    if (!stats) return;
    
    x86_atomic_fetch_add_uint64((atomic_uint64_t*)&stats->lock_acquisitions, 1, MEMORY_ORDER_RELAXED);
    x86_atomic_fetch_add_uint64((atomic_uint64_t*)&stats->total_wait_cycles, wait_cycles, MEMORY_ORDER_RELAXED);
    
    // Update max wait cycles
    uint64_t current_max;
    do {
        current_max = stats->max_wait_cycles;
        if (wait_cycles <= current_max) break;
    } while (!x86_atomic_compare_exchange_uint64((atomic_uint64_t*)&stats->max_wait_cycles, 
                                                &current_max, wait_cycles, MEMORY_ORDER_RELAXED));
    
    if (wait_cycles > 1000) { // Arbitrary threshold for contention
        x86_atomic_fetch_add_uint64((atomic_uint64_t*)&stats->lock_contentions, 1, MEMORY_ORDER_RELAXED);
    }
}

/**
 * @brief Print performance statistics
 */
void x86_atomic_perf_print(const atomic_perf_stats_t* stats) {
    if (!stats) return;
    
    printf("Atomic Performance Statistics:\n");
    printf("  Successful CAS: %lu\n", stats->successful_cas);
    printf("  Failed CAS: %lu\n", stats->failed_cas);
    printf("  CAS Success Rate: %.2f%%\n", 
           stats->successful_cas + stats->failed_cas > 0 ? 
           (100.0 * stats->successful_cas) / (stats->successful_cas + stats->failed_cas) : 0.0);
    printf("  Lock Acquisitions: %lu\n", stats->lock_acquisitions);
    printf("  Lock Contentions: %lu\n", stats->lock_contentions);
    printf("  Contention Rate: %.2f%%\n",
           stats->lock_acquisitions > 0 ?
           (100.0 * stats->lock_contentions) / stats->lock_acquisitions : 0.0);
    printf("  Total Wait Cycles: %lu\n", stats->total_wait_cycles);
    printf("  Average Wait Cycles: %.2f\n",
           stats->lock_acquisitions > 0 ?
           (double)stats->total_wait_cycles / stats->lock_acquisitions : 0.0);
    printf("  Max Wait Cycles: %lu\n", stats->max_wait_cycles);
}

/**
 * @brief Get memory order name
 */
const char* x86_memory_order_to_string(memory_order_t order) {
    if (order < sizeof(memory_order_names) / sizeof(memory_order_names[0])) {
        return memory_order_names[order];
    }
    return "unknown";
}

/**
 * @brief Check if memory order is valid
 */
bool x86_is_memory_order_valid(memory_order_t order) {
    return order >= MEMORY_ORDER_RELAXED && order <= MEMORY_ORDER_SEQ_CST;
}

/**
 * @brief Check if size is lock-free
 */
bool x86_atomic_is_lock_free_size(size_t size) {
    switch (size) {
        case 1:
        case 2:
        case 4:
            return true;
        case 8:
#ifdef __x86_64__
            return true;
#else
            return false; // Requires CMPXCHG8B which may not be lock-free
#endif
        case 16:
#ifdef __x86_64__
            // Check for CMPXCHG16B support
            uint32_t eax, ebx, ecx, edx;
            __asm__ volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
            return (ecx & (1 << 13)) != 0; // CMPXCHG16B bit
#else
            return false;
#endif
        default:
            return false;
    }
}
