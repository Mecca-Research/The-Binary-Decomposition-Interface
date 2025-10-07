# ML Primitives - Compiler-Native Machine Learning

## Overview

This implementation embeds 4 foundational machine learning algorithms as **compiler-native primitives** in the BDI C folder. The compiler now speaks ML natively—generating code in binary that thinks.

## Architecture

### Core Components

1. **AIBase/** - ML Algorithm Implementations
   - `linear/` - Linear Regression with gradient descent
   - `tree/` - Decision Tree with CART algorithm
   - `kernel/` - Support Vector Machine with kernel methods
   - `clustering/` - K-means clustering with Lloyd's algorithm

2. **CodeGen/** - IR and Bytecode Generation
   - `ml_codegen.c/h` - Emits IR opcodes for ML operations
   - Model serialization/deserialization
   - Integration with existing CodeGen infrastructure

3. **VM/** - Bytecode Execution
   - `ml_ops.c/h` - VM execution handlers for ML opcodes
   - Stack-based execution model
   - Model registry and lifecycle management

4. **BTL/** - Binary Translation Layer
   - `specs/ml.btl` - Token specifications for ML operations
   - Binary encoding format for ML primitives
   - Integration with BTL ISA

## Algorithms

### 1. Linear Regression (`compiler/AIBase/linear/`)

**Features:**
- Matrix operations (dot product, matrix-vector multiply)
- Gradient descent optimization
- Mean Squared Error (MSE) loss
- Forward pass prediction
- Integration with existing `gradient/` system

**API:**
```c
// Create model
LinearRegressionConfig config = linear_regression_default_config();
LinearRegressionModel* model = linear_regression_create(n_features, config);

// Train
linear_regression_fit(model, X, y, n_samples, n_features);

// Predict
double prediction = linear_regression_predict_single(model, x);

// Cleanup
linear_regression_destroy(model);
```

**Example:**
```c
// Training data: y = 2x + 3
double X[] = {1.0, 2.0, 3.0, 4.0, 5.0};
double y[] = {5.0, 7.0, 9.0, 11.0, 13.0};

LinearRegressionConfig config = linear_regression_default_config();
config.learning_rate = 0.01;
config.max_iterations = 1000;

LinearRegressionModel* model = linear_regression_create(1, config);
linear_regression_fit(model, X, y, 5, 1);

double test_x = 6.0;
double pred = linear_regression_predict_single(model, &test_x);
// pred ≈ 15.0
```

### 2. Decision Tree (`compiler/AIBase/tree/`)

**Features:**
- Gini impurity and information gain calculations
- Recursive node splitting with CART algorithm
- Binary tree structure
- Configurable depth and splitting criteria
- Prediction via tree traversal

**API:**
```c
// Create model
DecisionTreeConfig config = decision_tree_default_config();
DecisionTreeModel* model = decision_tree_create(config);

// Train
decision_tree_fit(model, X, y, n_samples, n_features);

// Predict
double prediction = decision_tree_predict_single(model, x);

// Cleanup
decision_tree_destroy(model);
```

**Example:**
```c
// Binary classification data
double X[] = {1.0, 2.0, 3.0, 7.0, 8.0, 9.0};
double y[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};

DecisionTreeConfig config = decision_tree_default_config();
config.max_depth = 5;
config.min_samples_split = 2;

DecisionTreeModel* model = decision_tree_create(config);
decision_tree_fit(model, X, y, 6, 1);

double test_x = 5.0;
double pred = decision_tree_predict_single(model, &test_x);
```

### 3. Support Vector Machine (`compiler/AIBase/kernel/`)

**Features:**
- Linear and RBF kernel functions
- Support vector storage and management
- Margin maximization with SMO algorithm
- Decision function computation
- Binary classification

**API:**
```c
// Create model
SVMConfig config = svm_default_config();
SVMModel* model = svm_create(n_features, config);

// Train
svm_fit(model, X, y, n_samples, n_features);

// Predict
double prediction = svm_predict_single(model, x);

// Cleanup
svm_destroy(model);
```

**Example:**
```c
// Linearly separable data
double X[] = {1.0, 1.0, 2.0, 2.0, 6.0, 6.0, 7.0, 7.0};
double y[] = {-1.0, -1.0, 1.0, 1.0};

SVMConfig config = svm_default_config();
config.kernel_type = SVM_KERNEL_RBF;
config.gamma = 0.5;
config.C = 1.0;

SVMModel* model = svm_create(2, config);
svm_fit(model, X, y, 4, 2);

double test_x[] = {1.5, 1.5};
double pred = svm_predict_single(model, test_x);
// pred = -1.0 or 1.0
```

### 4. K-means Clustering (`compiler/AIBase/clustering/`)

**Features:**
- Euclidean distance calculation
- K-means++ centroid initialization
- Lloyd's algorithm implementation
- Cluster assignment and centroid updates
- Convergence detection with inertia

**API:**
```c
// Create model
KMeansConfig config = kmeans_default_config(n_clusters);
KMeansModel* model = kmeans_create(n_features, config);

// Train
kmeans_fit(model, X, n_samples, n_features);

// Predict
size_t cluster = kmeans_predict_single(model, x);

// Cleanup
kmeans_destroy(model);
```

**Example:**
```c
// Two-cluster data
double X[] = {1.0, 1.0, 2.0, 2.0, 8.0, 8.0, 9.0, 9.0};

KMeansConfig config = kmeans_default_config(2);
config.max_iterations = 300;
config.tolerance = 1e-4;

KMeansModel* model = kmeans_create(2, config);
kmeans_fit(model, X, 4, 2);

double test_x[] = {1.5, 1.5};
size_t cluster = kmeans_predict_single(model, test_x);
// cluster = 0 or 1
```

## Integration with BDI Infrastructure

### 1. Gradient System Integration

Linear regression connects to the existing `trainer/autodiff/gradient.c` system:

```c
// Use existing gradient computation
#include "../../trainer/autodiff/gradient.h"

void linear_regression_compute_gradients(
    const LinearRegressionModel* model,
    const double* X, const double* y,
    size_t n_samples, size_t n_features,
    double* weight_gradients, double* bias_gradient
);
```

### 2. Loss Functions Integration

All algorithms use the existing `trainer/loss/loss.c` infrastructure:

```c
#include "../../trainer/loss/loss.h"

// MSE loss for linear regression
double loss = linear_regression_mse_loss(model, X, y, n_samples, n_features);
```

### 3. Optimizer Integration

Linear regression can use existing optimizers from `trainer/optimizers/`:

```c
#include "../../trainer/optimizers/sgd.h"

// Use SGD optimizer for weight updates
SGDOptimizer* opt = sgd_create(n_features, sgd_default_config());
sgd_step(opt, model->weights, weight_gradients);
```

### 4. CodeGen Integration

ML operations emit IR opcodes through `compiler/CodeGen/ml_codegen.c`:

```c
// Emit linear regression prediction
Chunk chunk;
chunk_init(&chunk);
emit_linear_regression_predict(&chunk, model_idx);

// Emit decision tree training
emit_decision_tree_train(&chunk, data_idx, labels_idx);
```

**ML Opcodes:**
- `OP_ML_LINEAR_REG_PREDICT` (100)
- `OP_ML_LINEAR_REG_TRAIN` (101)
- `OP_ML_TREE_PREDICT` (110)
- `OP_ML_TREE_TRAIN` (111)
- `OP_ML_SVM_PREDICT` (120)
- `OP_ML_SVM_TRAIN` (121)
- `OP_ML_KMEANS_PREDICT` (130)
- `OP_ML_KMEANS_TRAIN` (131)

### 5. VM Execution

ML opcodes execute through `compiler/VM/ml_ops.c`:

```c
// Create ML VM context
MLVMContext* ml_ctx = ml_vm_context_create();

// Register model
size_t handle = ml_vm_register_linear_regression(ml_ctx, model);

// Execute prediction
ml_vm_execute_linear_reg_predict(&vm, ml_ctx, handle);
```

### 6. BTL Tokenization

ML operations have binary token representations in `compiler/BTL/specs/ml.btl`:

```
# Linear Regression Prediction Token
[0xFF][0x10][0x02][n_features][feature_1]...[feature_n][0xFE]

# Decision Tree Training Token
[0xFF][0x11][0x01][max_depth][n_samples][n_features][X_data][y_data][0xFE]

# SVM with RBF Kernel Token
[0xFF][0x12][0x02][0x11][gamma][n_features][features][0xFE]

# K-means Clustering Token
[0xFF][0x13][0x01][n_clusters][n_samples][n_features][X_data][0xFE]
```

## Building and Testing

### Build System

Add to `C/Makefile`:

```makefile
# ML Primitives
ML_SRCS = compiler/AIBase/linear/linear_regression.c \
          compiler/AIBase/tree/decision_tree.c \
          compiler/AIBase/kernel/svm.c \
          compiler/AIBase/clustering/kmeans.c \
          compiler/CodeGen/ml_codegen.c \
          compiler/VM/ml_ops.c

ML_OBJS = $(ML_SRCS:.c=.o)
LIBML = libml.a

# ML Tests
ML_TESTS = compiler/tests/ml_linear_test \
           compiler/tests/ml_tree_test \
           compiler/tests/ml_svm_test \
           compiler/tests/ml_kmeans_test \
           compiler/tests/ml_vm_test

$(LIBML): $(ML_OBJS)
	ar rcs $@ $^

compiler/tests/ml_%_test: compiler/tests/ml_%_test.c $(LIBML)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBML) $(LDFLAGS)
```

### Running Tests

```bash
# Build ML library
make libml.a

# Build and run individual tests
make compiler/tests/ml_linear_test
./compiler/tests/ml_linear_test

make compiler/tests/ml_tree_test
./compiler/tests/ml_tree_test

make compiler/tests/ml_svm_test
./compiler/tests/ml_svm_test

make compiler/tests/ml_kmeans_test
./compiler/tests/ml_kmeans_test

make compiler/tests/ml_vm_test
./compiler/tests/ml_vm_test

# Run all ML tests
make test_ml
```

### Expected Test Output

```
=== Linear Regression ML Primitive Tests ===

Testing simple linear regression...
  Learned weight: 2.0000 (expected: 2.0)
  Learned bias: 3.0000 (expected: 3.0)
  Prediction for x=6.0: 15.0000 (expected: 15.0)
  Final MSE loss: 0.000000
  ✓ Simple linear regression test passed

=== All Linear Regression tests passed! ===
```

## Performance Characteristics

### Linear Regression
- **Training:** O(n_iterations × n_samples × n_features)
- **Prediction:** O(n_features)
- **Memory:** O(n_features)

### Decision Tree
- **Training:** O(n_samples × n_features × log(n_samples))
- **Prediction:** O(log(n_samples))
- **Memory:** O(n_nodes × n_features)

### SVM
- **Training:** O(n_samples² × n_features) (SMO algorithm)
- **Prediction:** O(n_support_vectors × n_features)
- **Memory:** O(n_support_vectors × n_features)

### K-means
- **Training:** O(n_iterations × n_samples × n_clusters × n_features)
- **Prediction:** O(n_clusters × n_features)
- **Memory:** O(n_clusters × n_features)

## Memory Safety

All implementations follow strict memory safety guidelines:

1. **Null checks** on all pointer parameters
2. **Bounds checking** for array accesses
3. **Proper cleanup** with destroy functions
4. **No memory leaks** - all allocations are freed
5. **Error handling** with boolean return values

Example:
```c
LinearRegressionModel* model = linear_regression_create(n_features, config);
if (!model) {
    // Handle allocation failure
    return false;
}

// Use model...

// Always cleanup
linear_regression_destroy(model);
```

## C23 Features Used

- `nullptr` keyword
- `static_assert` for compile-time checks
- `typeof` for type inference
- Modern struct initialization
- Compound literals

## Future Enhancements

### Phase 3 (Planned)
1. **Neural Networks** - Feedforward and backpropagation
2. **Random Forests** - Ensemble of decision trees
3. **Gradient Boosting** - XGBoost-style implementation
4. **PCA** - Dimensionality reduction

### Phase 4 (Planned)
1. **GPU Acceleration** - CUDA/OpenCL kernels
2. **SIMD Optimization** - AVX2/AVX-512 vectorization
3. **Parallel Training** - Multi-threaded algorithms
4. **Model Compression** - Quantization and pruning

## Contributing

When adding new ML algorithms:

1. Create algorithm implementation in `compiler/AIBase/<category>/`
2. Add CodeGen support in `compiler/CodeGen/ml_codegen.c`
3. Add VM execution in `compiler/VM/ml_ops.c`
4. Define BTL tokens in `compiler/BTL/specs/ml.btl`
5. Write comprehensive tests in `compiler/tests/`
6. Update this README with usage examples

## References

- **Linear Regression:** Gradient Descent Optimization
- **Decision Trees:** CART (Classification and Regression Trees)
- **SVM:** Sequential Minimal Optimization (SMO)
- **K-means:** Lloyd's Algorithm with k-means++ initialization

## License

Part of the BDI (Binary Decomposition Interface) Kernel project.

## Authors

BDI Kernel Team, 2024
