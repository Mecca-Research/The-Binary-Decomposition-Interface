
# BDI Optimizer Suite

This module implements a comprehensive suite of optimization algorithms for training neural networks and machine learning models in the Binary Decomposition Interface.

## Optimizers

### SGD (Stochastic Gradient Descent)

Classic gradient descent with optional momentum and weight decay.

**Features:**
- Standard SGD
- Momentum (accelerates convergence)
- Nesterov momentum (look-ahead gradient)
- Weight decay (L2 regularization)

**Usage:**
```c
#include "sgd.h"

// Create SGD optimizer with momentum
SGDConfig config = sgd_config_with_momentum(0.01, 0.9);
SGDOptimizer* opt = sgd_create(num_params, config);

// Training loop
for (int epoch = 0; epoch < num_epochs; epoch++) {
    // Compute gradients...
    sgd_step(opt, params, gradients);
}

sgd_destroy(opt);
```

### Adam (Adaptive Moment Estimation)

Adaptive learning rate optimizer combining momentum and RMSprop.

**Features:**
- Adaptive learning rates per parameter
- First and second moment estimates
- Bias correction
- AMSGrad variant for better convergence
- Weight decay support

**Usage:**
```c
#include "adam.h"

// Create Adam optimizer
AdamConfig config = adam_default_config();
config.learning_rate = 0.001;
AdamOptimizer* opt = adam_create(num_params, config);

// Training loop
for (int epoch = 0; epoch < num_epochs; epoch++) {
    // Compute gradients...
    adam_step(opt, params, gradients);
}

adam_destroy(opt);
```

### RMSprop (Root Mean Square Propagation)

Adaptive learning rate method that divides by running average of gradient magnitudes.

**Features:**
- Adaptive learning rates
- Moving average of squared gradients
- Centered variant (subtracts mean)
- Optional momentum
- Weight decay support

**Usage:**
```c
#include "rmsprop.h"

// Create RMSprop optimizer
RMSpropConfig config = rmsprop_default_config();
config.learning_rate = 0.001;
RMSpropOptimizer* opt = rmsprop_create(num_params, config);

// Training loop
for (int epoch = 0; epoch < num_epochs; epoch++) {
    // Compute gradients...
    rmsprop_step(opt, params, gradients);
}

rmsprop_destroy(opt);
```

## Learning Rate Schedulers

Dynamic learning rate adjustment during training.

**Scheduler Types:**
1. **Constant**: Fixed learning rate
2. **Step**: Multiply by gamma every step_size epochs
3. **Exponential**: Exponential decay
4. **Cosine**: Cosine annealing
5. **Cosine Warm Restarts**: Periodic cosine annealing
6. **Linear Warmup**: Gradual increase from low to target LR

**Usage:**
```c
#include "lr_scheduler.h"

// Create cosine annealing scheduler
LRSchedulerConfig config = lr_cosine_config(0.1, 100, 0.001);
LRScheduler* scheduler = lr_scheduler_create(config);

// Training loop
for (int epoch = 0; epoch < num_epochs; epoch++) {
    double lr = lr_scheduler_get_lr(scheduler);
    
    // Update optimizer learning rate
    optimizer->config.learning_rate = lr;
    
    // Train...
    
    lr_scheduler_step(scheduler);
}

lr_scheduler_destroy(scheduler);
```

## Optimizer Comparison

| Optimizer | Best For | Pros | Cons |
|-----------|----------|------|------|
| SGD | Simple problems, fine-tuning | Stable, well-understood | Slow convergence, requires tuning |
| SGD+Momentum | Most problems | Faster convergence | Requires momentum tuning |
| Adam | Default choice, most problems | Fast, adaptive, robust | Can overfit, memory overhead |
| RMSprop | RNNs, non-stationary problems | Adaptive, handles sparse gradients | Less stable than Adam |

## Hyperparameter Guidelines

### SGD
- Learning rate: 0.01 - 0.1
- Momentum: 0.9 - 0.99
- Weight decay: 1e-4 - 1e-5

### Adam
- Learning rate: 0.001 - 0.0001
- Beta1: 0.9
- Beta2: 0.999
- Epsilon: 1e-8

### RMSprop
- Learning rate: 0.001 - 0.01
- Alpha: 0.99
- Epsilon: 1e-8

## Testing

Comprehensive test suite with 80+ tests covering:
- All optimizer variants
- Learning rate schedulers
- Convergence tests
- Edge cases

Run tests:
```bash
make test_optimizers
./test_optimizers
```

## Performance Tips

1. **Start with Adam**: Good default for most problems
2. **Use learning rate scheduling**: Improves final accuracy
3. **Tune weight decay**: Prevents overfitting
4. **Monitor gradients**: Check for vanishing/exploding gradients
5. **Batch size matters**: Larger batches need higher learning rates
