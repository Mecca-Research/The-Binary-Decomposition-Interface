/**
 * @file test_profile.c
 * @brief Tests for Profile Rotation
 */

#include "../profiles/profile_rotation.h"
#include <stdio.h>
#include <assert.h>

void test_profile_initialization(void) {
    printf("Testing Profile initialization... ");
    
    profile_context_t* ctx = profile_initialize();
    assert(ctx != NULL);
    
    profile_shutdown(ctx);
    printf("PASSED\n");
}

void test_profile_selection(void) {
    printf("Testing Profile selection... ");
    
    profile_context_t* ctx = profile_initialize();
    
    // Test conservative selection for critical module
    profile_type_t profile = profile_select_for_task(
        ctx, TASK_BUG_FIX, 25, true
    );
    assert(profile == PROFILE_CONSERVATIVE);
    
    // Test aggressive selection for optimization
    profile = profile_select_for_task(
        ctx, TASK_OPTIMIZATION, 10, false
    );
    assert(profile == PROFILE_AGGRESSIVE);
    
    profile_shutdown(ctx);
    printf("PASSED\n");
}

void test_profile_config(void) {
    printf("Testing Profile configuration... ");
    
    profile_context_t* ctx = profile_initialize();
    
    profile_config_t config;
    crrss_status_t status = profile_get_config(
        ctx, PROFILE_CONSERVATIVE, &config
    );
    
    assert(status == CRRSS_STATUS_SUCCESS);
    assert(config.type == PROFILE_CONSERVATIVE);
    assert(config.safety_weight == 1.0);
    
    profile_shutdown(ctx);
    printf("PASSED\n");
}

int main(void) {
    printf("=== Profile Rotation Test Suite ===\n");
    
    test_profile_initialization();
    test_profile_selection();
    test_profile_config();
    
    printf("\n✓ All Profile tests passed!\n");
    return 0;
}
