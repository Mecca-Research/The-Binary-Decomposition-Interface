# CRRSS Reinforcement Learning Architecture

**Version:** 1.0.0  
**Phase:** 3 - Complete Intelligence  
**Date:** October 2025

## Overview

This document describes the reinforcement learning (RL) architecture for CRRSS, enabling the system to learn optimal code analysis strategies through interaction with codebases and feedback from developers.

## RL Problem Formulation

### State Space

The state represents the current context of code analysis:

```python
State = {
    # Code context
    'current_file': str,
    'current_function': str,
    'code_complexity': float,
    'lines_analyzed': int,
    
    # Analysis history
    'bugs_found': int,
    'false_positives': int,
    'analysis_time': float,
    
    # Module context
    'module_criticality': float,
    'dependency_count': int,
    'bug_history': List[Bug],
    
    # Resource constraints
    'time_budget': float,
    'memory_budget': float
}
```

### Action Space

Actions represent analysis decisions:

```python
Action = {
    # Analysis strategy
    'analysis_depth': int,  # 1=shallow, 5=deep
    'analysis_type': str,   # 'bugs', 'performance', 'security'
    
    # Focus areas
    'focus_functions': List[str],
    'focus_patterns': List[str],
    
    # Resource allocation
    'time_allocation': float,
    'thread_count': int,
    
    # Profile selection
    'profile': ProfileType  # Conservative, Aggressive, etc.
}
```

### Reward Function

Reward quantifies the quality of analysis decisions:

```python
def calculate_reward(state, action, result):
    reward = 0.0
    
    # Positive rewards
    reward += result['true_positives'] * 10.0
    reward += result['critical_bugs_found'] * 50.0
    reward += result['bugs_prevented'] * 20.0
    
    # Negative penalties
    reward -= result['false_positives'] * 5.0
    reward -= result['false_negatives'] * 15.0
    reward -= result['analysis_time'] * 0.1  # Time penalty
    
    # Efficiency bonus
    if result['bugs_found'] > 0 and result['analysis_time'] < state['time_budget']:
        reward += 10.0
    
    return reward
```

## Architecture Components

```
┌──────────────────────────────────────────────────────────┐
│              RL Agent Architecture                       │
│                                                          │
│  ┌────────────┐     ┌────────────┐     ┌────────────┐  │
│  │   Policy   │────→│   Value    │────→│  Action    │  │
│  │  Network   │     │  Network   │     │  Selection │  │
│  └────────────┘     └────────────┘     └────────────┘  │
│        ↑                  ↑                    │         │
│        │                  │                    ↓         │
│  ┌────────────────────────────────────────────────────┐ │
│  │            Experience Replay Buffer                │ │
│  └────────────────────────────────────────────────────┘ │
│        ↑                                       │         │
│        │                                       ↓         │
│  ┌────────────┐                         ┌────────────┐  │
│  │  Reward    │←────────────────────────│Environment │  │
│  │  Function  │                         │ (Codebase) │  │
│  └────────────┘                         └────────────┘  │
└──────────────────────────────────────────────────────────┘
```

## RL Algorithm: Deep Q-Network (DQN)

### Network Architecture

```python
class DQN(nn.Module):
    def __init__(self, state_dim, action_dim):
        super().__init__()
        
        # State encoder
        self.encoder = nn.Sequential(
            nn.Linear(state_dim, 256),
            nn.ReLU(),
            nn.Linear(256, 128),
            nn.ReLU(),
        )
        
        # Q-value head
        self.q_head = nn.Sequential(
            nn.Linear(128, 64),
            nn.ReLU(),
            nn.Linear(64, action_dim)
        )
    
    def forward(self, state):
        features = self.encoder(state)
        q_values = self.q_head(features)
        return q_values
```

### Training Algorithm

```python
class RLAgent:
    def __init__(self, state_dim, action_dim):
        self.q_network = DQN(state_dim, action_dim)
        self.target_network = DQN(state_dim, action_dim)
        self.optimizer = optim.Adam(self.q_network.parameters(), lr=0.001)
        self.replay_buffer = ReplayBuffer(capacity=10000)
        
        self.epsilon = 1.0  # Exploration rate
        self.epsilon_decay = 0.995
        self.epsilon_min = 0.01
        self.gamma = 0.99  # Discount factor
    
    def select_action(self, state):
        # Epsilon-greedy policy
        if random.random() < self.epsilon:
            return random.randint(0, self.action_dim - 1)
        else:
            with torch.no_grad():
                q_values = self.q_network(state)
                return q_values.argmax().item()
    
    def train(self, batch_size=32):
        if len(self.replay_buffer) < batch_size:
            return
        
        # Sample batch
        batch = self.replay_buffer.sample(batch_size)
        states, actions, rewards, next_states, dones = batch
        
        # Compute current Q values
        q_values = self.q_network(states).gather(1, actions)
        
        # Compute target Q values
        with torch.no_grad():
            next_q_values = self.target_network(next_states).max(1)[0]
            target_q_values = rewards + self.gamma * next_q_values * (1 - dones)
        
        # Compute loss
        loss = F.mse_loss(q_values, target_q_values.unsqueeze(1))
        
        # Optimize
        self.optimizer.zero_grad()
        loss.backward()
        self.optimizer.step()
        
        # Update epsilon
        self.epsilon = max(self.epsilon_min, self.epsilon * self.epsilon_decay)
```

## Policy Learning

### Policy Types

#### 1. Conservative Policy
- Prioritizes safety and accuracy
- Lower exploration rate
- Prefers well-tested analysis strategies

#### 2. Aggressive Policy
- Maximizes bug detection
- Higher exploration rate
- Tries novel analysis approaches

#### 3. Adaptive Policy
- Learns optimal balance
- Context-dependent behavior
- Adjusts based on feedback

### Policy Network

```python
class PolicyNetwork(nn.Module):
    def __init__(self, state_dim, action_dim):
        super().__init__()
        
        self.network = nn.Sequential(
            nn.Linear(state_dim, 256),
            nn.ReLU(),
            nn.Linear(256, 128),
            nn.ReLU(),
            nn.Linear(128, action_dim),
            nn.Softmax(dim=-1)
        )
    
    def forward(self, state):
        return self.network(state)
```

## Experience Replay

```python
class ReplayBuffer:
    def __init__(self, capacity):
        self.buffer = deque(maxlen=capacity)
    
    def push(self, state, action, reward, next_state, done):
        self.buffer.append((state, action, reward, next_state, done))
    
    def sample(self, batch_size):
        batch = random.sample(self.buffer, batch_size)
        states, actions, rewards, next_states, dones = zip(*batch)
        return (torch.stack(states),
                torch.tensor(actions),
                torch.tensor(rewards),
                torch.stack(next_states),
                torch.tensor(dones))
```

## Training Environment

```python
class CodeAnalysisEnvironment:
    def __init__(self, codebase):
        self.codebase = codebase
        self.current_file = None
        self.analysis_results = {}
    
    def reset(self):
        """Reset environment to initial state"""
        self.current_file = self.codebase.get_random_file()
        return self.get_state()
    
    def step(self, action):
        """Execute action and return next state, reward, done"""
        # Perform analysis based on action
        results = self.perform_analysis(action)
        
        # Calculate reward
        reward = self.calculate_reward(results)
        
        # Get next state
        next_state = self.get_state()
        
        # Check if episode is done
        done = self.is_done()
        
        return next_state, reward, done, results
    
    def get_state(self):
        """Get current state representation"""
        return {
            'file': self.current_file,
            'complexity': self.calculate_complexity(),
            'history': self.get_bug_history(),
            ...
        }
```

## Training Loop

```python
def train_rl_agent(agent, env, num_episodes=1000):
    for episode in range(num_episodes):
        state = env.reset()
        total_reward = 0
        done = False
        
        while not done:
            # Select action
            action = agent.select_action(state)
            
            # Execute action
            next_state, reward, done, info = env.step(action)
            
            # Store experience
            agent.replay_buffer.push(state, action, reward, next_state, done)
            
            # Train agent
            agent.train()
            
            # Update state
            state = next_state
            total_reward += reward
        
        # Update target network periodically
        if episode % 10 == 0:
            agent.target_network.load_state_dict(agent.q_network.state_dict())
        
        print(f"Episode {episode}: Total Reward = {total_reward}")
```

## Model Deployment

### Inference Pipeline

```python
class RLInferenceEngine:
    def __init__(self, model_path):
        self.agent = self.load_agent(model_path)
        self.agent.eval()
    
    def recommend_analysis_strategy(self, codebase_state):
        """Recommend optimal analysis strategy"""
        state = self.preprocess_state(codebase_state)
        with torch.no_grad():
            action = self.agent.select_action(state)
        return self.decode_action(action)
```

## Conclusion

This RL architecture enables CRRSS to continuously improve its code analysis strategies through feedback and experience, optimizing the balance between bug detection accuracy, analysis speed, and resource utilization.
