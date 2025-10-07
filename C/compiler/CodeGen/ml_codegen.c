
/**
 * @file ml_codegen.c
 * @brief ML Primitives Code Generation Implementation
 */

#include "ml_codegen.h"
#include <stdlib.h>
#include <string.h>

// Serialize Linear Regression Model
SerializedModel* serialize_linear_regression(const LinearRegressionModel* model) {
    if (!model || !model->fitted) {
        return nullptr;
    }
    
    SerializedModel* serialized = malloc(sizeof(SerializedModel));
    if (!serialized) return nullptr;
    
    // Calculate size: n_features + weights + bias + metadata
    size_t data_size = sizeof(size_t) + // n_features
                      model->n_features * sizeof(double) + // weights
                      sizeof(double) + // bias
                      sizeof(double) + // learning_rate
                      sizeof(size_t) + // max_iterations
                      sizeof(double);  // tolerance
    
    serialized->model_type = 0; // Linear regression
    serialized->data_size = data_size;
    serialized->data = malloc(data_size);
    
    if (!serialized->data) {
        free(serialized);
        return nullptr;
    }
    
    // Pack data
    uint8_t* ptr = serialized->data;
    
    memcpy(ptr, &model->n_features, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, model->weights, model->n_features * sizeof(double));
    ptr += model->n_features * sizeof(double);
    
    memcpy(ptr, &model->bias, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(ptr, &model->learning_rate, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(ptr, &model->max_iterations, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->tolerance, sizeof(double));
    
    return serialized;
}

// Deserialize Linear Regression Model
LinearRegressionModel* deserialize_linear_regression(const SerializedModel* data) {
    if (!data || data->model_type != 0) {
        return nullptr;
    }
    
    const uint8_t* ptr = data->data;
    
    size_t n_features;
    memcpy(&n_features, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    LinearRegressionConfig config = linear_regression_default_config();
    LinearRegressionModel* model = linear_regression_create(n_features, config);
    if (!model) return nullptr;
    
    memcpy(model->weights, ptr, n_features * sizeof(double));
    ptr += n_features * sizeof(double);
    
    memcpy(&model->bias, ptr, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(&model->learning_rate, ptr, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(&model->max_iterations, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(&model->tolerance, ptr, sizeof(double));
    
    model->fitted = true;
    
    return model;
}

// Emit Linear Regression Model
bool emit_linear_regression_model(Chunk* chunk, const LinearRegressionModel* model) {
    if (!chunk || !model) {
        return false;
    }
    
    SerializedModel* serialized = serialize_linear_regression(model);
    if (!serialized) {
        return false;
    }
    
    // Add serialized model as constant
    // In a real implementation, this would be stored in a model pool
    // For now, we just emit the opcode
    chunk_write(chunk, OP_ML_LINEAR_REG_PREDICT, 0);
    
    serialized_model_destroy(serialized);
    return true;
}

// Emit Linear Regression Predict
bool emit_linear_regression_predict(Chunk* chunk, size_t model_constant_idx) {
    if (!chunk) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_LINEAR_REG_PREDICT, 0);
    chunk_write(chunk, (uint8_t)model_constant_idx, 0);
    
    return true;
}

// Emit Linear Regression Train
bool emit_linear_regression_train(Chunk* chunk, size_t data_idx, size_t labels_idx) {
    if (!chunk) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_LINEAR_REG_TRAIN, 0);
    chunk_write(chunk, (uint8_t)data_idx, 0);
    chunk_write(chunk, (uint8_t)labels_idx, 0);
    
    return true;
}

// Serialize Decision Tree (simplified - full implementation would traverse tree)
SerializedModel* serialize_decision_tree(const DecisionTreeModel* model) {
    if (!model || !model->fitted) {
        return nullptr;
    }
    
    SerializedModel* serialized = malloc(sizeof(SerializedModel));
    if (!serialized) return nullptr;
    
    // Simplified: just store metadata
    size_t data_size = sizeof(size_t) * 3 + sizeof(double);
    
    serialized->model_type = 1; // Decision tree
    serialized->data_size = data_size;
    serialized->data = malloc(data_size);
    
    if (!serialized->data) {
        free(serialized);
        return nullptr;
    }
    
    uint8_t* ptr = serialized->data;
    
    memcpy(ptr, &model->max_depth, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->min_samples_split, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->min_samples_leaf, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->min_impurity_decrease, sizeof(double));
    
    return serialized;
}

// Emit Decision Tree Model
bool emit_decision_tree_model(Chunk* chunk, const DecisionTreeModel* model) {
    if (!chunk || !model) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_TREE_PREDICT, 0);
    return true;
}

// Emit Decision Tree Predict
bool emit_decision_tree_predict(Chunk* chunk, size_t model_constant_idx) {
    if (!chunk) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_TREE_PREDICT, 0);
    chunk_write(chunk, (uint8_t)model_constant_idx, 0);
    
    return true;
}

// Emit Decision Tree Train
bool emit_decision_tree_train(Chunk* chunk, size_t data_idx, size_t labels_idx) {
    if (!chunk) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_TREE_TRAIN, 0);
    chunk_write(chunk, (uint8_t)data_idx, 0);
    chunk_write(chunk, (uint8_t)labels_idx, 0);
    
    return true;
}

// Serialize SVM Model
SerializedModel* serialize_svm(const SVMModel* model) {
    if (!model || !model->fitted) {
        return nullptr;
    }
    
    SerializedModel* serialized = malloc(sizeof(SerializedModel));
    if (!serialized) return nullptr;
    
    size_t data_size = sizeof(size_t) * 2 + // n_support_vectors, n_features
                      sizeof(double) * 3 + // bias, gamma, C
                      sizeof(int) + // kernel_type
                      model->n_support_vectors * model->n_features * sizeof(double) + // support_vectors
                      model->n_support_vectors * sizeof(double) * 2; // alphas, support_labels
    
    serialized->model_type = 2; // SVM
    serialized->data_size = data_size;
    serialized->data = malloc(data_size);
    
    if (!serialized->data) {
        free(serialized);
        return nullptr;
    }
    
    uint8_t* ptr = serialized->data;
    
    memcpy(ptr, &model->n_support_vectors, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->n_features, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->bias, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(ptr, &model->gamma, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(ptr, &model->C, sizeof(double));
    ptr += sizeof(double);
    
    int kernel_type = (int)model->kernel_type;
    memcpy(ptr, &kernel_type, sizeof(int));
    ptr += sizeof(int);
    
    memcpy(ptr, model->support_vectors, 
           model->n_support_vectors * model->n_features * sizeof(double));
    ptr += model->n_support_vectors * model->n_features * sizeof(double);
    
    memcpy(ptr, model->alphas, model->n_support_vectors * sizeof(double));
    ptr += model->n_support_vectors * sizeof(double);
    
    memcpy(ptr, model->support_labels, model->n_support_vectors * sizeof(double));
    
    return serialized;
}

// Emit SVM Model
bool emit_svm_model(Chunk* chunk, const SVMModel* model) {
    if (!chunk || !model) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_SVM_PREDICT, 0);
    return true;
}

// Emit SVM Predict
bool emit_svm_predict(Chunk* chunk, size_t model_constant_idx) {
    if (!chunk) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_SVM_PREDICT, 0);
    chunk_write(chunk, (uint8_t)model_constant_idx, 0);
    
    return true;
}

// Emit SVM Train
bool emit_svm_train(Chunk* chunk, size_t data_idx, size_t labels_idx) {
    if (!chunk) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_SVM_TRAIN, 0);
    chunk_write(chunk, (uint8_t)data_idx, 0);
    chunk_write(chunk, (uint8_t)labels_idx, 0);
    
    return true;
}

// Serialize K-means Model
SerializedModel* serialize_kmeans(const KMeansModel* model) {
    if (!model || !model->fitted) {
        return nullptr;
    }
    
    SerializedModel* serialized = malloc(sizeof(SerializedModel));
    if (!serialized) return nullptr;
    
    size_t data_size = sizeof(size_t) * 3 + // n_clusters, n_features, n_iterations
                      sizeof(double) * 2 + // tolerance, inertia
                      model->n_clusters * model->n_features * sizeof(double); // centroids
    
    serialized->model_type = 3; // K-means
    serialized->data_size = data_size;
    serialized->data = malloc(data_size);
    
    if (!serialized->data) {
        free(serialized);
        return nullptr;
    }
    
    uint8_t* ptr = serialized->data;
    
    memcpy(ptr, &model->n_clusters, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->n_features, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->n_iterations, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->tolerance, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(ptr, &model->inertia, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(ptr, model->centroids, model->n_clusters * model->n_features * sizeof(double));
    
    return serialized;
}

// Emit K-means Model
bool emit_kmeans_model(Chunk* chunk, const KMeansModel* model) {
    if (!chunk || !model) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_KMEANS_PREDICT, 0);
    return true;
}

// Emit K-means Predict
bool emit_kmeans_predict(Chunk* chunk, size_t model_constant_idx) {
    if (!chunk) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_KMEANS_PREDICT, 0);
    chunk_write(chunk, (uint8_t)model_constant_idx, 0);
    
    return true;
}

// Emit K-means Train
bool emit_kmeans_train(Chunk* chunk, size_t data_idx) {
    if (!chunk) {
        return false;
    }
    
    chunk_write(chunk, OP_ML_KMEANS_TRAIN, 0);
    chunk_write(chunk, (uint8_t)data_idx, 0);
    
    return true;
}

// Destroy serialized model
void serialized_model_destroy(SerializedModel* model) {
    if (model) {
        free(model->data);
        free(model);
    }
}

// Placeholder deserializers (would need full implementation)
DecisionTreeModel* deserialize_decision_tree(const SerializedModel* data) {
    if (!data || data->model_type != 1) {
        return nullptr;
    }
    // Simplified implementation
    DecisionTreeConfig config = decision_tree_default_config();
    return decision_tree_create(config);
}

SVMModel* deserialize_svm(const SerializedModel* data) {
    if (!data || data->model_type != 2) {
        return nullptr;
    }
    
    const uint8_t* ptr = data->data;
    
    size_t n_support_vectors, n_features;
    memcpy(&n_support_vectors, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(&n_features, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    SVMConfig config = svm_default_config();
    SVMModel* model = svm_create(n_features, config);
    if (!model) return nullptr;
    
    memcpy(&model->bias, ptr, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(&model->gamma, ptr, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(&model->C, ptr, sizeof(double));
    ptr += sizeof(double);
    
    int kernel_type;
    memcpy(&kernel_type, ptr, sizeof(int));
    model->kernel_type = (SVMKernelType)kernel_type;
    ptr += sizeof(int);
    
    model->n_support_vectors = n_support_vectors;
    model->support_vectors = malloc(n_support_vectors * n_features * sizeof(double));
    model->alphas = malloc(n_support_vectors * sizeof(double));
    model->support_labels = malloc(n_support_vectors * sizeof(double));
    
    if (!model->support_vectors || !model->alphas || !model->support_labels) {
        svm_destroy(model);
        return nullptr;
    }
    
    memcpy(model->support_vectors, ptr, n_support_vectors * n_features * sizeof(double));
    ptr += n_support_vectors * n_features * sizeof(double);
    
    memcpy(model->alphas, ptr, n_support_vectors * sizeof(double));
    ptr += n_support_vectors * sizeof(double);
    
    memcpy(model->support_labels, ptr, n_support_vectors * sizeof(double));
    
    model->fitted = true;
    
    return model;
}

KMeansModel* deserialize_kmeans(const SerializedModel* data) {
    if (!data || data->model_type != 3) {
        return nullptr;
    }
    
    const uint8_t* ptr = data->data;
    
    size_t n_clusters, n_features;
    memcpy(&n_clusters, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(&n_features, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    KMeansConfig config = kmeans_default_config(n_clusters);
    KMeansModel* model = kmeans_create(n_features, config);
    if (!model) return nullptr;
    
    memcpy(&model->n_iterations, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(&model->tolerance, ptr, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(&model->inertia, ptr, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(model->centroids, ptr, n_clusters * n_features * sizeof(double));
    
    model->fitted = true;
    
    return model;
}
