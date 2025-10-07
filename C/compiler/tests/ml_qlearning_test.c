
/**
 * @file ml_qlearning_test.c
 * @brief Q-Learning Algorithm Tests
 */

#include "../AIBase/reinforcement/q_learning.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

// Simple grid world environment for testing
typedef struct {
    size_t grid_size;
    size_t agent_pos;
    size_t goal_pos;
    size_t n_states;
    size_t n_actions;
} GridWorld;

static GridWorld* gridworld_create(size_t grid_size) {
    GridWorld* env = malloc(sizeof(GridWorld));
    env->grid_size = grid_size;
    env->n_states = grid_size * grid_size;
    env->n_actions = 4; // up, down, left, right
    env->agent_pos = 0;
    env->goal_pos = env->n_states - 1;
    return env;
}

static void gridworld_reset(void* env_data) {
    GridWorld* env = (GridWorld*)env_data;
    env->agent_pos = 0;
}

static size_t gridworld_get_state(void* env_data) {
    GridWorld* env = (GridWorld*)env_data;
    return env->agent_pos;
}

static size_t gridworld_step(void* env_data, size_t action, double* reward, bool* done) {
    GridWorld* env = (GridWorld*)env_data;
    
    size_t row = env->agent_pos / env->grid_size;
    size_t col = env->agent_pos % env->grid_size;
    
    // Move based on action (0=up, 1=down, 2=left, 3=right)
    switch (action) {
        case 0: if (row > 0) row--; break;
        case 1: if (row < env->grid_size - 1) row++; break;
        case 2: if (col > 0) col--; break;
        case 3: if (col < env->grid_size - 1) col++; break;
    }
    
    env->agent_pos = row * env->grid_size + col;
    
    // Reward and done
    if (env->agent_pos == env->goal_pos) {
        *reward = 1.0;
        *done = true;
    } else {
        *reward = -0.01; // Small penalty for each step
        *done = false;
    }
    
    return env->agent_pos;
}

void test_qlearning_create_destroy(void) {
    printf("Testing Q-learning create/destroy...\n");
    
    QLearningConfig config = qlearning_default_config();
    QLearningModel* model = qlearning_create(10, 4, config);
    
    assert(model != nullptr);
    assert(model->n_states == 10);
    assert(model->n_actions == 4);
    assert(model->alpha == 0.1);
    assert(model->gamma == 0.99);
    
    qlearning_destroy(model);
    
    printf("✓ Q-learning create/destroy test passed\n");
}

void test_qlearning_q_table(void) {
    printf("Testing Q-learning Q-table operations...\n");
    
    QLearningConfig config = qlearning_default_config();
    QLearningModel* model = qlearning_create(5, 3, config);
    
    // Set and get Q-values
    qlearning_set_q_value(model, 0, 0, 1.5);
    qlearning_set_q_value(model, 0, 1, 2.0);
    qlearning_set_q_value(model, 0, 2, 0.5);
    
    assert(fabs(qlearning_get_q_value(model, 0, 0) - 1.5) < 1e-6);
    assert(fabs(qlearning_get_q_value(model, 0, 1) - 2.0) < 1e-6);
    assert(fabs(qlearning_get_q_value(model, 0, 2) - 0.5) < 1e-6);
    
    // Test max Q-value
    double max_q = qlearning_get_max_q_value(model, 0);
    assert(fabs(max_q - 2.0) < 1e-6);
    
    // Test best action
    size_t best_action = qlearning_get_best_action(model, 0);
    assert(best_action == 1);
    
    qlearning_destroy(model);
    
    printf("✓ Q-learning Q-table test passed\n");
}

void test_qlearning_update(void) {
    printf("Testing Q-learning update rule...\n");
    
    QLearningConfig config = qlearning_default_config();
    config.alpha = 0.5;
    config.gamma = 0.9;
    
    QLearningModel* model = qlearning_create(3, 2, config);
    
    // Initial Q-value
    qlearning_set_q_value(model, 0, 0, 0.0);
    qlearning_set_q_value(model, 1, 0, 1.0);
    qlearning_set_q_value(model, 1, 1, 2.0);
    
    // Update: Q(0,0) with reward=1.0, next_state=1
    Experience exp = {
        .state = 0,
        .action = 0,
        .reward = 1.0,
        .next_state = 1,
        .done = false
    };
    
    qlearning_update(model, &exp);
    
    // Expected: Q(0,0) = 0.0 + 0.5 * (1.0 + 0.9 * 2.0 - 0.0) = 1.4
    double updated_q = qlearning_get_q_value(model, 0, 0);
    assert(fabs(updated_q - 1.4) < 1e-6);
    
    qlearning_destroy(model);
    
    printf("✓ Q-learning update test passed\n");
}

void test_qlearning_gridworld(void) {
    printf("Testing Q-learning on grid world...\n");
    
    GridWorld* env = gridworld_create(3);
    
    QLearningConfig config = qlearning_default_config();
    config.alpha = 0.1;
    config.gamma = 0.99;
    config.epsilon = 1.0;
    config.epsilon_decay = 0.99;
    config.max_episodes = 100;
    
    QLearningModel* model = qlearning_create(env->n_states, env->n_actions, config);
    
    // Train for a few episodes
    for (size_t episode = 0; episode < 50; episode++) {
        qlearning_train_episode(model, env,
                               gridworld_get_state,
                               gridworld_step,
                               gridworld_reset);
    }
    
    assert(model->fitted);
    assert(model->total_episodes == 50);
    
    // Check that Q-values have been updated
    size_t non_zero_count = 0;
    for (size_t s = 0; s < env->n_states; s++) {
        for (size_t a = 0; a < env->n_actions; a++) {
            if (fabs(qlearning_get_q_value(model, s, a)) > 1e-6) {
                non_zero_count++;
            }
        }
    }
    
    assert(non_zero_count > 0);
    
    qlearning_destroy(model);
    free(env);
    
    printf("✓ Q-learning grid world test passed\n");
}

void test_qlearning_epsilon_decay(void) {
    printf("Testing Q-learning epsilon decay...\n");
    
    QLearningConfig config = qlearning_default_config();
    config.epsilon = 1.0;
    config.epsilon_decay = 0.9;
    config.epsilon_min = 0.1;
    
    QLearningModel* model = qlearning_create(5, 3, config);
    
    assert(fabs(model->epsilon - 1.0) < 1e-6);
    
    qlearning_update_epsilon(model);
    assert(fabs(model->epsilon - 0.9) < 1e-6);
    
    qlearning_update_epsilon(model);
    assert(fabs(model->epsilon - 0.81) < 1e-6);
    
    // Decay many times
    for (int i = 0; i < 100; i++) {
        qlearning_update_epsilon(model);
    }
    
    // Should not go below epsilon_min
    assert(model->epsilon >= config.epsilon_min);
    
    qlearning_destroy(model);
    
    printf("✓ Q-learning epsilon decay test passed\n");
}

int main(void) {
    printf("Running Q-Learning tests...\n\n");
    
    test_qlearning_create_destroy();
    test_qlearning_q_table();
    test_qlearning_update();
    test_qlearning_gridworld();
    test_qlearning_epsilon_decay();
    
    printf("\n✓ All Q-Learning tests passed!\n");
    return 0;
}
