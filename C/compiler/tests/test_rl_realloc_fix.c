/**
 * @file test_rl_realloc_fix.c
 * @brief Test for RL Framework realloc bug fix
 * 
 * This test verifies that the metrics array resize works correctly
 * and doesn't cause use-after-free or memory leaks.
 */

#include "../AIBase/reinforcement/rl_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Test that metrics can handle capacity expansion
void test_metrics_resize(void) {
    printf("Testing metrics resize...\n");
    
    // Create metrics with small initial capacity
    size_t initial_capacity = 10;
    RLMetrics* metrics = rl_metrics_create(initial_capacity);
    assert(metrics != nullptr);
    assert(metrics->capacity == initial_capacity);
    assert(metrics->num_episodes == 0);
    
    // Add episodes up to and beyond capacity to trigger resize
    size_t num_episodes = 25; // More than double the initial capacity
    
    for (size_t i = 0; i < num_episodes; i++) {
        double reward = (double)i * 10.0;
        size_t length = i + 1;
        double value = (double)i * 5.0;
        
        rl_metrics_add_episode(metrics, reward, length, value);
        
        // Verify episode was added
        assert(metrics->num_episodes == i + 1);
        
        // Verify data integrity
        assert(metrics->episode_rewards[i] == reward);
        assert(metrics->episode_lengths[i] == length);
        assert(metrics->episode_values[i] == value);
    }
    
    // Verify capacity was expanded
    assert(metrics->capacity >= num_episodes);
    assert(metrics->num_episodes == num_episodes);
    
    // Verify all data is still intact after resizes
    for (size_t i = 0; i < num_episodes; i++) {
        assert(metrics->episode_rewards[i] == (double)i * 10.0);
        assert(metrics->episode_lengths[i] == i + 1);
        assert(metrics->episode_values[i] == (double)i * 5.0);
    }
    
    // Compute statistics to ensure no crashes
    rl_metrics_compute_statistics(metrics);
    
    printf("  Average reward: %.2f\n", metrics->average_reward);
    printf("  Best reward: %.2f\n", metrics->best_reward);
    printf("  Worst reward: %.2f\n", metrics->worst_reward);
    
    rl_metrics_destroy(metrics);
    
    printf("✓ Metrics resize test passed\n\n");
}

// Test multiple resize cycles
void test_multiple_resizes(void) {
    printf("Testing multiple resize cycles...\n");
    
    // Start with very small capacity
    size_t initial_capacity = 2;
    RLMetrics* metrics = rl_metrics_create(initial_capacity);
    assert(metrics != nullptr);
    
    // Add many episodes to trigger multiple resizes
    size_t num_episodes = 100;
    
    for (size_t i = 0; i < num_episodes; i++) {
        rl_metrics_add_episode(metrics, (double)i, i, (double)i);
    }
    
    assert(metrics->num_episodes == num_episodes);
    
    // Verify all data is correct
    for (size_t i = 0; i < num_episodes; i++) {
        assert(metrics->episode_rewards[i] == (double)i);
        assert(metrics->episode_lengths[i] == i);
        assert(metrics->episode_values[i] == (double)i);
    }
    
    rl_metrics_destroy(metrics);
    
    printf("✓ Multiple resize cycles test passed\n\n");
}

// Test edge case: exactly at capacity boundary
void test_capacity_boundary(void) {
    printf("Testing capacity boundary conditions...\n");
    
    size_t capacity = 5;
    RLMetrics* metrics = rl_metrics_create(capacity);
    assert(metrics != nullptr);
    
    // Fill exactly to capacity
    for (size_t i = 0; i < capacity; i++) {
        rl_metrics_add_episode(metrics, (double)i, i, (double)i);
    }
    
    assert(metrics->num_episodes == capacity);
    assert(metrics->capacity == capacity);
    
    // Add one more to trigger resize
    rl_metrics_add_episode(metrics, 100.0, 100, 100.0);
    
    assert(metrics->num_episodes == capacity + 1);
    assert(metrics->capacity > capacity); // Should have expanded
    
    // Verify last element
    assert(metrics->episode_rewards[capacity] == 100.0);
    assert(metrics->episode_lengths[capacity] == 100);
    assert(metrics->episode_values[capacity] == 100.0);
    
    rl_metrics_destroy(metrics);
    
    printf("✓ Capacity boundary test passed\n\n");
}

// Test with realistic training scenario
void test_realistic_training(void) {
    printf("Testing realistic training scenario...\n");
    
    RLMetrics* metrics = rl_metrics_create(100);
    assert(metrics != nullptr);
    
    // Simulate 500 episodes of training
    for (size_t episode = 0; episode < 500; episode++) {
        // Simulate improving rewards over time
        double reward = -100.0 + (episode * 0.4);
        size_t length = 50 + (episode % 20);
        double value = reward * 0.5;
        
        rl_metrics_add_episode(metrics, reward, length, value);
    }
    
    assert(metrics->num_episodes == 500);
    
    // Compute and verify statistics
    rl_metrics_compute_statistics(metrics);
    
    printf("  Episodes: %zu\n", metrics->num_episodes);
    printf("  Average reward: %.2f ± %.2f\n", 
           metrics->average_reward, metrics->reward_std);
    printf("  Best reward: %.2f\n", metrics->best_reward);
    printf("  Worst reward: %.2f\n", metrics->worst_reward);
    
    // Verify statistics are reasonable
    assert(metrics->best_reward > metrics->worst_reward);
    assert(metrics->average_reward > metrics->worst_reward);
    assert(metrics->average_reward < metrics->best_reward);
    
    rl_metrics_destroy(metrics);
    
    printf("✓ Realistic training scenario test passed\n\n");
}

int main(void) {
    printf("=== RL Framework Realloc Fix Tests ===\n\n");
    
    test_metrics_resize();
    test_multiple_resizes();
    test_capacity_boundary();
    test_realistic_training();
    
    printf("=== All RL Framework Realloc Tests Passed ===\n");
    
    return 0;
}
