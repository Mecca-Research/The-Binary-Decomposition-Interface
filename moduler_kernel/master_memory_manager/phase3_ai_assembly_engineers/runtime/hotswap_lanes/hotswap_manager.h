
/**
 * Hot-Swap Manager Header - Runtime code replacement and optimization without system interruption
 * Part of Phase 3: AI Assembly Engineers for BDI
 */

#ifndef HOTSWAP_MANAGER_H
#define HOTSWAP_MANAGER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define MAX_LANE_ID_LENGTH 64
#define MAX_SYSTEM_LANES 32
#define MAX_CODE_SIZE (512 * 1024)  // 512KB max code size per lane
#define DEFAULT_STAGING_MULTIPLIER 2

// Enums
typedef enum {
    HOTSWAP_SUCCESS = 0,
    HOTSWAP_ERROR_INVALID_PARAMETER,
    HOTSWAP_ERROR_INVALID_STATE,
    HOTSWAP_ERROR_MEMORY_ALLOCATION,
    HOTSWAP_ERROR_CODE_TOO_LARGE,
    HOTSWAP_ERROR_INVALID_CODE,
    HOTSWAP_ERROR_SAFETY_VIOLATION,
    HOTSWAP_ERROR_EXECUTION_FAILED,
    HOTSWAP_ERROR_SWAP_TIMEOUT,
    HOTSWAP_ERROR_ROLLBACK_FAILED,
    HOTSWAP_ERROR_SYSTEM_NOT_INITIALIZED,
    HOTSWAP_ERROR_LANE_NOT_FOUND
} hotswap_status_t;

typedef enum {
    HOTSWAP_LANE_STATE_IDLE = 0,
    HOTSWAP_LANE_STATE_INITIALIZING,
    HOTSWAP_LANE_STATE_READY,
    HOTSWAP_LANE_STATE_ACTIVE,
    HOTSWAP_LANE_STATE_PREPARING,
    HOTSWAP_LANE_STATE_SWAPPING,
    HOTSWAP_LANE_STATE_ERROR,
    HOTSWAP_LANE_STATE_DESTROYING
} hotswap_lane_state_t;

typedef enum {
    HOTSWAP_SAFETY_NONE = 0,
    HOTSWAP_SAFETY_BASIC,
    HOTSWAP_SAFETY_STANDARD,
    HOTSWAP_SAFETY_STRICT,
    HOTSWAP_SAFETY_MAXIMUM
} hotswap_safety_level_t;

typedef enum {
    HOTSWAP_PRIORITY_LOW = 1,
    HOTSWAP_PRIORITY_NORMAL = 2,
    HOTSWAP_PRIORITY_HIGH = 3,
    HOTSWAP_PRIORITY_CRITICAL = 4,
    HOTSWAP_PRIORITY_EMERGENCY = 5
} hotswap_priority_t;

// Structures
typedef struct {
    int max_concurrent_lanes;
    bool enable_rollback;
    bool enable_profiling;
    bool enable_monitoring;
    hotswap_safety_level_t safety_level;
    int swap_timeout_seconds;
} hotswap_config_t;

typedef struct {
    size_t initial_code_size;
    bool enable_profiling;
    bool enable_rollback;
    hotswap_safety_level_t safety_level;
    int max_execution_time;
} hotswap_lane_config_t;

typedef struct {
    void* new_code_data;
    size_t new_code_size;
    hotswap_priority_t priority;
    hotswap_safety_level_t safety_level;
    bool enable_rollback;
    int timeout_seconds;
    char description[128];
} hotswap_request_t;

typedef struct {
    void* input_data;
    size_t input_size;
    void* output_buffer;
    size_t output_buffer_size;
    int timeout_seconds;
    bool enable_profiling;
} hotswap_execution_context_t;

typedef struct {
    int return_code;
    void* output_data;
    size_t output_size;
    time_t start_timestamp;
    time_t end_timestamp;
    long execution_time_seconds;
    uint64_t instructions_executed;
    uint64_t memory_used;
    uint64_t cpu_cycles;
} hotswap_execution_result_t;

typedef struct {
    char lane_id[MAX_LANE_ID_LENGTH];
    hotswap_lane_config_t config;
    hotswap_lane_state_t state;
    int lane_index;
    
    // Code memory management
    void* current_code;
    size_t current_code_size;
    void* staging_code;
    size_t staging_code_size;
    
    // Rollback support
    void* rollback_buffer;
    size_t rollback_buffer_size;
    size_t rollback_code_size;
    
    // Current swap request
    hotswap_request_t current_request;
    
    // Timing and statistics
    time_t creation_timestamp;
    time_t last_swap_timestamp;
    time_t swap_start_time;
    int execution_count;
    int successful_executions;
    int failed_executions;
    int successful_swaps;
    int failed_swaps;
    long total_execution_time;
    
    // Synchronization
    pthread_mutex_t lane_mutex;
    pthread_cond_t state_condition;
} hotswap_lane_t;

typedef struct {
    hotswap_lane_t* lanes;
    int max_lanes;
    int active_lanes;
    int total_swaps;
    int successful_swaps;
    int failed_swaps;
    bool enable_rollback;
    bool enable_profiling;
    hotswap_safety_level_t safety_level;
    bool initialized;
    
    // Monitor thread
    pthread_t monitor_thread;
    bool monitor_running;
} hotswap_system_t;

typedef struct {
    char lane_id[MAX_LANE_ID_LENGTH];
    hotswap_lane_state_t state;
    time_t creation_timestamp;
    time_t last_swap_timestamp;
    int execution_count;
    int successful_executions;
    int failed_executions;
    int successful_swaps;
    int failed_swaps;
    long total_execution_time;
    size_t current_code_size;
    bool has_rollback_available;
} hotswap_lane_status_t;

typedef struct {
    int max_lanes;
    int active_lanes;
    int total_swaps;
    int successful_swaps;
    int failed_swaps;
    bool enable_rollback;
    bool enable_profiling;
    hotswap_safety_level_t safety_level;
    int active_lane_count;
    char active_lane_ids[MAX_SYSTEM_LANES][MAX_LANE_ID_LENGTH];
} hotswap_system_info_t;

// Function declarations

/**
 * Initialize the hot-swap system
 */
hotswap_status_t hotswap_system_initialize(const hotswap_config_t* config);

/**
 * Create a new hot-swap lane
 */
hotswap_lane_t* hotswap_create_lane(const hotswap_lane_config_t* config);

/**
 * Request a code swap in a lane
 */
hotswap_status_t hotswap_request_swap(hotswap_lane_t* lane, const hotswap_request_t* request);

/**
 * Execute code in a hot-swap lane
 */
hotswap_status_t hotswap_execute_code(hotswap_lane_t* lane, const hotswap_execution_context_t* context,
                                     hotswap_execution_result_t* result);

/**
 * Get lane status information
 */
hotswap_status_t hotswap_get_lane_status(const hotswap_lane_t* lane, hotswap_lane_status_t* status);

/**
 * Destroy a hot-swap lane
 */
hotswap_status_t hotswap_destroy_lane(hotswap_lane_t* lane);

/**
 * Get system information
 */
hotswap_status_t hotswap_get_system_info(hotswap_system_info_t* info);

/**
 * Shutdown the hot-swap system
 */
void hotswap_system_shutdown(void);

// Utility functions

/**
 * Get status string representation
 */
static inline const char* hotswap_status_to_string(hotswap_status_t status) {
    switch (status) {
        case HOTSWAP_SUCCESS: return "Success";
        case HOTSWAP_ERROR_INVALID_PARAMETER: return "Invalid parameter";
        case HOTSWAP_ERROR_INVALID_STATE: return "Invalid state";
        case HOTSWAP_ERROR_MEMORY_ALLOCATION: return "Memory allocation failed";
        case HOTSWAP_ERROR_CODE_TOO_LARGE: return "Code too large";
        case HOTSWAP_ERROR_INVALID_CODE: return "Invalid code";
        case HOTSWAP_ERROR_SAFETY_VIOLATION: return "Safety violation";
        case HOTSWAP_ERROR_EXECUTION_FAILED: return "Execution failed";
        case HOTSWAP_ERROR_SWAP_TIMEOUT: return "Swap timeout";
        case HOTSWAP_ERROR_ROLLBACK_FAILED: return "Rollback failed";
        case HOTSWAP_ERROR_SYSTEM_NOT_INITIALIZED: return "System not initialized";
        case HOTSWAP_ERROR_LANE_NOT_FOUND: return "Lane not found";
        default: return "Unknown error";
    }
}

/**
 * Get lane state string representation
 */
static inline const char* hotswap_lane_state_to_string(hotswap_lane_state_t state) {
    switch (state) {
        case HOTSWAP_LANE_STATE_IDLE: return "Idle";
        case HOTSWAP_LANE_STATE_INITIALIZING: return "Initializing";
        case HOTSWAP_LANE_STATE_READY: return "Ready";
        case HOTSWAP_LANE_STATE_ACTIVE: return "Active";
        case HOTSWAP_LANE_STATE_PREPARING: return "Preparing";
        case HOTSWAP_LANE_STATE_SWAPPING: return "Swapping";
        case HOTSWAP_LANE_STATE_ERROR: return "Error";
        case HOTSWAP_LANE_STATE_DESTROYING: return "Destroying";
        default: return "Unknown";
    }
}

/**
 * Get safety level string representation
 */
static inline const char* hotswap_safety_level_to_string(hotswap_safety_level_t level) {
    switch (level) {
        case HOTSWAP_SAFETY_NONE: return "None";
        case HOTSWAP_SAFETY_BASIC: return "Basic";
        case HOTSWAP_SAFETY_STANDARD: return "Standard";
        case HOTSWAP_SAFETY_STRICT: return "Strict";
        case HOTSWAP_SAFETY_MAXIMUM: return "Maximum";
        default: return "Unknown";
    }
}

#ifdef __cplusplus
}
#endif

#endif // HOTSWAP_MANAGER_H
