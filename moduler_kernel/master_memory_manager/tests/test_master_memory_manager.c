
/**
 * @file test_master_memory_manager.c
 * @brief Master Memory Manager Test Suite
 * 
 * Comprehensive test suite for Master Memory Manager Phase 1 implementation
 * testing all core x86 competencies and HAL framework components.
 * 
 * @author BDI Development Team
 * @date September 29, 2025
 * @version 1.0.0
 */

#include "../master_memory_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// =============================================================================
// TEST FRAMEWORK
// =============================================================================

static int g_tests_run = 0;
static int g_tests_passed = 0;
static int g_tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        g_tests_run++; \
        if (condition) { \
            g_tests_passed++; \
            printf("PASS: %s\n", message); \
        } else { \
            g_tests_failed++; \
            printf("FAIL: %s\n", message); \
        } \
    } while(0)

// =============================================================================
// TEST FUNCTIONS
// =============================================================================

void test_mmm_initialization(void)
{
    printf("\n=== Testing Master Memory Manager Initialization ===\n");
    
    mmm_config_t config = {
        .enable_x86_core = true,
        .enable_hal_framework = true,
        .enable_debug_mode = true,
        .enable_performance_opt = true,
        .memory_pool_size = 1024 * 1024, // 1MB
        .tlb_cache_size = 64,
        .page_size = 4096
    };
    
    mmm_status_t status = mmm_initialize(&config);
    TEST_ASSERT(status == MMM_SUCCESS, "MMM initialization should succeed");
    
    mmm_context_t *context = mmm_get_context();
    TEST_ASSERT(context != NULL, "MMM context should be available");
    TEST_ASSERT(context->initialized == true, "MMM should be marked as initialized");
    
    const char *version = mmm_get_version();
    TEST_ASSERT(version != NULL, "MMM version should be available");
    TEST_ASSERT(strcmp(version, MMM_VERSION_STRING) == 0, "MMM version should match expected");
    
    status = mmm_shutdown();
    TEST_ASSERT(status == MMM_SUCCESS, "MMM shutdown should succeed");
}

void test_x86_registers(void)
{
    printf("\n=== Testing x86 Register Management ===\n");
    
    // Initialize MMM first
    mmm_config_t config = {
        .enable_x86_core = true,
        .enable_hal_framework = false,
        .memory_pool_size = 1024 * 1024,
        .tlb_cache_size = 64,
        .page_size = 4096
    };
    mmm_initialize(&config);
    
    // Test register allocation
    int reg_id = x86_allocate_register(32, 1, "test register");
    TEST_ASSERT(reg_id >= 0, "Register allocation should succeed");
    
    const x86_reg_alloc_entry_t *entry = x86_get_register_status(reg_id);
    TEST_ASSERT(entry != NULL, "Register status should be available");
    TEST_ASSERT(entry->status == X86_REG_ALLOCATED, "Register should be marked as allocated");
    
    int result = x86_free_register(reg_id, 1);
    TEST_ASSERT(result == 0, "Register deallocation should succeed");
    
    const char *reg_name = x86_get_register_name(0, false);
    TEST_ASSERT(reg_name != NULL, "Register name should be available");
    
    bool is_volatile = x86_is_register_volatile(0); // EAX
    TEST_ASSERT(is_volatile == true, "EAX should be volatile");
    
    mmm_shutdown();
}

void test_x86_calling_abi(void)
{
    printf("\n=== Testing x86 Calling Convention and ABI ===\n");
    
    mmm_config_t config = {
        .enable_x86_core = true,
        .enable_hal_framework = false,
        .memory_pool_size = 1024 * 1024,
        .tlb_cache_size = 64,
        .page_size = 4096
    };
    mmm_initialize(&config);
    
    // Test function signature creation
    size_t param_types[] = {4, 4, 8}; // int, int, long long
    x86_function_signature_t *sig = x86_create_function_signature(
        X86_CALL_CONV_CDECL, param_types, 3, 4, false);
    
    TEST_ASSERT(sig != NULL, "Function signature creation should succeed");
    TEST_ASSERT(sig->convention == X86_CALL_CONV_CDECL, "Calling convention should be set correctly");
    TEST_ASSERT(sig->param_count == 3, "Parameter count should be correct");
    
    bool is_valid = x86_validate_function_signature(sig);
    TEST_ASSERT(is_valid == true, "Function signature should be valid");
    
    const char *conv_name = x86_get_calling_convention_name(X86_CALL_CONV_CDECL);
    TEST_ASSERT(conv_name != NULL, "Calling convention name should be available");
    
    bool uses_shadow = x86_uses_shadow_space(X86_CALL_CONV_X64_MS);
    TEST_ASSERT(uses_shadow == true, "x64 MS calling convention should use shadow space");
    
    x86_destroy_function_signature(sig);
    mmm_shutdown();
}

void test_x86_paging_mmu(void)
{
    printf("\n=== Testing x86 Paging and MMU ===\n");
    
    mmm_config_t config = {
        .enable_x86_core = true,
        .enable_hal_framework = false,
        .memory_pool_size = 1024 * 1024,
        .tlb_cache_size = 64,
        .page_size = 4096
    };
    mmm_initialize(&config);
    
    // Test page directory creation
    x86_page_directory_t *page_dir = x86_create_page_directory();
    TEST_ASSERT(page_dir != NULL, "Page directory creation should succeed");
    
    // Test page mapping
    uint32_t virtual_addr = 0x10000000;
    uint32_t physical_addr = 0x20000000;
    uint32_t flags = X86_PAGE_PRESENT | X86_PAGE_WRITABLE;
    
    int result = x86_map_page(page_dir, virtual_addr, physical_addr, flags);
    TEST_ASSERT(result == 0, "Page mapping should succeed");
    
    bool is_present = x86_is_page_present(page_dir, virtual_addr);
    TEST_ASSERT(is_present == true, "Mapped page should be present");
    
    // Test address translation
    uint32_t translated_addr;
    result = x86_translate_address(page_dir, virtual_addr, &translated_addr);
    TEST_ASSERT(result == 0, "Address translation should succeed");
    
    // Test page unmapping
    result = x86_unmap_page(page_dir, virtual_addr);
    TEST_ASSERT(result == 0, "Page unmapping should succeed");
    
    is_present = x86_is_page_present(page_dir, virtual_addr);
    TEST_ASSERT(is_present == false, "Unmapped page should not be present");
    
    x86_destroy_page_directory(page_dir);
    mmm_shutdown();
}

void test_hal_framework(void)
{
    printf("\n=== Testing HAL Framework ===\n");
    
    mmm_config_t config = {
        .enable_x86_core = false,
        .enable_hal_framework = true,
        .memory_pool_size = 1024 * 1024,
        .tlb_cache_size = 64,
        .page_size = 4096
    };
    mmm_initialize(&config);
    
    // Test BSP functionality
    const mmm_bsp_board_info_t *board_info = mmm_bsp_get_board_info();
    TEST_ASSERT(board_info != NULL, "Board info should be available");
    TEST_ASSERT(board_info->board_name != NULL, "Board name should be set");
    
    // Test GPIO configuration
    mmm_bsp_gpio_config_t gpio_config = {
        .pin_number = 5,
        .direction = MMM_BSP_GPIO_OUTPUT,
        .pull = MMM_BSP_GPIO_PULL_NONE,
        .open_drain = false,
        .high_speed = false,
        .alternate_function = 0
    };
    
    int result = mmm_bsp_gpio_configure(&gpio_config);
    TEST_ASSERT(result == 0, "GPIO configuration should succeed");
    
    result = mmm_bsp_gpio_set_state(5, MMM_BSP_GPIO_HIGH);
    TEST_ASSERT(result == 0, "GPIO state setting should succeed");
    
    // Test peripheral drivers
    result = MMM_OSCILLATOR_Initialize();
    TEST_ASSERT(result == 0, "Oscillator initialization should succeed");
    
    result = MMM_OSCILLATOR_SetFrequency(100000000);
    TEST_ASSERT(result == 0, "Oscillator frequency setting should succeed");
    
    uint32_t freq = MMM_OSCILLATOR_GetFrequency();
    TEST_ASSERT(freq == 100000000, "Oscillator frequency should be correct");
    
    mmm_shutdown();
}

void test_cache_hints(void)
{
    printf("\n=== Testing Cache Hints and Optimization ===\n");
    
    mmm_config_t config = {
        .enable_x86_core = true,
        .enable_hal_framework = false,
        .memory_pool_size = 1024 * 1024,
        .tlb_cache_size = 64,
        .page_size = 4096
    };
    mmm_initialize(&config);
    
    // Test cache configuration
    const x86_cache_config_t *cache_config = x86_cache_get_config();
    TEST_ASSERT(cache_config != NULL, "Cache configuration should be available");
    
    // Test cache hint addition
    char test_data[1024];
    x86_cache_hint_t hint = {
        .start_addr = test_data,
        .size = sizeof(test_data),
        .policy = X86_CACHE_POLICY_WRITE_BACK,
        .pattern = X86_ACCESS_PATTERN_SEQUENTIAL,
        .prefetch = X86_PREFETCH_T0,
        .align_to_cache_line = true,
        .avoid_false_sharing = true
    };
    
    int result = x86_cache_add_hint(&hint);
    TEST_ASSERT(result == 0, "Cache hint addition should succeed");
    
    // Test cache prefetching
    result = x86_cache_prefetch(test_data, X86_PREFETCH_T0);
    TEST_ASSERT(result == 0, "Cache prefetching should succeed");
    
    // Test cache alignment check
    bool is_aligned = x86_cache_is_aligned(test_data);
    // Note: test_data may or may not be aligned, so we just check the function works
    (void)is_aligned;
    
    // Test cache statistics
    const x86_cache_stats_t *stats = x86_cache_get_stats();
    TEST_ASSERT(stats != NULL, "Cache statistics should be available");
    
    result = x86_cache_remove_hint(test_data);
    TEST_ASSERT(result == 0, "Cache hint removal should succeed");
    
    mmm_shutdown();
}

void test_error_handling(void)
{
    printf("\n=== Testing Error Handling ===\n");
    
    // Test invalid configuration
    mmm_config_t invalid_config = {
        .enable_x86_core = true,
        .enable_hal_framework = true,
        .memory_pool_size = 0, // Invalid
        .tlb_cache_size = 0,   // Invalid
        .page_size = 1000      // Invalid (not power of 2)
    };
    
    mmm_status_t status = mmm_initialize(&invalid_config);
    TEST_ASSERT(status != MMM_SUCCESS, "Invalid configuration should be rejected");
    
    // Test operations without initialization
    mmm_context_t *context = mmm_get_context();
    TEST_ASSERT(context == NULL, "Context should not be available without initialization");
    
    // Test status string conversion
    const char *status_str = mmm_status_to_string(MMM_ERROR_INVALID_PARAM);
    TEST_ASSERT(status_str != NULL, "Status string should be available");
    TEST_ASSERT(strlen(status_str) > 0, "Status string should not be empty");
}

void run_all_tests(void)
{
    printf("Master Memory Manager Phase 1 Test Suite\n");
    printf("=========================================\n");
    
    test_mmm_initialization();
    test_x86_registers();
    test_x86_calling_abi();
    test_x86_paging_mmu();
    test_hal_framework();
    test_cache_hints();
    test_error_handling();
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", g_tests_run);
    printf("Tests passed: %d\n", g_tests_passed);
    printf("Tests failed: %d\n", g_tests_failed);
    printf("Success rate: %.1f%%\n", (float)g_tests_passed / g_tests_run * 100.0f);
    
    if (g_tests_failed == 0) {
        printf("\nAll tests PASSED! ✓\n");
    } else {
        printf("\nSome tests FAILED! ✗\n");
    }
}

int main(void)
{
    run_all_tests();
    return (g_tests_failed == 0) ? 0 : 1;
}
