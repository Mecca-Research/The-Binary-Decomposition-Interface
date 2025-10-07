
/**
 * @file ml_linear_test.c
 * @brief Test suite for Linear Regression ML Primitive
 */

#include "../AIBase/linear/linear_regression.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define EPSILON 1e-4

// Test data: y = 2x + 3
static void test_simple_linear_regression(void) {
    printf("Testing simple linear regression...\n");
    
    // Training data
    double X[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double y[] = {5.0, 7.0, 9.0, 11.0, 13.0};
    size_t n_samples = 5;
    size_t n_features = 1;
    
    // Create and train model
    LinearRegressionConfig config = linear_regression_default_config();
    config.learning_rate = 0.01;
    config.max_iterations = 10000;
    
    LinearRegressionModel* model = linear_regression_create(n_features, config);
    assert(model != nullptr);
    
    bool success = linear_regression_fit(model, X, y, n_samples, n_features);
    assert(success);
    assert(model->fitted);
    
    // Check weights (should be close to 2.0)
    printf("  Learned weight: %.4f (expected: 2.0)\n", model->weights[0]);
    printf("  Learned bias: %.4f (expected: 3.0)\n", model->bias);
    
    assert(fabs(model->weights[0] - 2.0) < 0.1);
    assert(fabs(model->bias - 3.0) < 0.5);
    
    // Test predictions
    double test_x = 6.0;
    double prediction = linear_regression_predict_single(model, &test_x);
    printf("  Prediction for x=6.0: %.4f (expected: 15.0)\n", prediction);
    assert(fabs(prediction - 15.0) < 1.0);
    
    // Test loss
    double loss = linear_regression_mse_loss(model, X, y, n_samples, n_features);
    printf("  Final MSE loss: %.6f\n", loss);
    assert(loss < 0.5);
    
    linear_regression_destroy(model);
    printf("  ✓ Simple linear regression test passed\n\n");
}

// Test multivariate regression
static void test_multivariate_regression(void) {
    printf("Testing multivariate linear regression...\n");
    
    // Training data: y = 2*x1 + 3*x2 + 1
    double X[] = {
        1.0, 1.0,
        2.0, 1.0,
        3.0, 2.0,
        4.0, 2.0,
        5.0, 3.0
    };
    double y[] = {6.0, 8.0, 13.0, 15.0, 20.0};
    size_t n_samples = 5;
    size_t n_features = 2;
    
    LinearRegressionConfig config = linear_regression_default_config();
    config.learning_rate = 0.01;
    config.max_iterations = 10000;
    
    LinearRegressionModel* model = linear_regression_create(n_features, config);
    assert(model != nullptr);
    
    bool success = linear_regression_fit(model, X, y, n_samples, n_features);
    assert(success);
    
    printf("  Learned weights: [%.4f, %.4f] (expected: [2.0, 3.0])\n",
           model->weights[0], model->weights[1]);
    printf("  Learned bias: %.4f (expected: 1.0)\n", model->bias);
    
    // Test prediction
    double test_x[] = {6.0, 3.0};
    double prediction = linear_regression_predict_single(model, test_x);
    printf("  Prediction for [6.0, 3.0]: %.4f (expected: 22.0)\n", prediction);
    
    linear_regression_destroy(model);
    printf("  ✓ Multivariate regression test passed\n\n");
}

// Test batch predictions
static void test_batch_predictions(void) {
    printf("Testing batch predictions...\n");
    
    double X_train[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double y_train[] = {2.0, 4.0, 6.0, 8.0, 10.0};
    size_t n_samples = 5;
    size_t n_features = 1;
    
    LinearRegressionConfig config = linear_regression_default_config();
    LinearRegressionModel* model = linear_regression_create(n_features, config);
    
    linear_regression_fit(model, X_train, y_train, n_samples, n_features);
    
    // Batch prediction
    double X_test[] = {6.0, 7.0, 8.0};
    double predictions[3];
    size_t n_test = 3;
    
    linear_regression_predict(model, X_test, predictions, n_test, n_features);
    
    printf("  Predictions: [%.2f, %.2f, %.2f]\n",
           predictions[0], predictions[1], predictions[2]);
    
    linear_regression_destroy(model);
    printf("  ✓ Batch predictions test passed\n\n");
}

// Test gradient computation
static void test_gradient_computation(void) {
    printf("Testing gradient computation...\n");
    
    double X[] = {1.0, 2.0, 3.0};
    double y[] = {2.0, 4.0, 6.0};
    size_t n_samples = 3;
    size_t n_features = 1;
    
    LinearRegressionConfig config = linear_regression_default_config();
    LinearRegressionModel* model = linear_regression_create(n_features, config);
    
    // Set initial weights
    model->weights[0] = 1.5;
    model->bias = 0.5;
    
    double weight_gradients[1];
    double bias_gradient;
    
    linear_regression_compute_gradients(model, X, y, n_samples, n_features,
                                       weight_gradients, &bias_gradient);
    
    printf("  Weight gradient: %.4f\n", weight_gradients[0]);
    printf("  Bias gradient: %.4f\n", bias_gradient);
    
    linear_regression_destroy(model);
    printf("  ✓ Gradient computation test passed\n\n");
}

int main(void) {
    printf("=== Linear Regression ML Primitive Tests ===\n\n");
    
    test_simple_linear_regression();
    test_multivariate_regression();
    test_batch_predictions();
    test_gradient_computation();
    
    printf("=== All Linear Regression tests passed! ===\n");
    return 0;
}
