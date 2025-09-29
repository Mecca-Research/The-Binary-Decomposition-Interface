/**
 * @file capsule_loader.h
 * @brief Capsule Loader - Dynamic loading and execution of AI-generated assembly code capsules
 * Part of Phase 3: AI Assembly Engineers for BDI
 */

#ifndef CAPSULE_LOADER_H
#define CAPSULE_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

// Constants
#define MAX_CAPSULE_SIZE (16 * 1024 * 1024)  // 16MB max capsule size
#define MAX_CAPSULE_NAME_LENGTH 64
#define MAX_CAPSULE_ID_LENGTH 32
#define MAX_REGISTRY_CAPSULES 256
#define DEFAULT_STACK_SIZE (64 * 1024)       // 64KB default stack
#define DEFAULT_HEAP_SIZE (1 * 1024 * 1024)  // 1MB default heap
#define DEFAULT_MAX_EXECUTION_TIME 30        // 30 seconds default timeout
#define TSS_IO_BITMAP_SIZE 8192
#define TSS_IO_BITMAP_OFFSET 104

// Enumerations
typedef enum {
    CAPSULE_SUCCESS = 0,
    CAPSULE_ERROR_INVALID_PARAMETER = -1,
    CAPSULE_ERROR_MEMORY_ALLOCATION = -2,
    CAPSULE_ERROR_INVALID_FORMAT = -3,
    CAPSULE_ERROR_INVALID_STATE = -4,
    CAPSULE_ERROR_SECURITY_VIOLATION = -5,
    CAPSULE_ERROR_EXECUTION_FAILED = -6,
    CAPSULE_ERROR_TIMEOUT = -7,
    CAPSULE_ERROR_NOT_FOUND = -8,
    CAPSULE_ERROR_ALREADY_EXISTS = -9,
    CAPSULE_ERROR_SYSTEM_FAILURE = -10
} capsule_status_t;

typedef enum {
    CAPSULE_STATE_UNLOADED = 0,
    CAPSULE_STATE_LOADING = 1,
    CAPSULE_STATE_LOADED = 2,
    CAPSULE_STATE_READY = 3,
    CAPSULE_STATE_EXECUTING = 4,
    CAPSULE_STATE_ERROR = 5,
    CAPSULE_STATE_TERMINATED = 6
} capsule_state_t;

typedef enum {
    CAPSULE_SECURITY_NONE = 0,
    CAPSULE_SECURITY_BASIC = 1,
    CAPSULE_SECURITY_STANDARD = 2,
    CAPSULE_SECURITY_STRICT = 3,
    CAPSULE_SECURITY_MAXIMUM = 4
} capsule_security_level_t;

// Forward declarations
typedef struct capsule_t capsule_t;
typedef struct capsule_metadata_t capsule_metadata_t;
typedef struct capsule_execution_context_t capsule_execution_context_t;
typedef struct capsule_execution_result_t capsule_execution_result_t;
typedef struct capsule_environment_t capsule_environment_t;
typedef struct capsule_load_options_t capsule_load_options_t;
typedef struct capsule_loader_config_t capsule_loader_config_t;
typedef struct capsule_registry_t capsule_registry_t;
typedef struct capsule_registry_info_t capsule_registry_info_t;

// Structures
struct capsule_metadata_t {
    char name[MAX_CAPSULE_NAME_LENGTH];
    char version[16];
    char author[64];
    char description[256];
    char target_architecture[32];
    int optimization_level;
    int safety_level;
    time_t creation_timestamp;
    uint64_t checksum;
    uint32_t flags;
};

struct capsule_execution_context_t {
    uint32_t timeout_seconds;
    void* input_data;
    size_t input_size;
    void* output_buffer;
    size_t output_buffer_size;
    uint32_t execution_flags;
    void* user_context;
};

struct capsule_execution_result_t {
    int return_code;
    time_t start_timestamp;
    time_t end_timestamp;
    uint64_t execution_time_seconds;
    size_t output_size;
    uint64_t memory_used;
    uint64_t cpu_cycles;
    char error_message[256];
};

struct capsule_environment_t {
    size_t stack_size;
    size_t heap_size;
    uint32_t max_execution_time;
    void* stack_memory;
    void* heap_memory;
    uint32_t privilege_level;
    bool sandboxed;
};

struct capsule_load_options_t {
    bool validate_signature;
    bool enable_debugging;
    bool allocate_stack;
    bool enable_profiling;
    capsule_security_level_t security_level;
    uint32_t timeout_seconds;
    size_t max_memory_usage;
};

struct capsule_loader_config_t {
    uint32_t max_concurrent_capsules;
    capsule_security_level_t security_level;
    bool enable_sandboxing;
    bool enable_profiling;
    bool enable_hot_reload;
    size_t default_stack_size;
    size_t default_heap_size;
    uint32_t default_timeout;
};

struct capsule_t {
    char capsule_id[MAX_CAPSULE_ID_LENGTH];
    capsule_metadata_t metadata;
    capsule_state_t state;
    capsule_load_options_t load_options;
    capsule_environment_t environment;
    
    // Code and execution
    char* assembly_code;
    size_t assembly_size;
    void* executable_memory;
    size_t executable_size;
    size_t allocated_memory_size;  // Track actual allocation size for proper cleanup
    
    // Execution statistics
    capsule_execution_result_t last_execution_result;
    uint32_t execution_count;
    uint32_t successful_executions;
    uint32_t failed_executions;
    uint64_t total_execution_time;
    
    // Registry management
    int registry_slot;
    time_t load_timestamp;
    time_t last_access_timestamp;
    
    // Security
    bool security_validated;
    uint64_t security_hash;
    
    // Threading
    pthread_mutex_t capsule_mutex;
};

struct capsule_registry_t {
    capsule_t** capsules;
    uint32_t max_capsules;
    uint32_t active_count;
    capsule_security_level_t security_level;
    bool enable_sandboxing;
    bool enable_profiling;
    pthread_mutex_t registry_mutex;
};

struct capsule_registry_info_t {
    uint32_t max_capsules;
    uint32_t active_count;
    capsule_security_level_t security_level;
    bool enable_sandboxing;
    bool enable_profiling;
    char active_capsule_ids[MAX_REGISTRY_CAPSULES][MAX_CAPSULE_ID_LENGTH];
};

// Function declarations

/**
 * Initialize the capsule loader system
 * @param config Configuration for the capsule loader
 * @return CAPSULE_SUCCESS on success, error code on failure
 */
capsule_status_t capsule_loader_initialize(const capsule_loader_config_t* config);

/**
 * Load a capsule from file
 * @param filepath Path to the capsule file
 * @param options Load options
 * @return Pointer to loaded capsule, NULL on failure
 */
capsule_t* capsule_load_from_file(const char* filepath, const capsule_load_options_t* options);

/**
 * Load a capsule from memory
 * @param capsule_data Capsule data in memory
 * @param size Size of capsule data
 * @param options Load options
 * @return Pointer to loaded capsule, NULL on failure
 */
capsule_t* capsule_load_from_memory(const char* capsule_data, size_t size, 
                                   const capsule_load_options_t* options);

/**
 * Execute a loaded capsule
 * @param capsule Capsule to execute
 * @param context Execution context
 * @return CAPSULE_SUCCESS on success, error code on failure
 */
capsule_status_t capsule_execute(capsule_t* capsule, const capsule_execution_context_t* context);

/**
 * Unload a capsule
 * @param capsule Capsule to unload
 * @return CAPSULE_SUCCESS on success, error code on failure
 */
capsule_status_t capsule_unload(capsule_t* capsule);

/**
 * Find a capsule by ID
 * @param capsule_id ID of the capsule to find
 * @return Pointer to capsule, NULL if not found
 */
capsule_t* capsule_find_by_id(const char* capsule_id);

/**
 * Get registry information
 * @param info Structure to fill with registry information
 * @return CAPSULE_SUCCESS on success, error code on failure
 */
capsule_status_t capsule_get_registry_info(capsule_registry_info_t* info);

/**
 * Shutdown the capsule loader system
 */
void capsule_loader_shutdown(void);

/**
 * Convert capsule status to string
 * @param status Status code
 * @return String representation of status
 */
static inline const char* capsule_status_to_string(capsule_status_t status) {
    switch (status) {
        case CAPSULE_SUCCESS: return "Success";
        case CAPSULE_ERROR_INVALID_PARAMETER: return "Invalid Parameter";
        case CAPSULE_ERROR_MEMORY_ALLOCATION: return "Memory Allocation Error";
        case CAPSULE_ERROR_INVALID_FORMAT: return "Invalid Format";
        case CAPSULE_ERROR_INVALID_STATE: return "Invalid State";
        case CAPSULE_ERROR_SECURITY_VIOLATION: return "Security Violation";
        case CAPSULE_ERROR_EXECUTION_FAILED: return "Execution Failed";
        case CAPSULE_ERROR_TIMEOUT: return "Timeout";
        case CAPSULE_ERROR_NOT_FOUND: return "Not Found";
        case CAPSULE_ERROR_ALREADY_EXISTS: return "Already Exists";
        case CAPSULE_ERROR_SYSTEM_FAILURE: return "System Failure";
        default: return "Unknown Error";
    }
}

/**
 * Convert capsule state to string
 * @param state State code
 * @return String representation of state
 */
static inline const char* capsule_state_to_string(capsule_state_t state) {
    switch (state) {
        case CAPSULE_STATE_UNLOADED: return "Unloaded";
        case CAPSULE_STATE_LOADING: return "Loading";
        case CAPSULE_STATE_LOADED: return "Loaded";
        case CAPSULE_STATE_READY: return "Ready";
        case CAPSULE_STATE_EXECUTING: return "Executing";
        case CAPSULE_STATE_ERROR: return "Error";
        case CAPSULE_STATE_TERMINATED: return "Terminated";
        default: return "Unknown State";
    }
}

#ifdef __cplusplus
}
#endif

#endif // CAPSULE_LOADER_H
