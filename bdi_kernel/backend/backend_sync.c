// ===================================================================
// DESC: Unified synchronization primitives for backend devices
// PHASE 13: Backend Acceleration - Day 4
// ===================================================================
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

constexpr int MAX_EVENTS = 256;
constexpr int MAX_FENCES = 64;
constexpr int MAX_BARRIERS = 32;

// ============================================================================
// Event Synchronization
// ============================================================================

typedef enum {
    EVENT_STATE_UNSIGNALED = 0,
    EVENT_STATE_SIGNALED = 1
} EventState;

typedef struct {
    int event_id;
    _Atomic EventState state;
    _Atomic bool is_valid;
    int device_id;
    uint64_t timestamp;
    void (*callback)(int event_id, void* user_data);
    void* user_data;
} BackendEvent;

_Static_assert(sizeof(BackendEvent) <= 64, "BackendEvent structure too large");

typedef struct {
    BackendEvent events[MAX_EVENTS];
    _Atomic int event_count;
    _Atomic uint64_t total_events_created;
    _Atomic uint64_t total_events_signaled;
} EventManager;

static EventManager event_manager = {
    .event_count = 0,
    .total_events_created = 0,
    .total_events_signaled = 0
};

static uint64_t get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

[[nodiscard]] int backend_event_create(int device_id) {
    // Find available event slot
    for (int i = 0; i < MAX_EVENTS; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong(&event_manager.events[i].is_valid, 
                                          &expected, true)) {
            BackendEvent* event = &event_manager.events[i];
            event->event_id = i;
            atomic_store(&event->state, EVENT_STATE_UNSIGNALED);
            event->device_id = device_id;
            event->timestamp = get_timestamp();
            event->callback = nullptr;
            event->user_data = nullptr;
            
            atomic_fetch_add(&event_manager.event_count, 1);
            atomic_fetch_add(&event_manager.total_events_created, 1);
            
            printf("BACKEND_SYNC: Created event %d for device %d\n", i, device_id);
            return i;
        }
    }
    
    return -1; // No available events
}

void backend_event_destroy(int event_id) {
    if (event_id < 0 || event_id >= MAX_EVENTS) {
        return;
    }
    
    BackendEvent* event = &event_manager.events[event_id];
    atomic_store(&event->is_valid, false);
    atomic_fetch_sub(&event_manager.event_count, 1);
    
    printf("BACKEND_SYNC: Destroyed event %d\n", event_id);
}

[[nodiscard]] int backend_event_signal(int event_id) {
    if (event_id < 0 || event_id >= MAX_EVENTS) {
        return -1;
    }
    
    BackendEvent* event = &event_manager.events[event_id];
    if (!atomic_load(&event->is_valid)) {
        return -1;
    }
    
    atomic_store(&event->state, EVENT_STATE_SIGNALED);
    event->timestamp = get_timestamp();
    atomic_fetch_add(&event_manager.total_events_signaled, 1);
    
    printf("BACKEND_SYNC: Signaled event %d\n", event_id);
    
    // Call callback if registered
    if (event->callback != nullptr) {
        event->callback(event_id, event->user_data);
    }
    
    return 0;
}

[[nodiscard]] int backend_event_wait(int event_id, uint64_t timeout_ns) {
    if (event_id < 0 || event_id >= MAX_EVENTS) {
        return -1;
    }
    
    BackendEvent* event = &event_manager.events[event_id];
    if (!atomic_load(&event->is_valid)) {
        return -1;
    }
    
    uint64_t start_time = get_timestamp();
    
    while (atomic_load(&event->state) != EVENT_STATE_SIGNALED) {
        if (timeout_ns > 0) {
            uint64_t elapsed = get_timestamp() - start_time;
            if (elapsed >= timeout_ns) {
                printf("BACKEND_SYNC: Event %d wait timeout\n", event_id);
                return -2; // Timeout
            }
        }
    }
    
    printf("BACKEND_SYNC: Event %d wait complete\n", event_id);
    return 0;
}

[[nodiscard]] int backend_event_register_callback(int event_id, 
                                                  void (*callback)(int, void*),
                                                  void* user_data) {
    if (event_id < 0 || event_id >= MAX_EVENTS) {
        return -1;
    }
    
    BackendEvent* event = &event_manager.events[event_id];
    if (!atomic_load(&event->is_valid)) {
        return -1;
    }
    
    event->callback = callback;
    event->user_data = user_data;
    
    return 0;
}

// ============================================================================
// Fence Synchronization
// ============================================================================

typedef struct {
    int fence_id;
    _Atomic bool is_signaled;
    _Atomic bool is_valid;
    int device_id;
    uint64_t timestamp;
} BackendFence;

typedef struct {
    BackendFence fences[MAX_FENCES];
    _Atomic int fence_count;
} FenceManager;

static FenceManager fence_manager = {
    .fence_count = 0
};

[[nodiscard]] int backend_fence_create(int device_id) {
    for (int i = 0; i < MAX_FENCES; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong(&fence_manager.fences[i].is_valid,
                                          &expected, true)) {
            BackendFence* fence = &fence_manager.fences[i];
            fence->fence_id = i;
            atomic_store(&fence->is_signaled, false);
            fence->device_id = device_id;
            fence->timestamp = get_timestamp();
            
            atomic_fetch_add(&fence_manager.fence_count, 1);
            
            printf("BACKEND_SYNC: Created fence %d for device %d\n", i, device_id);
            return i;
        }
    }
    
    return -1;
}

void backend_fence_destroy(int fence_id) {
    if (fence_id < 0 || fence_id >= MAX_FENCES) {
        return;
    }
    
    BackendFence* fence = &fence_manager.fences[fence_id];
    atomic_store(&fence->is_valid, false);
    atomic_fetch_sub(&fence_manager.fence_count, 1);
    
    printf("BACKEND_SYNC: Destroyed fence %d\n", fence_id);
}

[[nodiscard]] int backend_fence_signal(int fence_id) {
    if (fence_id < 0 || fence_id >= MAX_FENCES) {
        return -1;
    }
    
    BackendFence* fence = &fence_manager.fences[fence_id];
    if (!atomic_load(&fence->is_valid)) {
        return -1;
    }
    
    atomic_store(&fence->is_signaled, true);
    fence->timestamp = get_timestamp();
    
    printf("BACKEND_SYNC: Signaled fence %d\n", fence_id);
    return 0;
}

[[nodiscard]] int backend_fence_wait(int fence_id) {
    if (fence_id < 0 || fence_id >= MAX_FENCES) {
        return -1;
    }
    
    BackendFence* fence = &fence_manager.fences[fence_id];
    if (!atomic_load(&fence->is_valid)) {
        return -1;
    }
    
    while (!atomic_load(&fence->is_signaled)) {
        // Busy wait
    }
    
    printf("BACKEND_SYNC: Fence %d wait complete\n", fence_id);
    return 0;
}

// ============================================================================
// Barrier Synchronization
// ============================================================================

typedef struct {
    int barrier_id;
    _Atomic int participant_count;
    _Atomic int arrived_count;
    _Atomic bool is_valid;
} BackendBarrier;

typedef struct {
    BackendBarrier barriers[MAX_BARRIERS];
    _Atomic int barrier_count;
} BarrierManager;

static BarrierManager barrier_manager = {
    .barrier_count = 0
};

[[nodiscard]] int backend_barrier_create(int participant_count) {
    if (participant_count <= 0) {
        return -1;
    }
    
    for (int i = 0; i < MAX_BARRIERS; i++) {
        bool expected = false;
        if (atomic_compare_exchange_strong(&barrier_manager.barriers[i].is_valid,
                                          &expected, true)) {
            BackendBarrier* barrier = &barrier_manager.barriers[i];
            barrier->barrier_id = i;
            atomic_store(&barrier->participant_count, participant_count);
            atomic_store(&barrier->arrived_count, 0);
            
            atomic_fetch_add(&barrier_manager.barrier_count, 1);
            
            printf("BACKEND_SYNC: Created barrier %d with %d participants\n", 
                   i, participant_count);
            return i;
        }
    }
    
    return -1;
}

void backend_barrier_destroy(int barrier_id) {
    if (barrier_id < 0 || barrier_id >= MAX_BARRIERS) {
        return;
    }
    
    BackendBarrier* barrier = &barrier_manager.barriers[barrier_id];
    atomic_store(&barrier->is_valid, false);
    atomic_fetch_sub(&barrier_manager.barrier_count, 1);
    
    printf("BACKEND_SYNC: Destroyed barrier %d\n", barrier_id);
}

[[nodiscard]] int backend_barrier_wait(int barrier_id) {
    if (barrier_id < 0 || barrier_id >= MAX_BARRIERS) {
        return -1;
    }
    
    BackendBarrier* barrier = &barrier_manager.barriers[barrier_id];
    if (!atomic_load(&barrier->is_valid)) {
        return -1;
    }
    
    int arrived = atomic_fetch_add(&barrier->arrived_count, 1) + 1;
    int participants = atomic_load(&barrier->participant_count);
    
    printf("BACKEND_SYNC: Barrier %d: %d/%d arrived\n", barrier_id, arrived, participants);
    
    // Wait for all participants
    while (atomic_load(&barrier->arrived_count) < participants) {
        // Busy wait
    }
    
    printf("BACKEND_SYNC: Barrier %d released\n", barrier_id);
    return 0;
}

// ============================================================================
// Statistics
// ============================================================================

void backend_sync_statistics(void) {
    printf("\n=== Backend Synchronization Statistics ===\n");
    printf("Events: %d active, %llu created, %llu signaled\n",
           atomic_load(&event_manager.event_count),
           (unsigned long long)atomic_load(&event_manager.total_events_created),
           (unsigned long long)atomic_load(&event_manager.total_events_signaled));
    printf("Fences: %d active\n", atomic_load(&fence_manager.fence_count));
    printf("Barriers: %d active\n", atomic_load(&barrier_manager.barrier_count));
    printf("===========================================\n\n");
}
