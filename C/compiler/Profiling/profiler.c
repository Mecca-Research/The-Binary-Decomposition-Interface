
#include "profiler.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_EVENT_CAPACITY 10000

static bool profiler_initialized = false;

bool profiler_init(void) {
    if (profiler_initialized) {
        return true;
    }
    profiler_initialized = true;
    return true;
}

void profiler_cleanup(void) {
    profiler_initialized = false;
}

ProfileSession* profiler_start_session(void) {
    if (!profiler_initialized) {
        return NULL;
    }

    ProfileSession *session = calloc(1, sizeof(ProfileSession));
    if (!session) {
        return NULL;
    }

    session->events = calloc(INITIAL_EVENT_CAPACITY, sizeof(ProfileEvent));
    if (!session->events) {
        free(session);
        return NULL;
    }

    session->event_capacity = INITIAL_EVENT_CAPACITY;
    session->event_count = 0;
    session->start_time_ns = profiler_get_timestamp_ns();
    session->is_active = true;

    return session;
}

void profiler_stop_session(ProfileSession *session) {
    if (!session) {
        return;
    }
    session->end_time_ns = profiler_get_timestamp_ns();
    session->is_active = false;
}

void profiler_record_event(ProfileSession *session, const ProfileEvent *event) {
    if (!session || !session->is_active || !event) {
        return;
    }

    // Expand capacity if needed
    if (session->event_count >= session->event_capacity) {
        size_t new_capacity = session->event_capacity * 2;
        ProfileEvent *new_events = realloc(session->events, 
                                          new_capacity * sizeof(ProfileEvent));
        if (!new_events) {
            return; // Failed to expand
        }
        session->events = new_events;
        session->event_capacity = new_capacity;
    }

    session->events[session->event_count++] = *event;
}

uint64_t profiler_get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

void profiler_hook_function_enter(ProfileSession *session, uint64_t func_id, const char *name) {
    if (!session) return;
    
    ProfileEvent event = {
        .type = PROFILE_EVENT_FUNCTION_ENTER,
        .timestamp_ns = profiler_get_timestamp_ns(),
        .function_id = func_id,
        .address = 0,
        .size = 0
    };
    
    if (name) {
        strncpy(event.name, name, sizeof(event.name) - 1);
    }
    
    profiler_record_event(session, &event);
}

void profiler_hook_function_exit(ProfileSession *session, uint64_t func_id) {
    if (!session) return;
    
    ProfileEvent event = {
        .type = PROFILE_EVENT_FUNCTION_EXIT,
        .timestamp_ns = profiler_get_timestamp_ns(),
        .function_id = func_id,
        .address = 0,
        .size = 0
    };
    
    profiler_record_event(session, &event);
}

void profiler_hook_memory_alloc(ProfileSession *session, uint64_t addr, size_t size) {
    if (!session) return;
    
    ProfileEvent event = {
        .type = PROFILE_EVENT_MEMORY_ALLOC,
        .timestamp_ns = profiler_get_timestamp_ns(),
        .address = addr,
        .size = size
    };
    
    profiler_record_event(session, &event);
}

void profiler_hook_memory_free(ProfileSession *session, uint64_t addr) {
    if (!session) return;
    
    ProfileEvent event = {
        .type = PROFILE_EVENT_MEMORY_FREE,
        .timestamp_ns = profiler_get_timestamp_ns(),
        .address = addr,
        .size = 0
    };
    
    profiler_record_event(session, &event);
}

void profiler_hook_cache_access(ProfileSession *session, uint64_t addr, bool hit) {
    if (!session) return;
    
    ProfileEvent event = {
        .type = hit ? PROFILE_EVENT_CACHE_HIT : PROFILE_EVENT_CACHE_MISS,
        .timestamp_ns = profiler_get_timestamp_ns(),
        .address = addr,
        .size = 0
    };
    
    profiler_record_event(session, &event);
}

void profiler_hook_branch(ProfileSession *session, uint64_t addr, bool taken) {
    if (!session) return;
    
    ProfileEvent event = {
        .type = taken ? PROFILE_EVENT_BRANCH_TAKEN : PROFILE_EVENT_BRANCH_NOT_TAKEN,
        .timestamp_ns = profiler_get_timestamp_ns(),
        .address = addr,
        .size = 0
    };
    
    profiler_record_event(session, &event);
}
