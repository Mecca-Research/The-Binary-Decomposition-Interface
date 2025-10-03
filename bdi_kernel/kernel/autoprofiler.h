
/**
 * BDI Kernel Autoprofiler - Phase 6
 * Automatic profiling for PGO integration
 */

#ifndef BDI_AUTOPROFILER_H
#define BDI_AUTOPROFILER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ============================================================================
 * Autoprofiler Configuration
 * ============================================================================ */

/* Maximum number of profiling points */
#define AUTOPROFILER_MAX_POINTS 10000

/* Profile data collection interval (in cycles) */
#define AUTOPROFILER_SAMPLE_INTERVAL 1000000

/* Profile data file path */
#define AUTOPROFILER_DATA_FILE "./pgo-data/autoprofiler.dat"

/* ============================================================================
 * Profiling Point Types
 * ============================================================================ */

typedef enum {
    PROFILE_FUNCTION_ENTRY,
    PROFILE_FUNCTION_EXIT,
    PROFILE_BRANCH_TAKEN,
    PROFILE_BRANCH_NOT_TAKEN,
    PROFILE_LOOP_ITERATION,
    PROFILE_CACHE_MISS,
    PROFILE_MEMORY_ACCESS,
} profile_point_type_t;

/* ============================================================================
 * Profiling Data Structures
 * ============================================================================ */

/**
 * Profiling point information
 */
typedef struct {
    const char* function_name;
    const char* file_name;
    uint32_t line_number;
    profile_point_type_t type;
    uint64_t hit_count;
    uint64_t total_cycles;
    uint64_t min_cycles;
    uint64_t max_cycles;
} profile_point_t;

/**
 * Autoprofiler context
 */
typedef struct {
    bool enabled;
    bool collecting;
    uint64_t start_time;
    uint64_t total_samples;
    uint32_t num_points;
    profile_point_t points[AUTOPROFILER_MAX_POINTS];
} autoprofiler_ctx_t;

/* ============================================================================
 * Autoprofiler API
 * ============================================================================ */

/**
 * Initialize the autoprofiler
 * @return 0 on success, -1 on error
 */
int autoprofiler_init(void);

/**
 * Start profiling
 * @return 0 on success, -1 on error
 */
int autoprofiler_start(void);

/**
 * Stop profiling
 * @return 0 on success, -1 on error
 */
int autoprofiler_stop(void);

/**
 * Save profiling data to file
 * @param filename Output file path (NULL for default)
 * @return 0 on success, -1 on error
 */
int autoprofiler_save(const char* filename);

/**
 * Load profiling data from file
 * @param filename Input file path (NULL for default)
 * @return 0 on success, -1 on error
 */
int autoprofiler_load(const char* filename);

/**
 * Reset profiling data
 */
void autoprofiler_reset(void);

/**
 * Register a profiling point
 * @param function_name Function name
 * @param file_name Source file name
 * @param line_number Line number
 * @param type Profiling point type
 * @return Profile point ID, or -1 on error
 */
int autoprofiler_register_point(
    const char* function_name,
    const char* file_name,
    uint32_t line_number,
    profile_point_type_t type
);

/**
 * Record a profiling event
 * @param point_id Profile point ID
 * @param cycles Number of cycles elapsed
 */
void autoprofiler_record(int point_id, uint64_t cycles);

/**
 * Get profiling statistics
 * @param point_id Profile point ID
 * @return Pointer to profile point data, or NULL if not found
 */
const profile_point_t* autoprofiler_get_stats(int point_id);

/**
 * Print profiling report
 */
void autoprofiler_print_report(void);

/**
 * Export profiling data in GCC PGO format
 * @param output_dir Output directory for PGO data
 * @return 0 on success, -1 on error
 */
int autoprofiler_export_pgo(const char* output_dir);

/* ============================================================================
 * Profiling Macros
 * ============================================================================ */

#ifdef ENABLE_AUTOPROFILER

/* Profile function entry/exit */
#define PROFILE_FUNCTION_START() \
    static int __profile_id = -1; \
    uint64_t __profile_start = 0; \
    if (__profile_id == -1) { \
        __profile_id = autoprofiler_register_point( \
            __func__, __FILE__, __LINE__, PROFILE_FUNCTION_ENTRY); \
    } \
    if (__profile_id >= 0) { \
        __profile_start = __builtin_ia32_rdtsc(); \
    }

#define PROFILE_FUNCTION_END() \
    if (__profile_id >= 0) { \
        uint64_t __profile_end = __builtin_ia32_rdtsc(); \
        autoprofiler_record(__profile_id, __profile_end - __profile_start); \
    }

/* Profile branch */
#define PROFILE_BRANCH(condition, taken) \
    do { \
        static int __branch_id = -1; \
        if (__branch_id == -1) { \
            __branch_id = autoprofiler_register_point( \
                __func__, __FILE__, __LINE__, \
                (taken) ? PROFILE_BRANCH_TAKEN : PROFILE_BRANCH_NOT_TAKEN); \
        } \
        if (__branch_id >= 0 && (condition)) { \
            autoprofiler_record(__branch_id, 1); \
        } \
    } while(0)

/* Profile loop iteration */
#define PROFILE_LOOP_ITERATION() \
    do { \
        static int __loop_id = -1; \
        if (__loop_id == -1) { \
            __loop_id = autoprofiler_register_point( \
                __func__, __FILE__, __LINE__, PROFILE_LOOP_ITERATION); \
        } \
        if (__loop_id >= 0) { \
            autoprofiler_record(__loop_id, 1); \
        } \
    } while(0)

#else

/* Disabled profiling macros */
#define PROFILE_FUNCTION_START()
#define PROFILE_FUNCTION_END()
#define PROFILE_BRANCH(condition, taken)
#define PROFILE_LOOP_ITERATION()

#endif /* ENABLE_AUTOPROFILER */

/* ============================================================================
 * Integration with GCC PGO
 * ============================================================================ */

/**
 * Initialize PGO runtime
 * Called automatically by GCC instrumented code
 */
void __gcov_init(void* info);

/**
 * Flush PGO data
 * Called automatically at program exit
 */
void __gcov_flush(void);

/**
 * Merge PGO data from multiple runs
 * @param profile_dir Directory containing .gcda files
 * @return 0 on success, -1 on error
 */
int autoprofiler_merge_pgo_data(const char* profile_dir);

#endif /* BDI_AUTOPROFILER_H */
