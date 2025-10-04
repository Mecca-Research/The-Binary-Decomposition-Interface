
# BDI AI Trainer - Phase 6

Complete machine learning training infrastructure for the Binary Decomposition Interface, implementing automatic differentiation, optimization algorithms, loss functions, and training loops.

## Overview

Phase 6 provides a comprehensive ML training framework with three main components:

1. **Automatic Differentiation** - Forward and reverse mode AD for gradient computation
2. **Optimizer Suite** - SGD, Adam, RMSprop with learning rate scheduling
3. **Loss Functions & Training** - Loss functions, metrics, and training infrastructure

## Components

### 1. Automatic Differentiation (`autodiff/`)

**Forward Mode AD** (`forward_ad.h/c`)
- Dual number arithmetic for computing derivatives
- Efficient for functions with few inputs, many outputs
- Supports all standard mathematical operations

**Reverse Mode AD** (`reverse_ad.h/c`)
- Gradient tape for backpropagation
- Efficient for functions with many inputs, few outputs
- Records computational graph during forward pass

**Gradient Utilities** (`gradient.h/c`)
- Numerical gradient computation for verification
- Gradient checking utilities
- Vector and matrix operations with gradients

### 2. Optimizer Suite (`optimizers/`)

**SGD** (`sgd.h/c`)
- Stochastic Gradient Descent
- Momentum and Nesterov momentum
- Weight decay (L2 regularization)

**Adam** (`adam.h/c`)
- Adaptive Moment Estimation
- First and second moment estimates
- Bias correction
- AMSGrad variant

**RMSprop** (`rmsprop.h/c`)
- Root Mean Square Propagation
- Adaptive learning rates
- Centered variant
- Optional momentum

**Learning Rate Schedulers** (`lr_scheduler.h/c`)
- Constant, Step, Exponential
- Cosine annealing
- Cosine warm restarts
- Linear warmup

### 3. Loss Functions & Training (`loss/`, `metrics/`, `training/`)

**Loss Functions** (`loss.h/c`)
- Mean Squared Error (MSE)
- Cross Entropy
- Binary Cross Entropy
- Mean Absolute Error (MAE)
- Huber Loss

**Metrics** (`metrics.h/c`)
- Accuracy (binary and multi-class)
- Precision, Recall, F1 Score
- Confusion Matrix
- Top-k Accuracy
- R² Score, RMSE

**Training Infrastructure** (`training.h/c`)
- Training loop with batching
- Validation support
- Early stopping
- Training history tracking
- Model evaluation

## Quick Start

### Basic Training Example

```c
#include "trainer/trainer.h"

// Define your model
Model model = {
    .model_data = my_model,
    .forward = my_forward_function,
    .backward = my_backward_function,
    .get_params = my_get_params,
    .get_grads = my_get_grads,
    .zero_grad = my_zero_grad
};

// Configure training
TrainingConfig config = training_config_classification(100, 32);
config.learning_rate = 0.001;
config.optimizer_type = OPT_ADAM;

// Train model
TrainingHistory* history = train_model(
    &model,
    train_data, train_labels,
    num_train_samples,
    input_size, output_size,
    config
);

// Print results
training_history_print(history);
training_history_destroy(history);
```

### Using Automatic Differentiation

```c
#include "trainer/autodiff/forward_ad.h"

// Forward mode AD
Dual x = dual_variable(2.0);
Dual y = dual_mul(x, x);  // y = x²
printf("f(2) = %.2f, f'(2) = %.2f\n", y.value, y.derivative);
// Output: f(2) = 4.00, f'(2) = 4.00
```

```c
#include "trainer/autodiff/reverse_ad.h"

// Reverse mode AD
GradientTape* tape = tape_create(100);
size_t x_id = tape_record_variable(tape, 2.0);
size_t y_id = tape_record_mul(tape, x_id, x_id, 2.0, 2.0);

tape_backward(tape, y_id);
double grad = tape_get_gradient(tape, x_id);
printf("∂f/∂x = %.2f\n", grad);  // Output: 4.00

tape_destroy(tape);
```

### Using Optimizers

```c
#include "trainer/optimizers/adam.h"

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

### Computing Loss

```c
#include "trainer/loss/loss.h"

// MSE Loss
LossResult* result = loss_mse(predictions, targets, size);
printf("Loss: %.6f\n", result->loss);

// Use gradients for backpropagation
// result->gradients contains ∂L/∂predictions

loss_result_destroy(result);
```

### Evaluating Metrics

```c
#include "trainer/metrics/metrics.h"

// Classification accuracy
double acc = metric_accuracy(predictions, targets, batch_size, num_classes);

// Binary classification metrics
ClassificationMetrics metrics = metric_binary_classification(
    predictions, targets, size, 0.5
);
printf("Precision: %.4f, Recall: %.4f, F1: %.4f\n",
       metrics.precision, metrics.recall, metrics.f1_score);
```

## Architecture

```
trainer/
├── autodiff/           # Automatic differentiation
│   ├── forward_ad.c/h  # Forward mode AD
│   ├── reverse_ad.c/h  # Reverse mode AD (backprop)
│   ├── gradient.c/h    # Gradient utilities
│   └── README.md
├── optimizers/         # Optimization algorithms
│   ├── sgd.c/h         # SGD with momentum
│   ├── adam.c/h        # Adam optimizer
│   ├── rmsprop.c/h     # RMSprop optimizer
│   ├── lr_scheduler.c/h # Learning rate schedulers
│   └── README.md
├── loss/               # Loss functions
│   ├── loss.c/h        # MSE, CE, BCE, MAE, Huber
│   └── README.md
├── metrics/            # Evaluation metrics
│   ├── metrics.c/h     # Accuracy, precision, recall, F1
│   └── README.md
├── training/           # Training infrastructure
│   ├── training.c/h    # Training loops and config
│   └── README.md
├── trainer.h           # Main header (includes all)
└── README.md           # This file
```

## Testing

Comprehensive test suite with 280+ tests:

```bash
# Build and run all tests
make test_trainer
./test_trainer

# Individual test suites
./test_forward_ad      # 40+ tests
./test_reverse_ad      # 40+ tests
./test_gradient        # 20+ tests
./test_sgd            # 25+ tests
./test_adam           # 25+ tests
./test_rmsprop        # 20+ tests
./test_lr_scheduler   # 15+ tests
./test_loss           # 35+ tests
./test_metrics        # 30+ tests
./test_training       # 30+ tests
```

## Performance Considerations

1. **Memory Management**: All structures use proper malloc/free
2. **Numerical Stability**: Epsilon values prevent division by zero
3. **Batch Processing**: Efficient batching for large datasets
4. **Gradient Tape**: Automatic memory management and resizing
5. **Thread Safety**: Atomic operations where needed

## Integration with BDI

The trainer integrates with BDI's computational graph:
- Automatic differentiation works with BDI nodes
- Optimizers update BDI graph parameters
- Loss functions operate on BDI tensor outputs
- Training loops orchestrate BDI graph execution

## Dependencies

- Phase 5 (Kernel Enhancement) - for graph operations
- Standard C library (math.h, stdlib.h, stdio.h)
- No external dependencies

## Future Enhancements

- GPU acceleration for training
- Distributed training support
- More optimizer variants (AdaGrad, AdaDelta)
- Advanced learning rate schedules
- Gradient clipping and normalization
- Mixed precision training

## References

- Automatic Differentiation: Griewank & Walther (2008)
- Adam: Kingma & Ba (2014)
- RMSprop: Hinton et al. (2012)
- Learning Rate Scheduling: Loshchilov & Hutter (2016)

## License

Part of the Binary Decomposition Interface project.
