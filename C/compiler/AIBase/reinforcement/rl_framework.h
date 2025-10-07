
/**
 * @file rl_framework.h
 * @brief Reinforcement Learning Framework
 * @details Reusable infrastructure for RL algorithms
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides a generic RL framework for various algorithms.
 * 
 * @author BDI Kernel Team
 * @date 2025
 */
#ifndef BDI_RL_FRAMEWORK_H
#define BDI_RL_FRAMEWORK_H

#include "../../../c23_compat.h"
#include <stddef.h>
#include <stdbool.h>

// Forward declarations
typedef struct RLEnvironment RLEnvironment;
typedef struct RLAgent RLAgent;
typedef struct RLTrajectory RLTrajectory;

// Environment interface
typedef struct {
    // Get current state
    size_t (*get_state)(void* env_data);
    
    // Take action, return next state, reward, and done flag
    size_t (*step)(void* env_data, size_t action, double* reward, bool* done);
    
    // Reset environment to initial state
    void (*reset)(void* env_data);
    
    // Get state space size
    size_t (*get_state_space_size)(void* env_data);
    
    // Get action space size
    size_t (*get_action_space_size)(void* env_data);
    
    // Optional: render environment
    void (*render)(void* env_data);
} RLEnvironmentInterface;

// Environment wrapper
struct RLEnvironment {
    void* env_data;
    RLEnvironmentInterface interface;
    size_t current_state;
    size_t episode_count;
    size_t total_steps;
};

// Agent interface
typedef struct {
    // Select action given state
    size_t (*select_action)(void* agent_data, size_t state);
    
    // Update agent with experience
    void (*update)(void* agent_data, size_t state, size_t action, 
                   double reward, size_t next_state, bool done);
    
    // Get value estimate for state
    double (*get_value)(void* agent_data, size_t state);
    
    // Get Q-value for state-action pair
    double (*get_q_value)(void* agent_data, size_t state, size_t action);
} RLAgentInterface;

// Agent wrapper
struct RLAgent {
    void* agent_data;
    RLAgentInterface interface;
    size_t n_states;
    size_t n_actions;
};

// Transition (single step)
typedef struct {
    size_t state;
    size_t action;
    double reward;
    size_t next_state;
    bool done;
} RLTransition;

// Trajectory (episode)
struct RLTrajectory {
    RLTransition* transitions;
    size_t length;
    size_t capacity;
    double total_reward;
    bool complete;
};

// Training metrics
typedef struct {
    double* episode_rewards;
    size_t* episode_lengths;
    double* episode_values;
    size_t num_episodes;
    size_t capacity;
    
    double best_reward;
    double worst_reward;
    double average_reward;
    double reward_std;
} RLMetrics;

// Training configuration
typedef struct {
    size_t max_episodes;
    size_t max_steps_per_episode;
    size_t eval_frequency;
    size_t eval_episodes;
    double convergence_threshold;
    size_t convergence_window;
    bool verbose;
    bool render;
} RLTrainingConfig;

// Environment functions
RLEnvironment* rl_env_create(void* env_data, RLEnvironmentInterface interface);
void rl_env_destroy(RLEnvironment* env);
size_t rl_env_reset(RLEnvironment* env);
size_t rl_env_step(RLEnvironment* env, size_t action, double* reward, bool* done);
size_t rl_env_get_state(const RLEnvironment* env);
void rl_env_render(RLEnvironment* env);

// Agent functions
RLAgent* rl_agent_create(void* agent_data, RLAgentInterface interface,
                         size_t n_states, size_t n_actions);
void rl_agent_destroy(RLAgent* agent);
size_t rl_agent_select_action(RLAgent* agent, size_t state);
void rl_agent_update(RLAgent* agent, size_t state, size_t action,
                     double reward, size_t next_state, bool done);
double rl_agent_get_value(RLAgent* agent, size_t state);
double rl_agent_get_q_value(RLAgent* agent, size_t state, size_t action);

// Trajectory functions
RLTrajectory* rl_trajectory_create(size_t initial_capacity);
void rl_trajectory_destroy(RLTrajectory* trajectory);
void rl_trajectory_add(RLTrajectory* trajectory, const RLTransition* transition);
void rl_trajectory_reset(RLTrajectory* trajectory);
double rl_trajectory_compute_return(const RLTrajectory* trajectory, double gamma);
double rl_trajectory_compute_discounted_return(const RLTrajectory* trajectory, 
                                               double gamma, size_t start_idx);

// Metrics functions
RLMetrics* rl_metrics_create(size_t capacity);
void rl_metrics_destroy(RLMetrics* metrics);
void rl_metrics_add_episode(RLMetrics* metrics, double reward, size_t length, double value);
void rl_metrics_compute_statistics(RLMetrics* metrics);
void rl_metrics_print(const RLMetrics* metrics);
bool rl_metrics_check_convergence(const RLMetrics* metrics, 
                                  double threshold, size_t window);

// Training functions
bool rl_train_episode(RLAgent* agent, RLEnvironment* env, 
                     RLTrajectory* trajectory, size_t max_steps);
double rl_evaluate_agent(RLAgent* agent, RLEnvironment* env,
                        size_t n_episodes, size_t max_steps);
bool rl_train(RLAgent* agent, RLEnvironment* env, RLTrainingConfig config,
             RLMetrics** out_metrics);

// Utility functions
double rl_compute_td_error(double reward, double gamma, 
                          double next_value, double current_value);
double rl_compute_gae(const RLTrajectory* trajectory, double gamma, double lambda);

// Default configurations
RLTrainingConfig rl_default_training_config(void);

// Compile-time invariants
static_assert(sizeof(double) == 8, "RL framework requires 64-bit doubles");
static_assert(sizeof(size_t) >= 4, "RL framework requires at least 32-bit size_t");

#endif // BDI_RL_FRAMEWORK_H
