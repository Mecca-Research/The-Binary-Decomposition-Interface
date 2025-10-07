
#ifndef BDI_PROFILER_H
#define BDI_PROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// Profiling event types
typedef enum {
    PROFILE_EVENT_FUNCTION_ENTER,
    PROFILE_EVENT_FUNCTION_EXIT,
    PROFILE_EVENT_MEMORY_ALLOC,
    PROFILE_EVENT_MEMORY_FREE,
    PROFILE_EVENT_CACHE_HIT,
    PROFILE_EVENT_CACHE_MISS,
    PROFILE_EVENT_BRANCH_TAKEN,
    PROFILE_EVENT_BRANCH_NOT_TAKEN,
    PROFILE_EVENT_SYSCALL,
    PROFILE_EVENT_CUSTOM
} ProfileEventType;

// Profiling event structure
typedef struct {
    ProfileEventType type;
    uint64_t timestamp_ns;
    uint64_t thread_id;
    uint64_t function_id;
    uint64_t address;
    uint64_t size;
    char name[64];
} ProfileEvent;

// Profiling session
typedef struct {
    ProfileEvent *events;
    size_t event_count;
    size_t event_capacity;
    uint64_t start_time_ns;
    uint64_t end_time_ns;
    bool is_active;
} ProfileSession;

// Initialize profiler
bool profiler_init(void);

// Cleanup profiler
void profiler_cleanup(void);

// Start profiling session
ProfileSession* profiler_start_session(void);

// Stop profiling session
void profiler_stop_session(ProfileSession *session);

// Record profiling event
void profiler_record_event(ProfileSession *session, const ProfileEvent *event);

// Get current timestamp in nanoseconds
uint64_t profiler_get_timestamp_ns(void);

// Profiling hooks for VM
void profiler_hook_function_enter(ProfileSession *session, uint64_t func_id, const char *name);
void profiler_hook_function_exit(ProfileSession *session, uint64_t func_id);
void profiler_hook_memory_alloc(ProfileSession *session, uint64_t addr, size_t size);
void profiler_hook_memory_free(ProfileSession *session, uint64_t addr);
void profiler_hook_cache_access(ProfileSession *session, uint64_t addr, bool hit);
void profiler_hook_branch(ProfileSession *session, uint64_t addr, bool taken);

#endif // BDI_PROFILER_H
