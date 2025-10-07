
/**
 * @file q_learning.c
 * @brief Q-Learning Reinforcement Learning Algorithm Implementation
 */

#include "q_learning.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

// Hash function for state-action pairs
static size_t hash_state_action(size_t state, size_t action, size_t capacity) {
    // Simple hash combining state and action
    size_t hash = state * 31 + action;
    return hash % capacity;
}

// Find Q-table entry
static QTableEntry* find_q_entry(const QLearningModel* model, size_t state, size_t action) {
    if (!model || !model->q_table) return nullptr;
    
    size_t hash = hash_state_action(state, action, model->q_table_capacity);
    size_t probe = 0;
    
    // Linear probing
    while (probe < model->q_table_capacity) {
        size_t idx = (hash + probe) % model->q_table_capacity;
        QTableEntry* entry = &model->q_table[idx];
        
        if (entry->q_value == 0.0 && entry->state == 0 && entry->action == 0) {
            // Empty slot
            return nullptr;
        }
        
        if (entry->state == state && entry->action == action) {
            return entry;
        }
        
        probe++;
    }
    
    return nullptr;
}

// Insert or update Q-table entry
static bool insert_q_entry(QLearningModel* model, size_t state, size_t action, double q_value) {
    if (!model || !model->q_table) return false;
    
    // Check if we need to resize
    if (model->q_table_size >= model->q_table_capacity * 0.7) {
        // Resize Q-table
        size_t new_capacity = model->q_table_capacity * 2;
        QTableEntry* new_table = calloc(new_capacity, sizeof(QTableEntry));
        if (!new_table) return false;
        
        // Rehash existing entries
        for (size_t i = 0; i < model->q_table_capacity; i++) {
            QTableEntry* old_entry = &model->q_table[i];
            if (old_entry->q_value != 0.0 || old_entry->state != 0 || old_entry->action != 0) {
                size_t hash = hash_state_action(old_entry->state, old_entry->action, new_capacity);
                size_t probe = 0;
                
                while (probe < new_capacity) {
                    size_t idx = (hash + probe) % new_capacity;
                    if (new_table[idx].q_value == 0.0 && new_table[idx].state == 0 && new_table[idx].action == 0) {
                        new_table[idx] = *old_entry;
                        break;
                    }
                    probe++;
                }
            }
        }
        
        free(model->q_table);
        model->q_table = new_table;
        model->q_table_capacity = new_capacity;
    }
    
    size_t hash = hash_state_action(state, action, model->q_table_capacity);
    size_t probe = 0;
    
    while (probe < model->q_table_capacity) {
        size_t idx = (hash + probe) % model->q_table_capacity;
        QTableEntry* entry = &model->q_table[idx];
        
        if (entry->q_value == 0.0 && entry->state == 0 && entry->action == 0) {
            // Empty slot - insert new entry
            entry->state = state;
            entry->action = action;
            entry->q_value = q_value;
            model->q_table_size++;
            return true;
        }
        
        if (entry->state == state && entry->action == action) {
            // Update existing entry
            entry->q_value = q_value;
            return true;
        }
        
        probe++;
    }
    
    return false;
}

// Default configuration
QLearningConfig qlearning_default_config(void) {
    return (QLearningConfig){
        .alpha = 0.1,
        .gamma = 0.99,
        .epsilon = 1.0,
        .epsilon_decay = 0.995,
        .epsilon_min = 0.01,
        .max_episodes = 1000,
        .max_steps_per_episode = 200,
        .verbose = false
    };
}

// Create Q-learning model
QLearningModel* qlearning_create(size_t n_states, size_t n_actions, QLearningConfig config) {
    if (n_states == 0 || n_actions == 0) return nullptr;
    if (config.alpha <= 0.0 || config.alpha > 1.0) return nullptr;
    if (config.gamma < 0.0 || config.gamma > 1.0) return nullptr;
    if (config.epsilon < 0.0 || config.epsilon > 1.0) return nullptr;
    
    QLearningModel* model = malloc(sizeof(QLearningModel));
    if (!model) return nullptr;
    
    // Initialize Q-table with reasonable initial capacity
    model->q_table_capacity = (n_states * n_actions) / 4 + 128;
    model->q_table = calloc(model->q_table_capacity, sizeof(QTableEntry));
    if (!model->q_table) {
        free(model);
        return nullptr;
    }
    
    model->q_table_size = 0;
    model->n_states = n_states;
    model->n_actions = n_actions;
    model->alpha = config.alpha;
    model->gamma = config.gamma;
    model->epsilon = config.epsilon;
    model->epsilon_decay = config.epsilon_decay;
    model->epsilon_min = config.epsilon_min;
    model->total_episodes = 0;
    model->total_steps = 0;
    model->fitted = false;
    
    // Initialize episode rewards tracking
    model->episode_rewards_capacity = config.max_episodes;
    model->episode_rewards = calloc(model->episode_rewards_capacity, sizeof(double));
    if (!model->episode_rewards) {
        free(model->q_table);
        free(model);
        return nullptr;
    }
    
    return model;
}

// Destroy Q-learning model
void qlearning_destroy(QLearningModel* model) {
    if (!model) return;
    
    free(model->q_table);
    free(model->episode_rewards);
    free(model);
}

// Get Q-value for state-action pair
double qlearning_get_q_value(const QLearningModel* model, size_t state, size_t action) {
    if (!model || state >= model->n_states || action >= model->n_actions) {
        return 0.0;
    }
    
    QTableEntry* entry = find_q_entry(model, state, action);
    return entry ? entry->q_value : 0.0;
}

// Set Q-value for state-action pair
void qlearning_set_q_value(QLearningModel* model, size_t state, size_t action, double q_value) {
    if (!model || state >= model->n_states || action >= model->n_actions) {
        return;
    }
    
    insert_q_entry(model, state, action, q_value);
}

// Get maximum Q-value for a state
double qlearning_get_max_q_value(const QLearningModel* model, size_t state) {
    if (!model || state >= model->n_states) return 0.0;
    
    double max_q = -INFINITY;
    bool found = false;
    
    for (size_t action = 0; action < model->n_actions; action++) {
        double q = qlearning_get_q_value(model, state, action);
        if (!found || q > max_q) {
            max_q = q;
            found = true;
        }
    }
    
    return found ? max_q : 0.0;
}

// Get best action for a state (greedy)
size_t qlearning_get_best_action(const QLearningModel* model, size_t state) {
    if (!model || state >= model->n_states) return 0;
    
    size_t best_action = 0;
    double max_q = -INFINITY;
    
    for (size_t action = 0; action < model->n_actions; action++) {
        double q = qlearning_get_q_value(model, state, action);
        if (q > max_q) {
            max_q = q;
            best_action = action;
        }
    }
    
    return best_action;
}

// Select random action
size_t qlearning_select_random_action(const QLearningModel* model) {
    if (!model || model->n_actions == 0) return 0;
    return rand() % model->n_actions;
}

// Select action using epsilon-greedy policy
size_t qlearning_select_action(QLearningModel* model, size_t state) {
    if (!model || state >= model->n_states) return 0;
    
    // Epsilon-greedy: explore with probability epsilon
    double random_val = (double)rand() / RAND_MAX;
    
    if (random_val < model->epsilon) {
        // Explore: random action
        return qlearning_select_random_action(model);
    } else {
        // Exploit: best action
        return qlearning_get_best_action(model, state);
    }
}

// Select greedy action (no exploration)
size_t qlearning_select_greedy_action(const QLearningModel* model, size_t state) {
    return qlearning_get_best_action(model, state);
}

// Update Q-value using TD learning rule
void qlearning_update(QLearningModel* model, const Experience* exp) {
    if (!model || !exp) return;
    if (exp->state >= model->n_states || exp->action >= model->n_actions) return;
    if (exp->next_state >= model->n_states) return;
    
    // Q(s,a) ← Q(s,a) + α[r + γ max Q(s',a') - Q(s,a)]
    double current_q = qlearning_get_q_value(model, exp->state, exp->action);
    double max_next_q = exp->done ? 0.0 : qlearning_get_max_q_value(model, exp->next_state);
    
    double td_target = exp->reward + model->gamma * max_next_q;
    double td_error = td_target - current_q;
    double new_q = current_q + model->alpha * td_error;
    
    qlearning_set_q_value(model, exp->state, exp->action, new_q);
}

// Update epsilon (decay)
void qlearning_update_epsilon(QLearningModel* model) {
    if (!model) return;
    
    model->epsilon *= model->epsilon_decay;
    if (model->epsilon < model->epsilon_min) {
        model->epsilon = model->epsilon_min;
    }
}

// Train one episode
bool qlearning_train_episode(QLearningModel* model, 
                             void* env,
                             size_t (*get_state)(void*),
                             size_t (*step)(void*, size_t, double*, bool*),
                             void (*reset)(void*)) {
    if (!model || !env || !get_state || !step || !reset) return false;
    
    reset(env);
    size_t state = get_state(env);
    
    double episode_reward = 0.0;
    size_t steps = 0;
    bool done = false;
    
    while (!done && steps < 1000) {
        // Select action
        size_t action = qlearning_select_action(model, state);
        
        // Take step in environment
        double reward;
        size_t next_state = step(env, action, &reward, &done);
        
        // Create experience
        Experience exp = {
            .state = state,
            .action = action,
            .reward = reward,
            .next_state = next_state,
            .done = done
        };
        
        // Update Q-value
        qlearning_update(model, &exp);
        
        episode_reward += reward;
        state = next_state;
        steps++;
    }
    
    // Store episode reward
    if (model->total_episodes < model->episode_rewards_capacity) {
        model->episode_rewards[model->total_episodes] = episode_reward;
    }
    
    model->total_episodes++;
    model->total_steps += steps;
    model->fitted = true;
    
    // Decay epsilon
    qlearning_update_epsilon(model);
    
    return true;
}

// Evaluate model
double qlearning_evaluate(const QLearningModel* model,
                         void* env,
                         size_t (*get_state)(void*),
                         size_t (*step)(void*, size_t, double*, bool*),
                         void (*reset)(void*),
                         size_t n_episodes) {
    if (!model || !env || !get_state || !step || !reset || n_episodes == 0) {
        return 0.0;
    }
    
    double total_reward = 0.0;
    
    for (size_t episode = 0; episode < n_episodes; episode++) {
        reset(env);
        size_t state = get_state(env);
        
        double episode_reward = 0.0;
        bool done = false;
        size_t steps = 0;
        
        while (!done && steps < 1000) {
            // Use greedy policy (no exploration)
            size_t action = qlearning_select_greedy_action(model, state);
            
            double reward;
            size_t next_state = step(env, action, &reward, &done);
            
            episode_reward += reward;
            state = next_state;
            steps++;
        }
        
        total_reward += episode_reward;
    }
    
    return total_reward / n_episodes;
}

// Get average reward over last N episodes
double qlearning_get_average_reward(const QLearningModel* model, size_t last_n_episodes) {
    if (!model || !model->episode_rewards || model->total_episodes == 0) {
        return 0.0;
    }
    
    size_t n = last_n_episodes;
    if (n > model->total_episodes) {
        n = model->total_episodes;
    }
    
    double sum = 0.0;
    size_t start = model->total_episodes - n;
    
    for (size_t i = start; i < model->total_episodes; i++) {
        if (i < model->episode_rewards_capacity) {
            sum += model->episode_rewards[i];
        }
    }
    
    return sum / n;
}

// Print statistics
void qlearning_print_statistics(const QLearningModel* model) {
    if (!model) return;
    
    printf("Q-Learning Statistics:\n");
    printf("  Total Episodes: %zu\n", model->total_episodes);
    printf("  Total Steps: %zu\n", model->total_steps);
    printf("  Q-Table Size: %zu / %zu\n", model->q_table_size, model->q_table_capacity);
    printf("  Current Epsilon: %.4f\n", model->epsilon);
    
    if (model->total_episodes > 0) {
        printf("  Average Reward (last 100): %.2f\n", 
               qlearning_get_average_reward(model, 100));
    }
}

// Get Q-table size
size_t qlearning_get_q_table_size(const QLearningModel* model) {
    return model ? model->q_table_size : 0;
}

// Export Q-table
void qlearning_export_q_table(const QLearningModel* model, QTableEntry* buffer, size_t buffer_size) {
    if (!model || !buffer || buffer_size == 0) return;
    
    size_t exported = 0;
    for (size_t i = 0; i < model->q_table_capacity && exported < buffer_size; i++) {
        QTableEntry* entry = &model->q_table[i];
        if (entry->q_value != 0.0 || entry->state != 0 || entry->action != 0) {
            buffer[exported++] = *entry;
        }
    }
}
