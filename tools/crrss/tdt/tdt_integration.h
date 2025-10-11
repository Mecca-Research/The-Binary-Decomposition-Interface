
/**
 * @file tdt_integration.h
 * @brief CRRSS Integration Layer - Integration with MSM, STP, and BPME
 * 
 * Phase 2 Stage 2 Implementation
 */

#ifndef TDT_INTEGRATION_H
#define TDT_INTEGRATION_H

#include "tdt.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==================== MSM Integration ====================

/**
 * @brief Generate tests for memory safety issues detected by MSM
 * @param ctx TDT context
 * @param msm_ctx MSM context
 * @param file_path File to generate tests for
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_msm_generate_memory_safety_tests(
    tdt_context_t* ctx,
    void* msm_ctx,
    const char* file_path,
    tdt_generation_result_t* result
);

/**
 * @brief Generate tests for memory leak patterns
 * @param ctx TDT context
 * @param msm_ctx MSM context
 * @param file_path File to test
 * @param test_cases Output test cases
 * @param max_tests Maximum tests to generate
 * @param num_tests Number of tests generated
 * @return Status code
 */
crrss_status_t tdt_msm_generate_leak_tests(
    tdt_context_t* ctx,
    void* msm_ctx,
    const char* file_path,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

/**
 * @brief Generate tests for use-after-free patterns
 * @param ctx TDT context
 * @param msm_ctx MSM context
 * @param file_path File to test
 * @param test_cases Output test cases
 * @param max_tests Maximum tests to generate
 * @param num_tests Number of tests generated
 * @return Status code
 */
crrss_status_t tdt_msm_generate_uaf_tests(
    tdt_context_t* ctx,
    void* msm_ctx,
    const char* file_path,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

/**
 * @brief Generate tests for buffer overflow scenarios
 * @param ctx TDT context
 * @param msm_ctx MSM context
 * @param file_path File to test
 * @param test_cases Output test cases
 * @param max_tests Maximum tests to generate
 * @param num_tests Number of tests generated
 * @return Status code
 */
crrss_status_t tdt_msm_generate_overflow_tests(
    tdt_context_t* ctx,
    void* msm_ctx,
    const char* file_path,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

// ==================== STP Integration ====================

/**
 * @brief Generate tests for type safety issues detected by STP
 * @param ctx TDT context
 * @param stp_ctx STP context
 * @param file_path File to generate tests for
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_stp_generate_type_safety_tests(
    tdt_context_t* ctx,
    void* stp_ctx,
    const char* file_path,
    tdt_generation_result_t* result
);

/**
 * @brief Generate tests for type conversion issues
 * @param ctx TDT context
 * @param stp_ctx STP context
 * @param file_path File to test
 * @param test_cases Output test cases
 * @param max_tests Maximum tests to generate
 * @param num_tests Number of tests generated
 * @return Status code
 */
crrss_status_t tdt_stp_generate_conversion_tests(
    tdt_context_t* ctx,
    void* stp_ctx,
    const char* file_path,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

/**
 * @brief Generate tests for struct alignment issues
 * @param ctx TDT context
 * @param stp_ctx STP context
 * @param file_path File to test
 * @param test_cases Output test cases
 * @param max_tests Maximum tests to generate
 * @param num_tests Number of tests generated
 * @return Status code
 */
crrss_status_t tdt_stp_generate_alignment_tests(
    tdt_context_t* ctx,
    void* stp_ctx,
    const char* file_path,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

// ==================== BPME Integration ====================

/**
 * @brief Generate tests for bug patterns detected by BPME
 * @param ctx TDT context
 * @param bpme_ctx BPME context
 * @param file_path File to generate tests for
 * @param result Output generation result
 * @return Status code
 */
crrss_status_t tdt_bpme_generate_bug_pattern_tests(
    tdt_context_t* ctx,
    void* bpme_ctx,
    const char* file_path,
    tdt_generation_result_t* result
);

/**
 * @brief Generate tests for predicted bugs
 * @param ctx TDT context
 * @param bpme_ctx BPME context
 * @param file_path File to test
 * @param test_cases Output test cases
 * @param max_tests Maximum tests to generate
 * @param num_tests Number of tests generated
 * @return Status code
 */
crrss_status_t tdt_bpme_generate_prediction_tests(
    tdt_context_t* ctx,
    void* bpme_ctx,
    const char* file_path,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

/**
 * @brief Generate tests for error handling paths
 * @param ctx TDT context
 * @param bpme_ctx BPME context
 * @param file_path File to test
 * @param test_cases Output test cases
 * @param max_tests Maximum tests to generate
 * @param num_tests Number of tests generated
 * @return Status code
 */
crrss_status_t tdt_bpme_generate_error_path_tests(
    tdt_context_t* ctx,
    void* bpme_ctx,
    const char* file_path,
    tdt_test_case_t* test_cases,
    uint32_t max_tests,
    uint32_t* num_tests
);

#ifdef __cplusplus
}
#endif

#endif // TDT_INTEGRATION_H
