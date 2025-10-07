
/**
 * @file ml_rl_framework_test.c
 * @brief RL Framework Tests
 */

#include "../AIBase/reinforcement/rl_framework.h"
#include "../AIBase/reinforcement/q_learning.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

// Simple environment for testing
typedef struct {
    size_t current_state;
    size_t n_states;
    size_t n_actions;
} SimpleEnv;

static size_t simple_get_state(void* env_data) {
    SimpleEnv* env = (SimpleEnv*)env_data;
    return env->current_state;
}

static size_t simple_step(void* env_data, size_t action, double* reward, bool* done) {
    SimpleEnv* env = (SimpleEnv*)env_data;
    
    env->current_state = (env->current_state + action + 1) % env->n_states;
    
    if (env->current_state == env->n_states - 1) {
        *reward = 1.0;
        *done = true;
    } else {
        *reward = 0.0;
        *done = false;
    }
    
    return env->current_state;
}

static void simple_reset(void* env_data) {
    SimpleEnv* env = (SimpleEnv*)env_data;
    env->current_state = 0;
}

static size_t simple_get_state_space_size(void* env_data) {
    SimpleEnv* env = (SimpleEnv*)env_data;
    return env->n_states;
}

static size_t simple_get_action_space_size(void* env_data) {
    SimpleEnv* env = (SimpleEnv*)env_data;
    return env->n_actions;
}

void test_rl_environment(void) {
    printf("Testing RL environment...\n");
    
    SimpleEnv* env_data = malloc(sizeof(SimpleEnv));
    env_data->n_states = 5;
    env_data->n_actions = 2;
    env_data->current_state = 0;
    
    RLEnvironmentInterface interface = {
        .get_state = simple_get_state,
        .step = simple_step,
        .reset = simple_reset,
        .get_state_space_size = simple_get_state_space_size,
        .get_action_space_size = simple_get_action_space_size,
        .render = nullptr
    };
    
    RLEnvironment* env = rl_env_create(env_data, interface);
    assert(env != nullptr);
    
    size_t state = rl_env_reset(env);
    assert(state == 0);
    
    double reward;
    bool done;
    size_t next_state = rl_env_step(env, 0, &reward, &done);
    assert(next_state == 1);
    
    rl_env_destroy(env);
    free(env_data);
    
    printf("✓ RL environment test passed\n");
}

void test_rl_trajectory(void) {
    printf("Testing RL trajectory...\n");
    
    RLTrajectory* trajectory = rl_trajectory_create(10);
    assert(trajectory != nullptr);
    assert(trajectory->length == 0);
    
    // Add transitions
    RLTransition t1 = {.state = 0, .action = 1, .reward = 0.5, .next_state = 1, .done = false};
    RLTransition t2 = {.state = 1, .action = 0, .reward = 1.0, .next_state = 2, .done = true};
    
    rl_trajectory_add(trajectory, &t1);
    rl_trajectory_add(trajectory, &t2);
    
    assert(trajectory->length == 2);
    assert(fabs(trajectory->total_reward - 1.5) < 1e-6);
    assert(trajectory->complete);
    
    // Test return computation
    double ret = rl_trajectory_compute_return(trajectory, 0.9);
    // Expected: 0.5 + 0.9 * 1.0 = 1.4
    assert(fabs(ret - 1.4) < 1e-6);
    
    rl_trajectory_destroy(trajectory);
    
    printf("✓ RL trajectory test passed\n");
}

void test_rl_metrics(void) {
    printf("Testing RL metrics...\n");
    
    RLMetrics* metrics = rl_metrics_create(100);
    assert(metrics != nullptr);
    
    // Add episode data
    rl_metrics_add_episode(metrics, 10.0, 50, 5.0);
    rl_metrics_add_episode(metrics, 15.0, 45, 7.0);
    rl_metrics_add_episode(metrics, 12.0, 48, 6.0);
    
    assert(metrics->num_episodes == 3);
    
    rl_metrics_compute_statistics(metrics);
    
    // Average should be (10 + 15 + 12) / 3 = 12.33...
    assert(fabs(metrics->average_reward - 12.333333) < 0.01);
    assert(fabs(metrics->best_reward - 15.0) < 1e-6);
    assert(fabs(metrics->worst_reward - 10.0) < 1e-6);
    
    rl_metrics_destroy(metrics);
    
    printf("✓ RL metrics test passed\n");
}

void test_rl_agent_wrapper(void) {
    printf("Testing RL agent wrapper...\n");
    
    // Create Q-learning model as agent
    QLearningConfig config = qlearning_default_config();
    QLearningModel* qmodel = qlearning_create(5, 3, config);
    
    // Create agent interface
    RLAgentInterface interface = {
        .select_action = (size_t (*)(void*, size_t))qlearning_select_action,
        .update = nullptr, // Would need wrapper function
        .get_value = nullptr,
        .get_q_value = (double (*)(void*, size_t, size_t))qlearning_get_q_value
    };
    
    RLAgent* agent = rl_agent_create(qmodel, interface, 5, 3);
    assert(agent != nullptr);
    assert(agent->n_states == 5);
    assert(agent->n_actions == 3);
    
    // Test action selection
    size_t action = rl_agent_select_action(agent, 0);
    assert(action < 3);
    
    rl_agent_destroy(agent);
    qlearning_destroy(qmodel);
    
    printf("✓ RL agent wrapper test passed\n");
}

void test_rl_training_config(void) {
    printf("Testing RL training config...\n");
    
    RLTrainingConfig config = rl_default_training_config();
    
    assert(config.max_episodes == 1000);
    assert(config.max_steps_per_episode == 200);
    assert(config.eval_frequency == 100);
    assert(config.verbose == false);
    
    printf("✓ RL training config test passed\n");
}

void test_rl_td_error(void) {
    printf("Testing RL TD error computation...\n");
    
    // TD error = reward + gamma * next_value - current_value
    double td_error = rl_compute_td_error(1.0, 0.9, 2.0, 0.5);
    // Expected: 1.0 + 0.9 * 2.0 - 0.5 = 2.3
    assert(fabs(td_error - 2.3) < 1e-6);
    
    printf("✓ RL TD error test passed\n");
}

int main(void) {
    printf("Running RL Framework tests...\n\n");
    
    test_rl_environment();
    test_rl_trajectory();
    test_rl_metrics();
    test_rl_agent_wrapper();
    test_rl_training_config();
    test_rl_td_error();
    
    printf("\n✓ All RL Framework tests passed!\n");
    return 0;
}
