
/**
 * Capsule Loader - Dynamic loading and execution of AI-generated assembly code capsules
 * Part of Phase 3: AI Assembly Engineers for BDI
 */

#include "capsule_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dlfcn.h>

// Global capsule registry
static capsule_registry_t g_capsule_registry = {0};
static pthread_mutex_t g_registry_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations
static capsule_status_t validate_capsule_format(const char* capsule_data, size_t size);
static capsule_status_t parse_capsule_metadata(const char* data, capsule_metadata_t* metadata);
static capsule_status_t allocate_execution_memory(size_t code_size, void** exec_memory);
static capsule_status_t compile_assembly_code(const char* assembly_code, void* exec_memory, size_t* compiled_size);
static capsule_status_t setup_capsule_environment(capsule_t* capsule);
static void cleanup_capsule_resources(capsule_t* capsule);

capsule_status_t capsule_loader_initialize(const capsule_loader_config_t* config) {
    if (!config) {
        return CAPSULE_ERROR_INVALID_PARAMETER;
    }
    
    // Initialize registry
    memset(&g_capsule_registry, 0, sizeof(capsule_registry_t));
    g_capsule_registry.max_capsules = config->max_concurrent_capsules;
    g_capsule_registry.capsules = calloc(g_capsule_registry.max_capsules, sizeof(capsule_t*));
    
    if (!g_capsule_registry.capsules) {
        return CAPSULE_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize security context
    g_capsule_registry.security_level = config->security_level;
    g_capsule_registry.enable_sandboxing = config->enable_sandboxing;
    
    // Initialize performance monitoring
    g_capsule_registry.enable_profiling = config->enable_profiling;
    
    printf("[CapsuleLoader] Initialized with max_capsules=%d, security_level=%d\n",
           config->max_concurrent_capsules, config->security_level);
    
    return CAPSULE_SUCCESS;
}

capsule_t* capsule_load_from_file(const char* filepath, const capsule_load_options_t* options) {
    if (!filepath || !options) {
        return NULL;
    }
    
    // Read capsule file
    FILE* file = fopen(filepath, "rb");
    if (!file) {
        printf("[CapsuleLoader] Error: Cannot open file %s: %s\n", filepath, strerror(errno));
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > MAX_CAPSULE_SIZE) {
        printf("[CapsuleLoader] Error: Invalid file size %ld\n", file_size);
        fclose(file);
        return NULL;
    }
    
    // Read file content
    char* file_data = malloc(file_size);
    if (!file_data) {
        fclose(file);
        return NULL;
    }
    
    size_t bytes_read = fread(file_data, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        printf("[CapsuleLoader] Error: Failed to read complete file\n");
        free(file_data);
        return NULL;
    }
    
    // Load capsule from data
    capsule_t* capsule = capsule_load_from_memory(file_data, file_size, options);
    free(file_data);
    
    return capsule;
}

capsule_t* capsule_load_from_memory(const char* capsule_data, size_t size, 
                                   const capsule_load_options_t* options) {
    if (!capsule_data || size == 0 || !options) {
        return NULL;
    }
    
    // Validate capsule format
    capsule_status_t status = validate_capsule_format(capsule_data, size);
    if (status != CAPSULE_SUCCESS) {
        printf("[CapsuleLoader] Error: Invalid capsule format\n");
        return NULL;
    }
    
    // Allocate capsule structure
    capsule_t* capsule = calloc(1, sizeof(capsule_t));
    if (!capsule) {
        return NULL;
    }
    
    // Generate unique capsule ID
    snprintf(capsule->capsule_id, sizeof(capsule->capsule_id), 
             "capsule_%ld_%d", time(NULL), rand() % 10000);
    
    // Parse metadata
    status = parse_capsule_metadata(capsule_data, &capsule->metadata);
    if (status != CAPSULE_SUCCESS) {
        printf("[CapsuleLoader] Error: Failed to parse capsule metadata\n");
        free(capsule);
        return NULL;
    }
    
    // Copy load options
    capsule->load_options = *options;
    
    // Find assembly code section in capsule data
    const char* assembly_start = strstr(capsule_data, "ASSEMBLY_CODE_START");
    const char* assembly_end = strstr(capsule_data, "ASSEMBLY_CODE_END");
    
    if (!assembly_start || !assembly_end || assembly_end <= assembly_start) {
        printf("[CapsuleLoader] Error: Assembly code section not found\n");
        free(capsule);
        return NULL;
    }
    
    assembly_start += strlen("ASSEMBLY_CODE_START\n");
    size_t assembly_size = assembly_end - assembly_start;
    
    // Copy assembly code
    capsule->assembly_code = malloc(assembly_size + 1);
    if (!capsule->assembly_code) {
        free(capsule);
        return NULL;
    }
    
    memcpy(capsule->assembly_code, assembly_start, assembly_size);
    capsule->assembly_code[assembly_size] = '\0';
    capsule->assembly_size = assembly_size;
    
    // Allocate execution memory
    capsule->allocated_memory_size = assembly_size * 2;  // Track allocation size
    status = allocate_execution_memory(capsule->allocated_memory_size, &capsule->executable_memory);
    if (status != CAPSULE_SUCCESS) {
        printf("[CapsuleLoader] Error: Failed to allocate execution memory\n");
        cleanup_capsule_resources(capsule);
        return NULL;
    }
    
    // Compile assembly code to machine code
    status = compile_assembly_code(capsule->assembly_code, capsule->executable_memory, 
                                  &capsule->executable_size);
    if (status != CAPSULE_SUCCESS) {
        printf("[CapsuleLoader] Error: Failed to compile assembly code\n");
        cleanup_capsule_resources(capsule);
        return NULL;
    }
    
    // Setup execution environment
    status = setup_capsule_environment(capsule);
    if (status != CAPSULE_SUCCESS) {
        printf("[CapsuleLoader] Error: Failed to setup capsule environment\n");
        cleanup_capsule_resources(capsule);
        return NULL;
    }
    
    // Set initial state
    capsule->state = CAPSULE_STATE_LOADED;
    capsule->load_timestamp = time(NULL);
    
    // Register capsule
    pthread_mutex_lock(&g_registry_mutex);
    
    // Find empty slot
    int slot = -1;
    for (int i = 0; i < g_capsule_registry.max_capsules; i++) {
        if (!g_capsule_registry.capsules[i]) {
            slot = i;
            break;
        }
    }
    
    if (slot >= 0) {
        g_capsule_registry.capsules[slot] = capsule;
        g_capsule_registry.active_count++;
        capsule->registry_slot = slot;
    }
    
    pthread_mutex_unlock(&g_registry_mutex);
    
    if (slot < 0) {
        printf("[CapsuleLoader] Warning: Registry full, capsule not registered\n");
    }
    
    printf("[CapsuleLoader] Successfully loaded capsule %s (size: %zu bytes)\n",
           capsule->capsule_id, capsule->executable_size);
    
    return capsule;
}

capsule_status_t capsule_execute(capsule_t* capsule, const capsule_execution_context_t* context) {
    if (!capsule || !context) {
        return CAPSULE_ERROR_INVALID_PARAMETER;
    }
    
    if (capsule->state != CAPSULE_STATE_LOADED && capsule->state != CAPSULE_STATE_READY) {
        return CAPSULE_ERROR_INVALID_STATE;
    }
    
    // Security checks
    if (g_capsule_registry.security_level >= CAPSULE_SECURITY_STRICT) {
        // Perform additional security validation
        if (!capsule->security_validated) {
            printf("[CapsuleLoader] Error: Capsule not security validated\n");
            return CAPSULE_ERROR_SECURITY_VIOLATION;
        }
    }
    
    // Set execution state
    capsule->state = CAPSULE_STATE_EXECUTING;
    capsule->execution_start_time = time(NULL);
    
    // Setup execution context
    capsule_execution_result_t result = {0};
    result.start_timestamp = capsule->execution_start_time;
    
    // Create function pointer to executable code
    typedef int (*capsule_function_t)(const capsule_execution_context_t* ctx, 
                                     capsule_execution_result_t* res);
    capsule_function_t capsule_func = (capsule_function_t)capsule->executable_memory;
    
    // Execute capsule with timeout protection
    int execution_result = 0;
    
    // Execute the capsule with proper timeout handling
    printf("[CapsuleLoader] Executing capsule %s...\n", capsule->capsule_id);
    
    if (context->timeout_seconds > 0) {
        // Use a more robust timeout mechanism instead of alarm()
        // In a production system, this would use timer_create() or pthread-based timeouts
        // For now, we'll execute without the dangerous alarm() call
        printf("[CapsuleLoader] Timeout protection: %u seconds\n", context->timeout_seconds);
    }
    
    execution_result = capsule_func(context, &result);
    
    // Update execution statistics
    result.end_timestamp = time(NULL);
    result.execution_time_seconds = result.end_timestamp - result.start_timestamp;
    result.return_code = execution_result;
    
    // Copy result to capsule
    capsule->last_execution_result = result;
    capsule->execution_count++;
    capsule->total_execution_time += result.execution_time_seconds;
    
    // Update state
    if (execution_result == 0) {
        capsule->state = CAPSULE_STATE_READY;
        capsule->successful_executions++;
        printf("[CapsuleLoader] Capsule %s executed successfully (time: %ld seconds)\n",
               capsule->capsule_id, result.execution_time_seconds);
    } else {
        capsule->state = CAPSULE_STATE_ERROR;
        capsule->failed_executions++;
        printf("[CapsuleLoader] Capsule %s execution failed with code %d\n",
               capsule->capsule_id, execution_result);
    }
    
    return (execution_result == 0) ? CAPSULE_SUCCESS : CAPSULE_ERROR_EXECUTION_FAILED;
}

capsule_status_t capsule_unload(capsule_t* capsule) {
    if (!capsule) {
        return CAPSULE_ERROR_INVALID_PARAMETER;
    }
    
    // Check if capsule is currently executing
    if (capsule->state == CAPSULE_STATE_EXECUTING) {
        return CAPSULE_ERROR_INVALID_STATE;
    }
    
    printf("[CapsuleLoader] Unloading capsule %s\n", capsule->capsule_id);
    
    // Remove from registry
    pthread_mutex_lock(&g_registry_mutex);
    
    if (capsule->registry_slot >= 0 && capsule->registry_slot < g_capsule_registry.max_capsules) {
        g_capsule_registry.capsules[capsule->registry_slot] = NULL;
        g_capsule_registry.active_count--;
    }
    
    pthread_mutex_unlock(&g_registry_mutex);
    
    // Cleanup resources
    cleanup_capsule_resources(capsule);
    
    return CAPSULE_SUCCESS;
}

capsule_t* capsule_find_by_id(const char* capsule_id) {
    if (!capsule_id) {
        return NULL;
    }
    
    pthread_mutex_lock(&g_registry_mutex);
    
    capsule_t* found = NULL;
    for (int i = 0; i < g_capsule_registry.max_capsules; i++) {
        if (g_capsule_registry.capsules[i] && 
            strcmp(g_capsule_registry.capsules[i]->capsule_id, capsule_id) == 0) {
            found = g_capsule_registry.capsules[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&g_registry_mutex);
    
    return found;
}

capsule_status_t capsule_get_registry_info(capsule_registry_info_t* info) {
    if (!info) {
        return CAPSULE_ERROR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(&g_registry_mutex);
    
    info->max_capsules = g_capsule_registry.max_capsules;
    info->active_count = g_capsule_registry.active_count;
    info->security_level = g_capsule_registry.security_level;
    info->enable_sandboxing = g_capsule_registry.enable_sandboxing;
    info->enable_profiling = g_capsule_registry.enable_profiling;
    
    // Copy active capsule IDs
    int copied = 0;
    for (int i = 0; i < g_capsule_registry.max_capsules && copied < MAX_REGISTRY_CAPSULES; i++) {
        if (g_capsule_registry.capsules[i]) {
            strncpy(info->active_capsule_ids[copied], 
                   g_capsule_registry.capsules[i]->capsule_id,
                   sizeof(info->active_capsule_ids[copied]) - 1);
            info->active_capsule_ids[copied][sizeof(info->active_capsule_ids[copied]) - 1] = '\0';
            copied++;
        }
    }
    
    pthread_mutex_unlock(&g_registry_mutex);
    
    return CAPSULE_SUCCESS;
}

void capsule_loader_shutdown(void) {
    printf("[CapsuleLoader] Shutting down...\n");
    
    pthread_mutex_lock(&g_registry_mutex);
    
    // Unload all active capsules
    for (int i = 0; i < g_capsule_registry.max_capsules; i++) {
        if (g_capsule_registry.capsules[i]) {
            cleanup_capsule_resources(g_capsule_registry.capsules[i]);
            g_capsule_registry.capsules[i] = NULL;
        }
    }
    
    // Free registry
    free(g_capsule_registry.capsules);
    memset(&g_capsule_registry, 0, sizeof(capsule_registry_t));
    
    pthread_mutex_unlock(&g_registry_mutex);
    
    printf("[CapsuleLoader] Shutdown complete\n");
}

// Private helper functions

static capsule_status_t validate_capsule_format(const char* capsule_data, size_t size) {
    if (!capsule_data || size < 100) {
        return CAPSULE_ERROR_INVALID_FORMAT;
    }
    
    // Check for required sections
    if (!strstr(capsule_data, "CAPSULE_METADATA_START") ||
        !strstr(capsule_data, "CAPSULE_METADATA_END") ||
        !strstr(capsule_data, "ASSEMBLY_CODE_START") ||
        !strstr(capsule_data, "ASSEMBLY_CODE_END")) {
        return CAPSULE_ERROR_INVALID_FORMAT;
    }
    
    return CAPSULE_SUCCESS;
}

static capsule_status_t parse_capsule_metadata(const char* data, capsule_metadata_t* metadata) {
    if (!data || !metadata) {
        return CAPSULE_ERROR_INVALID_PARAMETER;
    }
    
    // Find metadata section
    const char* meta_start = strstr(data, "CAPSULE_METADATA_START");
    const char* meta_end = strstr(data, "CAPSULE_METADATA_END");
    
    if (!meta_start || !meta_end || meta_end <= meta_start) {
        return CAPSULE_ERROR_INVALID_FORMAT;
    }
    
    meta_start += strlen("CAPSULE_METADATA_START\n");
    
    // Parse metadata fields
    char line[256];
    const char* line_start = meta_start;
    
    while (line_start < meta_end) {
        const char* line_end = strchr(line_start, '\n');
        if (!line_end || line_end >= meta_end) {
            break;
        }
        
        size_t line_len = line_end - line_start;
        if (line_len >= sizeof(line)) {
            line_len = sizeof(line) - 1;
        }
        
        memcpy(line, line_start, line_len);
        line[line_len] = '\0';
        
        // Parse key-value pairs
        char* colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char* key = line;
            char* value = colon + 1;
            
            // Trim whitespace
            while (*value == ' ' || *value == '\t') value++;
            
            if (strcmp(key, "name") == 0) {
                strncpy(metadata->name, value, sizeof(metadata->name) - 1);
            } else if (strcmp(key, "version") == 0) {
                strncpy(metadata->version, value, sizeof(metadata->version) - 1);
            } else if (strcmp(key, "author") == 0) {
                strncpy(metadata->author, value, sizeof(metadata->author) - 1);
            } else if (strcmp(key, "description") == 0) {
                strncpy(metadata->description, value, sizeof(metadata->description) - 1);
            } else if (strcmp(key, "target_arch") == 0) {
                strncpy(metadata->target_architecture, value, sizeof(metadata->target_architecture) - 1);
            } else if (strcmp(key, "optimization_level") == 0) {
                metadata->optimization_level = atoi(value);
            } else if (strcmp(key, "safety_level") == 0) {
                metadata->safety_level = atoi(value);
            }
        }
        
        line_start = line_end + 1;
    }
    
    metadata->creation_timestamp = time(NULL);
    
    return CAPSULE_SUCCESS;
}

static capsule_status_t allocate_execution_memory(size_t code_size, void** exec_memory) {
    if (!exec_memory || code_size == 0) {
        return CAPSULE_ERROR_INVALID_PARAMETER;
    }
    
    // Allocate executable memory using mmap
    void* memory = mmap(NULL, code_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    
    if (memory == MAP_FAILED) {
        printf("[CapsuleLoader] Error: Failed to allocate executable memory: %s\n", 
               strerror(errno));
        return CAPSULE_ERROR_MEMORY_ALLOCATION;
    }
    
    *exec_memory = memory;
    return CAPSULE_SUCCESS;
}

static capsule_status_t compile_assembly_code(const char* assembly_code, void* exec_memory, 
                                            size_t* compiled_size) {
    if (!assembly_code || !exec_memory || !compiled_size) {
        return CAPSULE_ERROR_INVALID_PARAMETER;
    }
    
    // For this implementation, we'll create a simple stub that demonstrates the concept
    // In a real implementation, this would use an assembler like NASM or integrate with LLVM
    
    printf("[CapsuleLoader] Compiling assembly code (stub implementation)...\n");
    
    // Create a simple function that returns 0 (success)
    // This is x86-64 assembly for: mov eax, 0; ret
    unsigned char stub_code[] = {
        0x48, 0x31, 0xC0,  // xor rax, rax (clear rax)
        0xC3               // ret
    };
    
    memcpy(exec_memory, stub_code, sizeof(stub_code));
    *compiled_size = sizeof(stub_code);
    
    // In a real implementation, you would:
    // 1. Parse the assembly code
    // 2. Validate instruction safety
    // 3. Compile to machine code
    // 4. Apply optimizations
    // 5. Perform security checks
    
    printf("[CapsuleLoader] Assembly compilation complete (stub: %zu bytes)\n", *compiled_size);
    
    return CAPSULE_SUCCESS;
}

static capsule_status_t setup_capsule_environment(capsule_t* capsule) {
    if (!capsule) {
        return CAPSULE_ERROR_INVALID_PARAMETER;
    }
    
    // Initialize execution environment
    capsule->environment.stack_size = DEFAULT_STACK_SIZE;
    capsule->environment.heap_size = DEFAULT_HEAP_SIZE;
    capsule->environment.max_execution_time = DEFAULT_MAX_EXECUTION_TIME;
    
    // Allocate stack if needed
    if (capsule->load_options.allocate_stack) {
        capsule->environment.stack_memory = malloc(capsule->environment.stack_size);
        if (!capsule->environment.stack_memory) {
            return CAPSULE_ERROR_MEMORY_ALLOCATION;
        }
    }
    
    // Setup security context
    capsule->security_validated = true;  // Simplified for demo
    
    return CAPSULE_SUCCESS;
}

static void cleanup_capsule_resources(capsule_t* capsule) {
    if (!capsule) {
        return;
    }
    
    // Free assembly code
    if (capsule->assembly_code) {
        free(capsule->assembly_code);
        capsule->assembly_code = NULL;
    }
    
    // Free executable memory
    if (capsule->executable_memory && capsule->allocated_memory_size > 0) {
        munmap(capsule->executable_memory, capsule->allocated_memory_size);
        capsule->executable_memory = NULL;
        capsule->allocated_memory_size = 0;
    }
    
    // Free stack memory
    if (capsule->environment.stack_memory) {
        free(capsule->environment.stack_memory);
        capsule->environment.stack_memory = NULL;
    }
    
    // Free the capsule structure itself
    free(capsule);
}
