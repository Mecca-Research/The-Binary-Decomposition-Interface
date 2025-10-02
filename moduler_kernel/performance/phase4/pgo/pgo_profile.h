/**
 * @file pgo_profile.h
 * @brief Profile-Guided Optimization infrastructure
 */

#ifndef PHASE4_PGO_PROFILE_H
#define PHASE4_PGO_PROFILE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize PGO profiling
 * @return 0 on success, negative on error
 */
int pgo_init(void);

/**
 * @brief Shutdown PGO profiling
 */
void pgo_shutdown(void);

/**
 * @brief Start profile collection
 * @param output_file Output profile file path
 * @return 0 on success, negative on error
 */
int pgo_start_profiling(const char* output_file);

/**
 * @brief Stop profile collection
 * @return 0 on success, negative on error
 */
int pgo_stop_profiling(void);

/**
 * @brief Merge multiple profile files
 * @param input_files Array of input profile files
 * @param num_files Number of input files
 * @param output_file Output merged profile file
 * @return 0 on success, negative on error
 */
int pgo_merge_profiles(const char** input_files, size_t num_files,
                       const char* output_file);

#ifdef __cplusplus
}
#endif

#endif
