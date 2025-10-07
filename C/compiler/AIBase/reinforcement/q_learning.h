
/**
 * @file q_learning.h
 * @brief Q-Learning Reinforcement Learning Algorithm
 * @details Temporal Difference learning for discrete state-action spaces
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides Q-learning as a first-class compiler feature.
 * 
 * @author BDI Kernel Team
 * @date 2025
 */
#ifndef BDI_Q_LEARNING_H
#define BDI_Q_LEARNING_H

#include "../../../c23_compat.h"
#include <stddef.h>
#include <stdbool.h>

// Q-table entry for state-action pairs
typedef struct {
    size_t state;
    size_t action;
    double q_value;
} QTableEntry;

// Q-learning model structure
typedef struct {
    // Q-table storage (hash map implementation)
    QTableEntry* q_table;
    size_t q_table_size;
    size_t q_table_capacity;
    
    // State and action space dimensions
    size_t n_states;
    size_t n_actions;
    
    // Learning parameters
    double alpha;           // Learning rate (0 < alpha <= 1)
    double gamma;           // Discount factor (0 <= gamma <= 1)
    double epsilon;         // Exploration rate (0 <= epsilon <= 1)
    double epsilon_decay;   // Epsilon decay rate
    double epsilon_min;     // Minimum epsilon value
    
    // Training statistics
    size_t total_episodes;
    size_t total_steps;
    double* episode_rewards;
    size_t episode_rewards_capacity;
    
    bool fitted;
} QLearningModel;

// Training configuration
typedef struct {
    double alpha;
    double gamma;
    double epsilon;
    double epsilon_decay;
    double epsilon_min;
    size_t max_episodes;
    size_t max_steps_per_episode;
    bool verbose;
} QLearningConfig;

// Experience tuple for training
typedef struct {
    size_t state;
    size_t action;
    double reward;
    size_t next_state;
    bool done;
} Experience;

// Model lifecycle
QLearningModel* qlearning_create(size_t n_states, size_t n_actions, QLearningConfig config);
void qlearning_destroy(QLearningModel* model);
QLearningConfig qlearning_default_config(void);

// Q-table operations
double qlearning_get_q_value(const QLearningModel* model, size_t state, size_t action);
void qlearning_set_q_value(QLearningModel* model, size_t state, size_t action, double q_value);
double qlearning_get_max_q_value(const QLearningModel* model, size_t state);
size_t qlearning_get_best_action(const QLearningModel* model, size_t state);

// Policy
size_t qlearning_select_action(QLearningModel* model, size_t state);
size_t qlearning_select_greedy_action(const QLearningModel* model, size_t state);
size_t qlearning_select_random_action(const QLearningModel* model);

// Learning
void qlearning_update(QLearningModel* model, const Experience* exp);
void qlearning_update_epsilon(QLearningModel* model);

// Training
bool qlearning_train_episode(QLearningModel* model, 
                             void* env,
                             size_t (*get_state)(void*),
                             size_t (*step)(void*, size_t, double*, bool*),
                             void (*reset)(void*));

// Evaluation
double qlearning_evaluate(const QLearningModel* model,
                         void* env,
                         size_t (*get_state)(void*),
                         size_t (*step)(void*, size_t, double*, bool*),
                         void (*reset)(void*),
                         size_t n_episodes);

// Statistics
double qlearning_get_average_reward(const QLearningModel* model, size_t last_n_episodes);
void qlearning_print_statistics(const QLearningModel* model);

// Serialization helpers
size_t qlearning_get_q_table_size(const QLearningModel* model);
void qlearning_export_q_table(const QLearningModel* model, QTableEntry* buffer, size_t buffer_size);

// Compile-time invariants
static_assert(sizeof(double) == 8, "Q-learning requires 64-bit doubles");
static_assert(sizeof(size_t) >= 4, "Q-learning requires at least 32-bit size_t");

#endif // BDI_Q_LEARNING_H
