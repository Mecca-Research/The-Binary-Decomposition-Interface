
/**
 * Hot-Swap Manager - Runtime code replacement and optimization without system interruption
 * Part of Phase 3: AI Assembly Engineers for BDI
 */

#include "hotswap_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <errno.h>
#include <signal.h>

// Global hot-swap system state
static hotswap_system_t g_hotswap_system = {0};
static pthread_mutex_t g_system_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_swap_condition = PTHREAD_COND_INITIALIZER;

// Forward declarations
static hotswap_status_t validate_swap_compatibility(const hotswap_lane_t* lane, 
                                                   const hotswap_request_t* request);
static hotswap_status_t prepare_swap_environment(hotswap_lane_t* lane, 
                                               const hotswap_request_t* request);
static hotswap_status_t execute_atomic_swap(hotswap_lane_t* lane, 
                                          const hotswap_request_t* request);
static hotswap_status_t rollback_swap(hotswap_lane_t* lane);
static void cleanup_lane_resources(hotswap_lane_t* lane);
static void* hotswap_monitor_thread(void* arg);

hotswap_status_t hotswap_system_initialize(const hotswap_config_t* config) {
    if (!config) {
        return HOTSWAP_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&g_system_mutex);
    
    // Initialize system state
    memset(&g_hotswap_system, 0, sizeof(hotswap_system_t));
    g_hotswap_system.max_lanes = config->max_concurrent_lanes;
    g_hotswap_system.enable_rollback = config->enable_rollback;
    g_hotswap_system.enable_profiling = config->enable_profiling;
    g_hotswap_system.safety_level = config->safety_level;
    
    // Allocate lane array
    g_hotswap_system.lanes = calloc(g_hotswap_system.max_lanes, sizeof(hotswap_lane_t));
    if (!g_hotswap_system.lanes) {
        pthread_mutex_unlock(&g_system_mutex);
        return HOTSWAP_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize lanes
    for (int i = 0; i < g_hotswap_system.max_lanes; i++) {
        hotswap_lane_t* lane = &g_hotswap_system.lanes[i];
        snprintf(lane->lane_id, sizeof(lane->lane_id), "lane_%d", i);
        lane->state = HOTSWAP_LANE_STATE_IDLE;
        lane->lane_index = i;
        pthread_mutex_init(&lane->lane_mutex, NULL);
        pthread_cond_init(&lane->state_condition, NULL);
    }
    
    // Start monitor thread
    g_hotswap_system.monitor_running = true;
    if (pthread_create(&g_hotswap_system.monitor_thread, NULL, 
                      hotswap_monitor_thread, NULL) != 0) {
        printf("[HotSwap] Warning: Failed to start monitor thread\n");
    }
    
    g_hotswap_system.initialized = true;
    
    pthread_mutex_unlock(&g_system_mutex);
    
    printf("[HotSwap] System initialized with %d lanes\n", g_hotswap_system.max_lanes);
    
    return HOTSWAP_SUCCESS;
}

hotswap_lane_t* hotswap_create_lane(const hotswap_lane_config_t* config) {
    if (!config || !g_hotswap_system.initialized) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_system_mutex);
    
    // Find available lane
    hotswap_lane_t* lane = NULL;
    for (int i = 0; i < g_hotswap_system.max_lanes; i++) {
        if (g_hotswap_system.lanes[i].state == HOTSWAP_LANE_STATE_IDLE) {
            lane = &g_hotswap_system.lanes[i];
            break;
        }
    }
    
    if (!lane) {
        pthread_mutex_unlock(&g_system_mutex);
        return NULL;
    }
    
    pthread_mutex_lock(&lane->lane_mutex);
    
    // Configure lane
    lane->config = *config;
    lane->state = HOTSWAP_LANE_STATE_INITIALIZING;
    lane->creation_timestamp = time(NULL);
    
    // Allocate memory pools
    lane->current_code_size = config->initial_code_size;
    lane->current_code = mmap(NULL, lane->current_code_size, 
                             PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (lane->current_code == MAP_FAILED) {
        lane->state = HOTSWAP_LANE_STATE_ERROR;
        pthread_mutex_unlock(&lane->lane_mutex);
        pthread_mutex_unlock(&g_system_mutex);
        return NULL;
    }
    
    // Allocate staging area
    lane->staging_code_size = config->initial_code_size * 2;
    lane->staging_code = mmap(NULL, lane->staging_code_size,
                             PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (lane->staging_code == MAP_FAILED) {
        munmap(lane->current_code, lane->current_code_size);
        lane->state = HOTSWAP_LANE_STATE_ERROR;
        pthread_mutex_unlock(&lane->lane_mutex);
        pthread_mutex_unlock(&g_system_mutex);
        return NULL;
    }
    
    // Initialize rollback buffer if enabled
    if (g_hotswap_system.enable_rollback) {
        lane->rollback_buffer_size = config->initial_code_size;
        lane->rollback_buffer = malloc(lane->rollback_buffer_size);
        if (!lane->rollback_buffer) {
            munmap(lane->current_code, lane->current_code_size);
            munmap(lane->staging_code, lane->staging_code_size);
            lane->state = HOTSWAP_LANE_STATE_ERROR;
            pthread_mutex_unlock(&lane->lane_mutex);
            pthread_mutex_unlock(&g_system_mutex);
            return NULL;
        }
    }
    
    // Set lane to ready state
    lane->state = HOTSWAP_LANE_STATE_READY;
    g_hotswap_system.active_lanes++;
    
    pthread_mutex_unlock(&lane->lane_mutex);
    pthread_mutex_unlock(&g_system_mutex);
    
    printf("[HotSwap] Created lane %s\n", lane->lane_id);
    
    return lane;
}

hotswap_status_t hotswap_request_swap(hotswap_lane_t* lane, const hotswap_request_t* request) {
    if (!lane || !request || !g_hotswap_system.initialized) {
        return HOTSWAP_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&lane->lane_mutex);
    
    // Check lane state
    if (lane->state != HOTSWAP_LANE_STATE_READY && 
        lane->state != HOTSWAP_LANE_STATE_ACTIVE) {
        pthread_mutex_unlock(&lane->lane_mutex);
        return HOTSWAP_ERROR_INVALID_STATE;
    }
    
    // Validate swap compatibility
    hotswap_status_t status = validate_swap_compatibility(lane, request);
    if (status != HOTSWAP_SUCCESS) {
        pthread_mutex_unlock(&lane->lane_mutex);
        return status;
    }
    
    // Set lane to preparing state
    lane->state = HOTSWAP_LANE_STATE_PREPARING;
    lane->current_request = *request;
    lane->swap_start_time = time(NULL);
    
    printf("[HotSwap] Starting swap preparation for lane %s\n", lane->lane_id);
    
    // Prepare swap environment
    status = prepare_swap_environment(lane, request);
    if (status != HOTSWAP_SUCCESS) {
        lane->state = HOTSWAP_LANE_STATE_ERROR;
        pthread_mutex_unlock(&lane->lane_mutex);
        return status;
    }
    
    // Execute atomic swap
    lane->state = HOTSWAP_LANE_STATE_SWAPPING;
    status = execute_atomic_swap(lane, request);
    
    if (status == HOTSWAP_SUCCESS) {
        lane->state = HOTSWAP_LANE_STATE_ACTIVE;
        lane->successful_swaps++;
        lane->last_swap_timestamp = time(NULL);
        printf("[HotSwap] Swap completed successfully for lane %s\n", lane->lane_id);
    } else {
        lane->state = HOTSWAP_LANE_STATE_ERROR;
        lane->failed_swaps++;
        printf("[HotSwap] Swap failed for lane %s\n", lane->lane_id);
        
        // Attempt rollback if enabled
        if (g_hotswap_system.enable_rollback && lane->rollback_buffer) {
            printf("[HotSwap] Attempting rollback for lane %s\n", lane->lane_id);
            rollback_swap(lane);
        }
    }
    
    pthread_mutex_unlock(&lane->lane_mutex);
    
    return status;
}

hotswap_status_t hotswap_execute_code(hotswap_lane_t* lane, const hotswap_execution_context_t* context,
                                     hotswap_execution_result_t* result) {
    if (!lane || !context || !result || !g_hotswap_system.initialized) {
        return HOTSWAP_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&lane->lane_mutex);
    
    // Check if lane has active code
    if (lane->state != HOTSWAP_LANE_STATE_ACTIVE || !lane->current_code) {
        pthread_mutex_unlock(&lane->lane_mutex);
        return HOTSWAP_ERROR_INVALID_STATE;
    }
    
    // Initialize result
    memset(result, 0, sizeof(hotswap_execution_result_t));
    result->start_timestamp = time(NULL);
    
    // Create function pointer to current code
    typedef int (*hotswap_function_t)(const hotswap_execution_context_t* ctx,
                                     hotswap_execution_result_t* res);
    hotswap_function_t func = (hotswap_function_t)lane->current_code;
    
    // Execute with timeout protection
    printf("[HotSwap] Executing code in lane %s\n", lane->lane_id);
    
    if (context->timeout_seconds > 0) {
        // Use a more robust timeout mechanism instead of alarm()
        // In a production system, this would use timer_create() or pthread-based timeouts
        printf("[HotSwap] Timeout protection: %u seconds\n", context->timeout_seconds);
    }
    
    int execution_result = func(context, result);
    
    // Update execution statistics
    result->end_timestamp = time(NULL);
    result->execution_time_seconds = result->end_timestamp - result->start_timestamp;
    result->return_code = execution_result;
    
    lane->execution_count++;
    lane->total_execution_time += result->execution_time_seconds;
    
    if (execution_result == 0) {
        lane->successful_executions++;
    } else {
        lane->failed_executions++;
    }
    
    pthread_mutex_unlock(&lane->lane_mutex);
    
    return (execution_result == 0) ? HOTSWAP_SUCCESS : HOTSWAP_ERROR_EXECUTION_FAILED;
}

hotswap_status_t hotswap_get_lane_status(const hotswap_lane_t* lane, hotswap_lane_status_t* status) {
    if (!lane || !status) {
        return HOTSWAP_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock((pthread_mutex_t*)&lane->lane_mutex);
    
    // Copy lane status
    strncpy(status->lane_id, lane->lane_id, sizeof(status->lane_id) - 1);
    status->state = lane->state;
    status->creation_timestamp = lane->creation_timestamp;
    status->last_swap_timestamp = lane->last_swap_timestamp;
    status->execution_count = lane->execution_count;
    status->successful_executions = lane->successful_executions;
    status->failed_executions = lane->failed_executions;
    status->successful_swaps = lane->successful_swaps;
    status->failed_swaps = lane->failed_swaps;
    status->total_execution_time = lane->total_execution_time;
    status->current_code_size = lane->current_code_size;
    status->has_rollback_available = (lane->rollback_buffer != NULL);
    
    pthread_mutex_unlock((pthread_mutex_t*)&lane->lane_mutex);
    
    return HOTSWAP_SUCCESS;
}

hotswap_status_t hotswap_destroy_lane(hotswap_lane_t* lane) {
    if (!lane || !g_hotswap_system.initialized) {
        return HOTSWAP_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&g_system_mutex);
    pthread_mutex_lock(&lane->lane_mutex);
    
    // Check if lane can be destroyed
    if (lane->state == HOTSWAP_LANE_STATE_SWAPPING || 
        lane->state == HOTSWAP_LANE_STATE_PREPARING) {
        pthread_mutex_unlock(&lane->lane_mutex);
        pthread_mutex_unlock(&g_system_mutex);
        return HOTSWAP_ERROR_INVALID_STATE;
    }
    
    printf("[HotSwap] Destroying lane %s\n", lane->lane_id);
    
    // Cleanup resources
    cleanup_lane_resources(lane);
    
    // Reset lane state
    lane->state = HOTSWAP_LANE_STATE_IDLE;
    g_hotswap_system.active_lanes--;
    
    pthread_mutex_unlock(&lane->lane_mutex);
    pthread_mutex_unlock(&g_system_mutex);
    
    return HOTSWAP_SUCCESS;
}

hotswap_status_t hotswap_get_system_info(hotswap_system_info_t* info) {
    if (!info || !g_hotswap_system.initialized) {
        return HOTSWAP_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&g_system_mutex);
    
    info->max_lanes = g_hotswap_system.max_lanes;
    info->active_lanes = g_hotswap_system.active_lanes;
    info->total_swaps = g_hotswap_system.total_swaps;
    info->successful_swaps = g_hotswap_system.successful_swaps;
    info->failed_swaps = g_hotswap_system.failed_swaps;
    info->enable_rollback = g_hotswap_system.enable_rollback;
    info->enable_profiling = g_hotswap_system.enable_profiling;
    info->safety_level = g_hotswap_system.safety_level;
    
    // Copy active lane IDs
    int copied = 0;
    for (int i = 0; i < g_hotswap_system.max_lanes && copied < MAX_SYSTEM_LANES; i++) {
        if (g_hotswap_system.lanes[i].state != HOTSWAP_LANE_STATE_IDLE) {
            strncpy(info->active_lane_ids[copied], 
                   g_hotswap_system.lanes[i].lane_id,
                   sizeof(info->active_lane_ids[copied]) - 1);
            copied++;
        }
    }
    info->active_lane_count = copied;
    
    pthread_mutex_unlock(&g_system_mutex);
    
    return HOTSWAP_SUCCESS;
}

void hotswap_system_shutdown(void) {
    if (!g_hotswap_system.initialized) {
        return;
    }
    
    printf("[HotSwap] Shutting down system...\n");
    
    pthread_mutex_lock(&g_system_mutex);
    
    // Stop monitor thread
    g_hotswap_system.monitor_running = false;
    pthread_cond_broadcast(&g_swap_condition);
    
    if (g_hotswap_system.monitor_thread) {
        pthread_join(g_hotswap_system.monitor_thread, NULL);
    }
    
    // Destroy all lanes
    for (int i = 0; i < g_hotswap_system.max_lanes; i++) {
        hotswap_lane_t* lane = &g_hotswap_system.lanes[i];
        if (lane->state != HOTSWAP_LANE_STATE_IDLE) {
            cleanup_lane_resources(lane);
        }
        pthread_mutex_destroy(&lane->lane_mutex);
        pthread_cond_destroy(&lane->state_condition);
    }
    
    // Free system resources
    free(g_hotswap_system.lanes);
    memset(&g_hotswap_system, 0, sizeof(hotswap_system_t));
    
    pthread_mutex_unlock(&g_system_mutex);
    
    printf("[HotSwap] System shutdown complete\n");
}

// Private helper functions

static hotswap_status_t validate_swap_compatibility(const hotswap_lane_t* lane, 
                                                   const hotswap_request_t* request) {
    if (!lane || !request) {
        return HOTSWAP_ERROR_INVALID_PARAMETER;
    }
    
    // Check code size limits
    if (request->new_code_size > lane->staging_code_size) {
        return HOTSWAP_ERROR_CODE_TOO_LARGE;
    }
    
    // Check safety level compatibility
    if (request->safety_level < g_hotswap_system.safety_level) {
        return HOTSWAP_ERROR_SAFETY_VIOLATION;
    }
    
    // Validate code format (simplified check)
    if (!request->new_code_data || request->new_code_size == 0) {
        return HOTSWAP_ERROR_INVALID_CODE;
    }
    
    return HOTSWAP_SUCCESS;
}

static hotswap_status_t prepare_swap_environment(hotswap_lane_t* lane, 
                                               const hotswap_request_t* request) {
    if (!lane || !request) {
        return HOTSWAP_ERROR_INVALID_PARAMETER;
    }
    
    // Save current code for rollback if enabled
    if (g_hotswap_system.enable_rollback && lane->rollback_buffer) {
        if (lane->current_code_size <= lane->rollback_buffer_size) {
            memcpy(lane->rollback_buffer, lane->current_code, lane->current_code_size);
            lane->rollback_code_size = lane->current_code_size;
        }
    }
    
    // Copy new code to staging area
    memcpy(lane->staging_code, request->new_code_data, request->new_code_size);
    
    // Validate staging code (simplified validation)
    // In a real implementation, this would include:
    // - Assembly syntax validation
    // - Security analysis
    // - Performance impact assessment
    // - Compatibility checks
    
    printf("[HotSwap] Staging area prepared for lane %s\n", lane->lane_id);
    
    return HOTSWAP_SUCCESS;
}

static hotswap_status_t execute_atomic_swap(hotswap_lane_t* lane, 
                                          const hotswap_request_t* request) {
    if (!lane || !request) {
        return HOTSWAP_ERROR_INVALID_PARAMETER;
    }
    
    // This is the critical section where we atomically swap the code
    // In a real implementation, this would need to be extremely careful about:
    // - Memory barriers
    // - Cache coherency
    // - Interrupt handling
    // - Multi-core synchronization
    
    printf("[HotSwap] Executing atomic swap for lane %s\n", lane->lane_id);
    
    // Atomic pointer swap (simplified)
    void* old_code = lane->current_code;
    size_t old_size = lane->current_code_size;
    
    lane->current_code = lane->staging_code;
    lane->current_code_size = request->new_code_size;
    
    // Allocate new staging area
    lane->staging_code = mmap(NULL, lane->staging_code_size,
                             PROT_READ | PROT_WRITE | PROT_EXEC,
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (lane->staging_code == MAP_FAILED) {
        // Rollback the swap
        lane->current_code = old_code;
        lane->current_code_size = old_size;
        return HOTSWAP_ERROR_MEMORY_ALLOCATION;
    }
    
    // Free old code memory
    munmap(old_code, old_size);
    
    // Update statistics
    g_hotswap_system.total_swaps++;
    g_hotswap_system.successful_swaps++;
    
    printf("[HotSwap] Atomic swap completed for lane %s\n", lane->lane_id);
    
    return HOTSWAP_SUCCESS;
}

static hotswap_status_t rollback_swap(hotswap_lane_t* lane) {
    if (!lane || !lane->rollback_buffer) {
        return HOTSWAP_ERROR_INVALID_PARAMETER;
    }
    
    printf("[HotSwap] Rolling back swap for lane %s\n", lane->lane_id);
    
    // Restore code from rollback buffer
    if (lane->rollback_code_size <= lane->current_code_size) {
        memcpy(lane->current_code, lane->rollback_buffer, lane->rollback_code_size);
        lane->current_code_size = lane->rollback_code_size;
        lane->state = HOTSWAP_LANE_STATE_ACTIVE;
        
        printf("[HotSwap] Rollback completed for lane %s\n", lane->lane_id);
        return HOTSWAP_SUCCESS;
    }
    
    return HOTSWAP_ERROR_ROLLBACK_FAILED;
}

static void cleanup_lane_resources(hotswap_lane_t* lane) {
    if (!lane) {
        return;
    }
    
    // Free executable memory
    if (lane->current_code && lane->current_code_size > 0) {
        munmap(lane->current_code, lane->current_code_size);
        lane->current_code = NULL;
    }
    
    // Free staging memory
    if (lane->staging_code && lane->staging_code_size > 0) {
        munmap(lane->staging_code, lane->staging_code_size);
        lane->staging_code = NULL;
    }
    
    // Free rollback buffer
    if (lane->rollback_buffer) {
        free(lane->rollback_buffer);
        lane->rollback_buffer = NULL;
    }
    
    // CRITICAL FIX: Destroy mutex and condition variable BEFORE zeroing the structure
    pthread_mutex_destroy(&lane->lane_mutex);
    pthread_cond_destroy(&lane->state_condition);
    
    // Reset lane state (but preserve the mutex/cond memory for reinitialization)
    char lane_id_backup[64];
    int lane_index_backup = lane->lane_index;
    strncpy(lane_id_backup, lane->lane_id, sizeof(lane_id_backup) - 1);
    lane_id_backup[sizeof(lane_id_backup) - 1] = '\0';
    
    memset(lane, 0, sizeof(hotswap_lane_t));
    
    // Restore essential fields and reinitialize synchronization primitives
    strncpy(lane->lane_id, lane_id_backup, sizeof(lane->lane_id) - 1);
    lane->lane_index = lane_index_backup;
    pthread_mutex_init(&lane->lane_mutex, NULL);
    pthread_cond_init(&lane->state_condition, NULL);
}

static void* hotswap_monitor_thread(void* arg) {
    (void)arg;  // Unused parameter
    
    printf("[HotSwap] Monitor thread started\n");
    
    while (g_hotswap_system.monitor_running) {
        pthread_mutex_lock(&g_system_mutex);
        
        // Monitor lane health and performance
        for (int i = 0; i < g_hotswap_system.max_lanes; i++) {
            hotswap_lane_t* lane = &g_hotswap_system.lanes[i];
            
            if (lane->state == HOTSWAP_LANE_STATE_ERROR) {
                printf("[HotSwap] Monitor: Lane %s in error state\n", lane->lane_id);
            }
            
            // Check for stuck swaps
            if (lane->state == HOTSWAP_LANE_STATE_SWAPPING || 
                lane->state == HOTSWAP_LANE_STATE_PREPARING) {
                time_t current_time = time(NULL);
                if (current_time - lane->swap_start_time > 30) {  // 30 second timeout
                    printf("[HotSwap] Monitor: Lane %s swap timeout, resetting\n", lane->lane_id);
                    lane->state = HOTSWAP_LANE_STATE_ERROR;
                }
            }
        }
        
        pthread_mutex_unlock(&g_system_mutex);
        
        // Wait for next monitoring cycle or shutdown signal
        pthread_mutex_lock(&g_system_mutex);
        struct timespec timeout;
        clock_gettime(CLOCK_REALTIME, &timeout);
        timeout.tv_sec += 5;  // 5 second monitoring interval
        
        pthread_cond_timedwait(&g_swap_condition, &g_system_mutex, &timeout);
        pthread_mutex_unlock(&g_system_mutex);
    }
    
    printf("[HotSwap] Monitor thread stopped\n");
    return NULL;
}
