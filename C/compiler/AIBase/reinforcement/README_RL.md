
# Reinforcement Learning Framework

This directory contains the Q-learning algorithm and a general-purpose reinforcement learning framework for the BDI compiler.

## Q-Learning Algorithm

Q-learning is a model-free reinforcement learning algorithm that learns the value of actions in different states. It uses temporal difference (TD) learning to update Q-values based on the Bellman equation.

### Features

- **Q-table**: Hash map-based storage for state-action values
- **Epsilon-greedy policy**: Balances exploration and exploitation
- **TD learning**: Updates Q-values using: Q(s,a) ← Q(s,a) + α[r + γ max Q(s',a') - Q(s,a)]
- **Episode management**: Tracks training progress and convergence
- **Configurable parameters**: Learning rate (α), discount factor (γ), exploration rate (ε)

### Usage Example

```c
#include "q_learning.h"

// Create Q-learning model
QLearningConfig config = qlearning_default_config();
config.alpha = 0.1;           // Learning rate
config.gamma = 0.99;          // Discount factor
config.epsilon = 1.0;         // Initial exploration rate
config.epsilon_decay = 0.995; // Decay rate

QLearningModel* model = qlearning_create(n_states, n_actions, config);

// Training loop
for (size_t episode = 0; episode < 1000; episode++) {
    qlearning_train_episode(model, env,
                           get_state_fn,
                           step_fn,
                           reset_fn);
}

// Prediction (greedy policy)
size_t state = get_current_state();
size_t action = qlearning_select_greedy_action(model, state);

// Cleanup
qlearning_destroy(model);
```

### API Reference

#### Model Creation
- `qlearning_create()`: Create Q-learning model with specified state/action spaces
- `qlearning_destroy()`: Free model resources
- `qlearning_default_config()`: Get default configuration

#### Q-table Operations
- `qlearning_get_q_value()`: Get Q-value for state-action pair
- `qlearning_set_q_value()`: Set Q-value for state-action pair
- `qlearning_get_max_q_value()`: Get maximum Q-value for a state
- `qlearning_get_best_action()`: Get action with highest Q-value

#### Policy
- `qlearning_select_action()`: Select action using epsilon-greedy policy
- `qlearning_select_greedy_action()`: Select best action (no exploration)
- `qlearning_select_random_action()`: Select random action

#### Learning
- `qlearning_update()`: Update Q-value using TD learning rule
- `qlearning_update_epsilon()`: Decay exploration rate
- `qlearning_train_episode()`: Train for one episode

#### Evaluation
- `qlearning_evaluate()`: Evaluate model performance
- `qlearning_get_average_reward()`: Get average reward over last N episodes
- `qlearning_print_statistics()`: Print training statistics

## RL Framework

A general-purpose framework for implementing various reinforcement learning algorithms.

### Components

1. **Environment Interface**: Defines how agents interact with environments
2. **Agent Interface**: Defines agent behavior (action selection, learning)
3. **Trajectory**: Stores episode transitions for analysis
4. **Metrics**: Tracks training progress and convergence

### Environment Interface

```c
typedef struct {
    size_t (*get_state)(void* env_data);
    size_t (*step)(void* env_data, size_t action, double* reward, bool* done);
    void (*reset)(void* env_data);
    size_t (*get_state_space_size)(void* env_data);
    size_t (*get_action_space_size)(void* env_data);
    void (*render)(void* env_data);
} RLEnvironmentInterface;
```

### Agent Interface

```c
typedef struct {
    size_t (*select_action)(void* agent_data, size_t state);
    void (*update)(void* agent_data, size_t state, size_t action, 
                   double reward, size_t next_state, bool done);
    double (*get_value)(void* agent_data, size_t state);
    double (*get_q_value)(void* agent_data, size_t state, size_t action);
} RLAgentInterface;
```

### Usage Example

```c
#include "rl_framework.h"

// Create environment
RLEnvironment* env = rl_env_create(env_data, env_interface);

// Create agent
RLAgent* agent = rl_agent_create(agent_data, agent_interface, n_states, n_actions);

// Training configuration
RLTrainingConfig config = rl_default_training_config();
config.max_episodes = 1000;
config.verbose = true;

// Train agent
RLMetrics* metrics = nullptr;
rl_train(agent, env, config, &metrics);

// Print results
rl_metrics_print(metrics);

// Cleanup
rl_metrics_destroy(metrics);
rl_agent_destroy(agent);
rl_env_destroy(env);
```

### API Reference

#### Environment
- `rl_env_create()`: Create environment wrapper
- `rl_env_destroy()`: Free environment
- `rl_env_reset()`: Reset environment to initial state
- `rl_env_step()`: Take action in environment
- `rl_env_get_state()`: Get current state
- `rl_env_render()`: Render environment (optional)

#### Agent
- `rl_agent_create()`: Create agent wrapper
- `rl_agent_destroy()`: Free agent
- `rl_agent_select_action()`: Select action given state
- `rl_agent_update()`: Update agent with experience
- `rl_agent_get_value()`: Get state value estimate
- `rl_agent_get_q_value()`: Get state-action value estimate

#### Trajectory
- `rl_trajectory_create()`: Create trajectory storage
- `rl_trajectory_destroy()`: Free trajectory
- `rl_trajectory_add()`: Add transition to trajectory
- `rl_trajectory_reset()`: Clear trajectory
- `rl_trajectory_compute_return()`: Compute discounted return

#### Metrics
- `rl_metrics_create()`: Create metrics tracker
- `rl_metrics_destroy()`: Free metrics
- `rl_metrics_add_episode()`: Record episode statistics
- `rl_metrics_compute_statistics()`: Compute summary statistics
- `rl_metrics_print()`: Print metrics
- `rl_metrics_check_convergence()`: Check if training has converged

#### Training
- `rl_train_episode()`: Train agent for one episode
- `rl_evaluate_agent()`: Evaluate agent performance
- `rl_train()`: Full training loop with metrics

#### Utilities
- `rl_compute_td_error()`: Compute temporal difference error
- `rl_compute_gae()`: Compute generalized advantage estimation
- `rl_default_training_config()`: Get default training configuration

## Integration with BDI

The RL framework integrates with the BDI compiler through:

1. **CodeGen**: `emit_qlearning_model()` generates bytecode for Q-learning models
2. **VM Opcodes**: `OP_QLEARN_UPDATE`, `OP_QLEARN_SELECT_ACTION` for execution
3. **BTL Tokens**: Binary encoding for Q-learning operations
4. **DSL**: Declarative syntax for defining Q-learning agents

## Performance Considerations

- Q-table uses hash map with linear probing for efficient lookups
- Automatic resizing when load factor exceeds 70%
- Episode rewards tracked for convergence analysis
- Epsilon decay for exploration-exploitation balance

## Future Extensions

- Deep Q-Networks (DQN)
- Policy gradient methods (REINFORCE, A3C)
- Actor-Critic algorithms
- Multi-agent reinforcement learning
- Continuous action spaces

## References

- Watkins, C. J., & Dayan, P. (1992). Q-learning. Machine learning, 8(3-4), 279-292.
- Sutton, R. S., & Barto, A. G. (2018). Reinforcement learning: An introduction. MIT press.
