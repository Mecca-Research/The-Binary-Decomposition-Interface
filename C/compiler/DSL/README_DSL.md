
# ML DSL (Domain-Specific Language)

A declarative language for defining, training, and using machine learning models in the BDI compiler.

## Overview

The ML DSL provides an intuitive syntax for working with ML models without writing low-level code. It supports all BDI ML primitives: linear regression, decision trees, SVM, k-means, and Q-learning.

## Syntax

### Model Declaration

```btl
ml model <name> {
    type: <algorithm_type>;
    <parameter>: <value>;
    ...
}
```

### Training

```btl
train <model_name> with dataset "<path>" {
    epochs: <number>;
    batch_size: <number>;
    ...
};
```

### Prediction

```btl
predict <model_name> on input [<value1>, <value2>, ...];
```

## Supported Algorithms

### 1. Linear Regression

```btl
ml model linear_reg {
    type: linear_regression;
    input: vector[n];
    output: scalar;
    loss: mse;
    optimizer: gradient_descent;
    learning_rate: 0.01;
    max_iterations: 1000;
    tolerance: 0.0001;
}

train linear_reg with dataset "housing_data.csv" {
    epochs: 100;
    batch_size: 32;
};

predict linear_reg on input [1500.0, 3.0, 2.0, 1.5];
```

### 2. Decision Tree

```btl
ml model tree_classifier {
    type: decision_tree;
    input: vector[n];
    output: class;
    max_depth: 10;
    min_samples_split: 2;
    criterion: gini;
}

train tree_classifier with dataset "iris.csv" {
    epochs: 1;
};

predict tree_classifier on input [5.1, 3.5, 1.4, 0.2];
```

### 3. Support Vector Machine (SVM)

```btl
ml model svm_classifier {
    type: svm;
    input: vector[n];
    output: class;
    kernel: rbf;
    C: 1.0;
    gamma: 0.1;
    max_iterations: 1000;
}

train svm_classifier with dataset "binary_classification.csv";

predict svm_classifier on input [2.5, 3.7, 1.2];
```

### 4. K-means Clustering

```btl
ml model kmeans_cluster {
    type: kmeans;
    input: vector[n];
    output: cluster;
    n_clusters: 3;
    max_iterations: 100;
    tolerance: 0.0001;
}

train kmeans_cluster with dataset "customer_data.csv";

predict kmeans_cluster on input [25.0, 50000.0, 3.5];
```

### 5. Q-learning

```btl
ml model qlearning_agent {
    type: qlearning;
    state_space: discrete[100];
    action_space: discrete[4];
    learning_rate: 0.1;
    discount_factor: 0.99;
    epsilon: 1.0;
    epsilon_decay: 0.995;
    epsilon_min: 0.01;
}

train qlearning_agent with dataset "environment_config.json" {
    epochs: 1000;
    max_steps_per_episode: 200;
};

predict qlearning_agent on input [42.0];  // state = 42
```

## Complete Example

```btl
// Define multiple models
ml model price_predictor {
    type: linear_regression;
    input: vector[5];
    output: scalar;
    learning_rate: 0.01;
}

ml model customer_segmentation {
    type: kmeans;
    input: vector[10];
    output: cluster;
    n_clusters: 4;
}

ml model game_agent {
    type: qlearning;
    state_space: discrete[64];
    action_space: discrete[4];
    learning_rate: 0.1;
    discount_factor: 0.95;
}

// Train models
train price_predictor with dataset "prices.csv" {
    epochs: 500;
    batch_size: 64;
};

train customer_segmentation with dataset "customers.csv" {
    epochs: 100;
};

train game_agent with dataset "game_env.json" {
    epochs: 2000;
};

// Make predictions
predict price_predictor on input [1200.0, 3.0, 2.0, 1800.0, 2015.0];
predict customer_segmentation on input [35.0, 75000.0, 5.0, 2.0, 1.0, 0.0, 3.5, 12.0, 1.0, 0.0];
predict game_agent on input [15.0];
```

## Language Features

### Data Types

- `vector[n]`: N-dimensional feature vector
- `scalar`: Single numeric value
- `class`: Discrete class label
- `cluster`: Cluster assignment
- `discrete[n]`: Discrete space with N elements

### Loss Functions

- `mse`: Mean squared error (regression)
- `gini`: Gini impurity (classification)
- `entropy`: Cross-entropy (classification)

### Optimizers

- `gradient_descent`: Batch gradient descent
- `sgd`: Stochastic gradient descent
- `adam`: Adaptive moment estimation

### Kernels (SVM)

- `linear`: Linear kernel
- `rbf`: Radial basis function kernel
- `poly`: Polynomial kernel

### Parameters

Common parameters across algorithms:
- `learning_rate`: Step size for optimization
- `max_iterations`: Maximum training iterations
- `tolerance`: Convergence threshold
- `epochs`: Number of training epochs
- `batch_size`: Mini-batch size

## Compiler Pipeline

1. **Lexical Analysis**: Tokenize DSL source code
2. **Syntax Analysis**: Parse tokens into Abstract Syntax Tree (AST)
3. **Semantic Analysis**: Validate model declarations and parameters
4. **Code Generation**: Generate VM bytecode
5. **Execution**: Run bytecode in ML VM

## API Usage

### Compile from Source

```c
#include "ml_dsl_compiler.h"

const char* source = "ml model test { type: linear_regression; }";
MLVMContext* vm_context = nullptr;

if (dsl_compile_source(source, &vm_context)) {
    // Use vm_context for training/prediction
    ml_vm_context_destroy(vm_context);
}
```

### Compile from File

```c
#include "ml_dsl_compiler.h"

MLVMContext* vm_context = nullptr;

if (dsl_compile_file("models.btl", &vm_context)) {
    // Use vm_context
    ml_vm_context_destroy(vm_context);
}
```

## Error Handling

The DSL compiler provides detailed error messages with line and column numbers:

```
Lexer error at line 3, column 15: Unexpected character: '@'
Parser error at line 5, column 10: Expected ':' after parameter name
Compiler error: Model 'my_model' already declared
```

## Implementation Details

### Lexer (ml_dsl_lexer.c)
- Tokenizes source code into keywords, identifiers, numbers, strings
- Handles comments (// and /* */)
- Tracks line and column numbers for error reporting

### Parser (ml_dsl_parser.c)
- Recursive descent parser
- Builds Abstract Syntax Tree (AST)
- Error recovery with synchronization

### AST (ml_dsl_ast.c)
- Tree representation of program structure
- Node types: Program, ModelDecl, TrainStmt, PredictStmt
- Parameter lists with type-safe values

### Compiler (ml_dsl_compiler.c)
- Semantic analysis and validation
- Symbol table for model tracking
- Code generation to VM bytecode
- Integration with existing ML primitives

## Future Extensions

- Type inference for input/output dimensions
- Model composition and pipelines
- Hyperparameter tuning directives
- Cross-validation syntax
- Model serialization/deserialization
- Ensemble methods
- Transfer learning support

## Examples Directory

See `examples/ml_models.btl` for complete working examples of all supported algorithms.

## Testing

Run DSL tests:
```bash
./ml_dsl_test
```

Tests cover:
- Lexer tokenization
- Parser AST construction
- Compiler model creation
- End-to-end compilation
- Error handling

## Performance

- Lexer: O(n) where n is source length
- Parser: O(n) where n is number of tokens
- Compiler: O(m) where m is number of models
- Memory: Minimal overhead, models stored efficiently

## References

- Domain-Specific Languages (Fowler, 2010)
- Compiler Design Principles
- BDI ML Primitives Documentation
