
# BDI Automatic Differentiation

This module implements automatic differentiation (AD) for the Binary Decomposition Interface, providing both forward-mode and reverse-mode differentiation capabilities.

## Components

### Forward Mode AD (`forward_ad.h/c`)

Forward-mode automatic differentiation using dual numbers. Each dual number carries both a value and its derivative.

**Key Features:**
- Dual number arithmetic (add, subtract, multiply, divide)
- Mathematical functions (sin, cos, exp, log, sqrt, pow, etc.)
- Activation functions (sigmoid, tanh, ReLU)
- Efficient for functions with few inputs and many outputs

**Usage Example:**
```c
#include "forward_ad.h"

// Compute f(x) = x^2 + 2x + 1 and its derivative at x=3
Dual x = dual_variable(3.0);  // x with derivative 1.0
Dual x_sq = dual_mul(x, x);   // x^2
Dual two_x = dual_mul(dual_constant(2.0), x);  // 2x
Dual result = dual_add(dual_add(x_sq, two_x), dual_constant(1.0));

printf("f(3) = %.2f, f'(3) = %.2f\n", result.value, result.derivative);
// Output: f(3) = 16.00, f'(3) = 8.00
```

### Reverse Mode AD (`reverse_ad.h/c`)

Reverse-mode automatic differentiation (backpropagation) using a gradient tape. Records operations during forward pass and computes gradients during backward pass.

**Key Features:**
- Gradient tape for recording computational graph
- Efficient for functions with many inputs and few outputs
- Supports all standard operations and activation functions
- Memory-efficient gradient computation

**Usage Example:**
```c
#include "reverse_ad.h"

// Compute gradients of f(x,y) = x*y + sin(x)
GradientTape* tape = tape_create(100);

size_t x_id = tape_record_variable(tape, 2.0);
size_t y_id = tape_record_variable(tape, 3.0);

size_t xy_id = tape_record_mul(tape, x_id, y_id, 2.0, 3.0);
size_t sin_x_id = tape_record_sin(tape, x_id, 2.0);
size_t result_id = tape_record_add(tape, xy_id, sin_x_id, 6.0, sin(2.0));

tape_backward(tape, result_id);

double grad_x = tape_get_gradient(tape, x_id);
double grad_y = tape_get_gradient(tape, y_id);

printf("∂f/∂x = %.4f, ∂f/∂y = %.4f\n", grad_x, grad_y);

tape_destroy(tape);
```

### Gradient Utilities (`gradient.h/c`)

Helper functions for gradient computation and verification.

**Key Features:**
- Numerical gradient computation for verification
- Gradient checking (comparing analytical vs numerical)
- Vector operations with gradients
- Matrix-vector multiplication with gradients

**Usage Example:**
```c
#include "gradient.h"

// Numerical gradient checking
double func(double* params, void* data) {
    return params[0] * params[0] + params[1] * params[1];
}

double params[2] = {3.0, 4.0};
double analytical[2] = {6.0, 8.0};  // 2*x, 2*y
double numerical[2];

for (int i = 0; i < 2; i++) {
    numerical[i] = numerical_gradient(func, params, i, 2, NULL, 1e-5);
}

bool correct = check_gradients(analytical, numerical, 2, 1e-5, 1e-8);
printf("Gradients %s\n", correct ? "match" : "differ");
```

## Performance Considerations

1. **Forward Mode**: Best for functions with few inputs (e.g., f: R^n → R^m where n << m)
2. **Reverse Mode**: Best for functions with few outputs (e.g., f: R^n → R^m where n >> m)
3. **Memory**: Reverse mode requires storing the computational graph
4. **Speed**: Forward mode is faster for small problems, reverse mode scales better

## Testing

Comprehensive test suite with 100+ tests covering:
- All dual number operations
- Gradient tape operations
- Numerical gradient verification
- Edge cases and error handling

Run tests:
```bash
make test_autodiff
./test_autodiff
```

## Integration with BDI

The autodiff module integrates with BDI's graph structure for:
- Computing gradients of loss functions
- Backpropagation through neural networks
- Optimization of computational graphs
- Automatic differentiation of BDI operations
