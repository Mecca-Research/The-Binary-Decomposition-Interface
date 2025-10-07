
/**
 * @file rl_framework.c
 * @brief Reinforcement Learning Framework Implementation
 */

#include "rl_framework.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

// Environment functions

RLEnvironment* rl_env_create(void* env_data, RLEnvironmentInterface interface) {
    if (!env_data) return nullptr;
    
    RLEnvironment* env = malloc(sizeof(RLEnvironment));
    if (!env) return nullptr;
    
    env->env_data = env_data;
    env->interface = interface;
    env->current_state = 0;
    env->episode_count = 0;
    env->total_steps = 0;
    
    return env;
}

void rl_env_destroy(RLEnvironment* env) {
    if (!env) return;
    free(env);
}

size_t rl_env_reset(RLEnvironment* env) {
    if (!env || !env->interface.reset) return 0;
    
    env->interface.reset(env->env_data);
    env->current_state = env->interface.get_state(env->env_data);
    env->episode_count++;
    
    return env->current_state;
}

size_t rl_env_step(RLEnvironment* env, size_t action, double* reward, bool* done) {
    if (!env || !env->interface.step) {
        if (reward) *reward = 0.0;
        if (done) *done = true;
        return 0;
    }
    
    size_t next_state = env->interface.step(env->env_data, action, reward, done);
    env->current_state = next_state;
    env->total_steps++;
    
    return next_state;
}

size_t rl_env_get_state(const RLEnvironment* env) {
    return env ? env->current_state : 0;
}

void rl_env_render(RLEnvironment* env) {
    if (env && env->interface.render) {
        env->interface.render(env->env_data);
    }
}

// Agent functions

RLAgent* rl_agent_create(void* agent_data, RLAgentInterface interface,
                         size_t n_states, size_t n_actions) {
    if (!agent_data) return nullptr;
    
    RLAgent* agent = malloc(sizeof(RLAgent));
    if (!agent) return nullptr;
    
    agent->agent_data = agent_data;
    agent->interface = interface;
    agent->n_states = n_states;
    agent->n_actions = n_actions;
    
    return agent;
}

void rl_agent_destroy(RLAgent* agent) {
    if (!agent) return;
    free(agent);
}

size_t rl_agent_select_action(RLAgent* agent, size_t state) {
    if (!agent || !agent->interface.select_action) return 0;
    return agent->interface.select_action(agent->agent_data, state);
}

void rl_agent_update(RLAgent* agent, size_t state, size_t action,
                     double reward, size_t next_state, bool done) {
    if (!agent || !agent->interface.update) return;
    agent->interface.update(agent->agent_data, state, action, reward, next_state, done);
}

double rl_agent_get_value(RLAgent* agent, size_t state) {
    if (!agent || !agent->interface.get_value) return 0.0;
    return agent->interface.get_value(agent->agent_data, state);
}

double rl_agent_get_q_value(RLAgent* agent, size_t state, size_t action) {
    if (!agent || !agent->interface.get_q_value) return 0.0;
    return agent->interface.get_q_value(agent->agent_data, state, action);
}

// Trajectory functions

RLTrajectory* rl_trajectory_create(size_t initial_capacity) {
    if (initial_capacity == 0) initial_capacity = 100;
    
    RLTrajectory* trajectory = malloc(sizeof(RLTrajectory));
    if (!trajectory) return nullptr;
    
    trajectory->transitions = malloc(initial_capacity * sizeof(RLTransition));
    if (!trajectory->transitions) {
        free(trajectory);
        return nullptr;
    }
    
    trajectory->length = 0;
    trajectory->capacity = initial_capacity;
    trajectory->total_reward = 0.0;
    trajectory->complete = false;
    
    return trajectory;
}

void rl_trajectory_destroy(RLTrajectory* trajectory) {
    if (!trajectory) return;
    free(trajectory->transitions);
    free(trajectory);
}

void rl_trajectory_add(RLTrajectory* trajectory, const RLTransition* transition) {
    if (!trajectory || !transition) return;
    
    // Resize if needed
    if (trajectory->length >= trajectory->capacity) {
        size_t new_capacity = trajectory->capacity * 2;
        RLTransition* new_transitions = realloc(trajectory->transitions,
                                               new_capacity * sizeof(RLTransition));
        if (!new_transitions) return;
        
        trajectory->transitions = new_transitions;
        trajectory->capacity = new_capacity;
    }
    
    trajectory->transitions[trajectory->length++] = *transition;
    trajectory->total_reward += transition->reward;
    
    if (transition->done) {
        trajectory->complete = true;
    }
}

void rl_trajectory_reset(RLTrajectory* trajectory) {
    if (!trajectory) return;
    
    trajectory->length = 0;
    trajectory->total_reward = 0.0;
    trajectory->complete = false;
}

double rl_trajectory_compute_return(const RLTrajectory* trajectory, double gamma) {
    if (!trajectory || trajectory->length == 0) return 0.0;
    
    double ret = 0.0;
    double discount = 1.0;
    
    for (size_t i = 0; i < trajectory->length; i++) {
        ret += discount * trajectory->transitions[i].reward;
        discount *= gamma;
    }
    
    return ret;
}

double rl_trajectory_compute_discounted_return(const RLTrajectory* trajectory,
                                               double gamma, size_t start_idx) {
    if (!trajectory || start_idx >= trajectory->length) return 0.0;
    
    double ret = 0.0;
    double discount = 1.0;
    
    for (size_t i = start_idx; i < trajectory->length; i++) {
        ret += discount * trajectory->transitions[i].reward;
        discount *= gamma;
    }
    
    return ret;
}

// Metrics functions

RLMetrics* rl_metrics_create(size_t capacity) {
    if (capacity == 0) capacity = 1000;
    
    RLMetrics* metrics = malloc(sizeof(RLMetrics));
    if (!metrics) return nullptr;
    
    metrics->episode_rewards = malloc(capacity * sizeof(double));
    metrics->episode_lengths = malloc(capacity * sizeof(size_t));
    metrics->episode_values = malloc(capacity * sizeof(double));
    
    if (!metrics->episode_rewards || !metrics->episode_lengths || !metrics->episode_values) {
        free(metrics->episode_rewards);
        free(metrics->episode_lengths);
        free(metrics->episode_values);
        free(metrics);
        return nullptr;
    }
    
    metrics->num_episodes = 0;
    metrics->capacity = capacity;
    metrics->best_reward = -INFINITY;
    metrics->worst_reward = INFINITY;
    metrics->average_reward = 0.0;
    metrics->reward_std = 0.0;
    
    return metrics;
}

void rl_metrics_destroy(RLMetrics* metrics) {
    if (!metrics) return;
    
    free(metrics->episode_rewards);
    free(metrics->episode_lengths);
    free(metrics->episode_values);
    free(metrics);
}

void rl_metrics_add_episode(RLMetrics* metrics, double reward, size_t length, double value) {
    if (!metrics) return;
    
    if (metrics->num_episodes >= metrics->capacity) {
        // Resize arrays - use temporary variables to ensure all succeed before updating
        size_t new_capacity = metrics->capacity * 2;
        
        double* new_rewards = realloc(metrics->episode_rewards, new_capacity * sizeof(double));
        size_t* new_lengths = realloc(metrics->episode_lengths, new_capacity * sizeof(size_t));
        double* new_values = realloc(metrics->episode_values, new_capacity * sizeof(double));
        
        // Check if ALL reallocs succeeded
        if (!new_rewards || !new_lengths || !new_values) {
            // If any failed, we need to handle cleanup carefully
            // Note: realloc frees the original pointer only on success
            // If realloc fails, the original pointer is still valid
            // However, if one succeeded and another failed, we have a problem
            
            // Free any successful allocations that are different from originals
            if (new_rewards && new_rewards != metrics->episode_rewards) {
                free(new_rewards);
            }
            if (new_lengths && new_lengths != metrics->episode_lengths) {
                free(new_lengths);
            }
            if (new_values && new_values != metrics->episode_values) {
                free(new_values);
            }
            
            // Cannot resize, skip this episode recording
            return;
        }
        
        // All reallocs succeeded, safe to update pointers
        metrics->episode_rewards = new_rewards;
        metrics->episode_lengths = new_lengths;
        metrics->episode_values = new_values;
        metrics->capacity = new_capacity;
    }
    
    metrics->episode_rewards[metrics->num_episodes] = reward;
    metrics->episode_lengths[metrics->num_episodes] = length;
    metrics->episode_values[metrics->num_episodes] = value;
    metrics->num_episodes++;
    
    // Update best/worst
    if (reward > metrics->best_reward) metrics->best_reward = reward;
    if (reward < metrics->worst_reward) metrics->worst_reward = reward;
}

void rl_metrics_compute_statistics(RLMetrics* metrics) {
    if (!metrics || metrics->num_episodes == 0) return;
    
    // Compute average
    double sum = 0.0;
    for (size_t i = 0; i < metrics->num_episodes; i++) {
        sum += metrics->episode_rewards[i];
    }
    metrics->average_reward = sum / metrics->num_episodes;
    
    // Compute standard deviation
    double variance = 0.0;
    for (size_t i = 0; i < metrics->num_episodes; i++) {
        double diff = metrics->episode_rewards[i] - metrics->average_reward;
        variance += diff * diff;
    }
    metrics->reward_std = sqrt(variance / metrics->num_episodes);
}

void rl_metrics_print(const RLMetrics* metrics) {
    if (!metrics) return;
    
    printf("RL Training Metrics:\n");
    printf("  Episodes: %zu\n", metrics->num_episodes);
    printf("  Average Reward: %.2f ± %.2f\n", metrics->average_reward, metrics->reward_std);
    printf("  Best Reward: %.2f\n", metrics->best_reward);
    printf("  Worst Reward: %.2f\n", metrics->worst_reward);
    
    if (metrics->num_episodes > 0) {
        size_t avg_length = 0;
        for (size_t i = 0; i < metrics->num_episodes; i++) {
            avg_length += metrics->episode_lengths[i];
        }
        avg_length /= metrics->num_episodes;
        printf("  Average Episode Length: %zu\n", avg_length);
    }
}

bool rl_metrics_check_convergence(const RLMetrics* metrics,
                                  double threshold, size_t window) {
    if (!metrics || metrics->num_episodes < window) return false;
    
    // Check if reward variance in last 'window' episodes is below threshold
    size_t start = metrics->num_episodes - window;
    
    double sum = 0.0;
    for (size_t i = start; i < metrics->num_episodes; i++) {
        sum += metrics->episode_rewards[i];
    }
    double mean = sum / window;
    
    double variance = 0.0;
    for (size_t i = start; i < metrics->num_episodes; i++) {
        double diff = metrics->episode_rewards[i] - mean;
        variance += diff * diff;
    }
    variance /= window;
    
    return variance < threshold;
}

// Training functions

bool rl_train_episode(RLAgent* agent, RLEnvironment* env,
                     RLTrajectory* trajectory, size_t max_steps) {
    if (!agent || !env) return false;
    
    if (trajectory) {
        rl_trajectory_reset(trajectory);
    }
    
    size_t state = rl_env_reset(env);
    size_t steps = 0;
    bool done = false;
    
    while (!done && steps < max_steps) {
        // Select action
        size_t action = rl_agent_select_action(agent, state);
        
        // Take step
        double reward;
        size_t next_state = rl_env_step(env, action, &reward, &done);
        
        // Update agent
        rl_agent_update(agent, state, action, reward, next_state, done);
        
        // Store transition
        if (trajectory) {
            RLTransition transition = {
                .state = state,
                .action = action,
                .reward = reward,
                .next_state = next_state,
                .done = done
            };
            rl_trajectory_add(trajectory, &transition);
        }
        
        state = next_state;
        steps++;
    }
    
    return true;
}

double rl_evaluate_agent(RLAgent* agent, RLEnvironment* env,
                        size_t n_episodes, size_t max_steps) {
    if (!agent || !env || n_episodes == 0) return 0.0;
    
    double total_reward = 0.0;
    
    for (size_t episode = 0; episode < n_episodes; episode++) {
        size_t state = rl_env_reset(env);
        double episode_reward = 0.0;
        bool done = false;
        size_t steps = 0;
        
        while (!done && steps < max_steps) {
            size_t action = rl_agent_select_action(agent, state);
            double reward;
            size_t next_state = rl_env_step(env, action, &reward, &done);
            
            episode_reward += reward;
            state = next_state;
            steps++;
        }
        
        total_reward += episode_reward;
    }
    
    return total_reward / n_episodes;
}

bool rl_train(RLAgent* agent, RLEnvironment* env, RLTrainingConfig config,
             RLMetrics** out_metrics) {
    if (!agent || !env) return false;
    
    RLMetrics* metrics = rl_metrics_create(config.max_episodes);
    if (!metrics) return false;
    
    RLTrajectory* trajectory = rl_trajectory_create(config.max_steps_per_episode);
    if (!trajectory) {
        rl_metrics_destroy(metrics);
        return false;
    }
    
    for (size_t episode = 0; episode < config.max_episodes; episode++) {
        // Train one episode
        rl_train_episode(agent, env, trajectory, config.max_steps_per_episode);
        
        // Record metrics
        double episode_reward = trajectory->total_reward;
        size_t episode_length = trajectory->length;
        double episode_value = rl_agent_get_value(agent, 0); // Initial state value
        
        rl_metrics_add_episode(metrics, episode_reward, episode_length, episode_value);
        
        // Verbose output
        if (config.verbose && (episode + 1) % 100 == 0) {
            rl_metrics_compute_statistics(metrics);
            printf("Episode %zu/%zu - Avg Reward: %.2f\n",
                   episode + 1, config.max_episodes, metrics->average_reward);
        }
        
        // Evaluation
        if (config.eval_frequency > 0 && (episode + 1) % config.eval_frequency == 0) {
            double eval_reward = rl_evaluate_agent(agent, env, config.eval_episodes,
                                                   config.max_steps_per_episode);
            if (config.verbose) {
                printf("  Evaluation Reward: %.2f\n", eval_reward);
            }
        }
        
        // Check convergence
        if (config.convergence_window > 0 && episode >= config.convergence_window) {
            if (rl_metrics_check_convergence(metrics, config.convergence_threshold,
                                            config.convergence_window)) {
                if (config.verbose) {
                    printf("Converged at episode %zu\n", episode + 1);
                }
                break;
            }
        }
    }
    
    rl_trajectory_destroy(trajectory);
    rl_metrics_compute_statistics(metrics);
    
    if (out_metrics) {
        *out_metrics = metrics;
    } else {
        rl_metrics_destroy(metrics);
    }
    
    return true;
}

// Utility functions

double rl_compute_td_error(double reward, double gamma,
                          double next_value, double current_value) {
    return reward + gamma * next_value - current_value;
}

double rl_compute_gae(const RLTrajectory* trajectory, double gamma, double lambda) {
    if (!trajectory || trajectory->length == 0) return 0.0;
    
    // Generalized Advantage Estimation (simplified)
    double gae = 0.0;
    double discount = 1.0;
    
    for (size_t i = 0; i < trajectory->length; i++) {
        gae += discount * trajectory->transitions[i].reward;
        discount *= gamma * lambda;
    }
    
    return gae;
}

// Default configuration

RLTrainingConfig rl_default_training_config(void) {
    return (RLTrainingConfig){
        .max_episodes = 1000,
        .max_steps_per_episode = 200,
        .eval_frequency = 100,
        .eval_episodes = 10,
        .convergence_threshold = 0.01,
        .convergence_window = 100,
        .verbose = false,
        .render = false
    };
}
