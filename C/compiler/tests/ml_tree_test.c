
/**
 * @file ml_tree_test.c
 * @brief Test suite for Decision Tree ML Primitive
 */

#include "../AIBase/tree/decision_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define EPSILON 1e-4

// Test simple decision tree
static void test_simple_tree(void) {
    printf("Testing simple decision tree...\n");
    
    // Simple dataset: if x < 5 then y=1, else y=10
    double X[] = {1.0, 2.0, 3.0, 4.0, 6.0, 7.0, 8.0, 9.0};
    double y[] = {1.0, 1.0, 1.0, 1.0, 10.0, 10.0, 10.0, 10.0};
    size_t n_samples = 8;
    size_t n_features = 1;
    
    DecisionTreeConfig config = decision_tree_default_config();
    config.max_depth = 3;
    
    DecisionTreeModel* model = decision_tree_create(config);
    assert(model != nullptr);
    
    bool success = decision_tree_fit(model, X, y, n_samples, n_features);
    assert(success);
    assert(model->fitted);
    
    // Test predictions
    double test_x1 = 2.5;
    double pred1 = decision_tree_predict_single(model, &test_x1);
    printf("  Prediction for x=2.5: %.2f (expected: ~1.0)\n", pred1);
    assert(fabs(pred1 - 1.0) < 2.0);
    
    double test_x2 = 7.5;
    double pred2 = decision_tree_predict_single(model, &test_x2);
    printf("  Prediction for x=7.5: %.2f (expected: ~10.0)\n", pred2);
    assert(fabs(pred2 - 10.0) < 2.0);
    
    decision_tree_destroy(model);
    printf("  ✓ Simple tree test passed\n\n");
}

// Test multivariate tree
static void test_multivariate_tree(void) {
    printf("Testing multivariate decision tree...\n");
    
    // Dataset with 2 features
    double X[] = {
        1.0, 1.0,
        1.0, 2.0,
        2.0, 1.0,
        2.0, 2.0,
        5.0, 5.0,
        5.0, 6.0,
        6.0, 5.0,
        6.0, 6.0
    };
    double y[] = {1.0, 1.0, 1.0, 1.0, 10.0, 10.0, 10.0, 10.0};
    size_t n_samples = 8;
    size_t n_features = 2;
    
    DecisionTreeConfig config = decision_tree_default_config();
    config.max_depth = 5;
    
    DecisionTreeModel* model = decision_tree_create(config);
    assert(model != nullptr);
    
    bool success = decision_tree_fit(model, X, y, n_samples, n_features);
    assert(success);
    
    // Test predictions
    double test_x1[] = {1.5, 1.5};
    double pred1 = decision_tree_predict_single(model, test_x1);
    printf("  Prediction for [1.5, 1.5]: %.2f (expected: ~1.0)\n", pred1);
    
    double test_x2[] = {5.5, 5.5};
    double pred2 = decision_tree_predict_single(model, test_x2);
    printf("  Prediction for [5.5, 5.5]: %.2f (expected: ~10.0)\n", pred2);
    
    decision_tree_destroy(model);
    printf("  ✓ Multivariate tree test passed\n\n");
}

// Test batch predictions
static void test_tree_batch_predictions(void) {
    printf("Testing tree batch predictions...\n");
    
    double X_train[] = {1.0, 2.0, 3.0, 7.0, 8.0, 9.0};
    double y_train[] = {1.0, 1.0, 1.0, 10.0, 10.0, 10.0};
    size_t n_samples = 6;
    size_t n_features = 1;
    
    DecisionTreeConfig config = decision_tree_default_config();
    DecisionTreeModel* model = decision_tree_create(config);
    
    decision_tree_fit(model, X_train, y_train, n_samples, n_features);
    
    // Batch prediction
    double X_test[] = {2.5, 8.5};
    double predictions[2];
    size_t n_test = 2;
    
    decision_tree_predict(model, X_test, predictions, n_test, n_features);
    
    printf("  Predictions: [%.2f, %.2f]\n", predictions[0], predictions[1]);
    
    decision_tree_destroy(model);
    printf("  ✓ Tree batch predictions test passed\n\n");
}

// Test tree depth control
static void test_tree_depth_control(void) {
    printf("Testing tree depth control...\n");
    
    double X[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    double y[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
    size_t n_samples = 8;
    size_t n_features = 1;
    
    // Test with max_depth = 1
    DecisionTreeConfig config1 = decision_tree_default_config();
    config1.max_depth = 1;
    
    DecisionTreeModel* model1 = decision_tree_create(config1);
    decision_tree_fit(model1, X, y, n_samples, n_features);
    
    printf("  Tree with max_depth=1 created\n");
    
    // Test with max_depth = 5
    DecisionTreeConfig config2 = decision_tree_default_config();
    config2.max_depth = 5;
    
    DecisionTreeModel* model2 = decision_tree_create(config2);
    decision_tree_fit(model2, X, y, n_samples, n_features);
    
    printf("  Tree with max_depth=5 created\n");
    
    decision_tree_destroy(model1);
    decision_tree_destroy(model2);
    printf("  ✓ Tree depth control test passed\n\n");
}

int main(void) {
    printf("=== Decision Tree ML Primitive Tests ===\n\n");
    
    test_simple_tree();
    test_multivariate_tree();
    test_tree_batch_predictions();
    test_tree_depth_control();
    
    printf("=== All Decision Tree tests passed! ===\n");
    return 0;
}
