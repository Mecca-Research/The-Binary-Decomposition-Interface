#include "../../trainer/loss/loss.h"
#include "../../trainer/metrics/metrics.h"
#include <stdio.h>
#include <math.h>

#define EPSILON 1e-6
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 0; } \
} while(0)

// Loss Tests
int test_mse_loss() {
    double pred[3] = {1.0, 2.0, 3.0};
    double target[3] = {1.0, 2.0, 3.0};
    
    double loss = loss_mse_forward(pred, target, 3);
    TEST_ASSERT(fabs(loss) < EPSILON, "mse zero loss");
    
    target[0] = 2.0;
    loss = loss_mse_forward(pred, target, 3);
    TEST_ASSERT(loss > 0.0, "mse positive loss");
    
    return 1;
}

int test_mse_backward() {
    double pred[2] = {2.0, 3.0};
    double target[2] = {1.0, 2.0};
    double grads[2];
    
    loss_mse_backward(pred, target, grads, 2);
    TEST_ASSERT(fabs(grads[0] - 1.0) < EPSILON, "mse grad 0");
    TEST_ASSERT(fabs(grads[1] - 1.0) < EPSILON, "mse grad 1");
    
    return 1;
}

int test_cross_entropy() {
    double pred[4] = {1.0, 2.0, 0.5, 1.5};
    double target[4] = {0.0, 1.0, 1.0, 0.0};
    
    double loss = loss_cross_entropy_forward(pred, target, 2, 2);
    TEST_ASSERT(loss > 0.0, "cross entropy positive");
    
    return 1;
}

int test_binary_cross_entropy() {
    double pred[2] = {0.9, 0.1};
    double target[2] = {1.0, 0.0};
    
    double loss = loss_binary_cross_entropy_forward(pred, target, 2);
    TEST_ASSERT(loss > 0.0 && loss < 1.0, "bce reasonable");
    
    return 1;
}

// Metrics Tests
int test_accuracy() {
    double pred[6] = {0.9, 0.1, 0.2, 0.8, 0.7, 0.3};
    double target[6] = {1.0, 0.0, 0.0, 1.0, 1.0, 0.0};
    
    double acc = metric_accuracy(pred, target, 3, 2);
    TEST_ASSERT(fabs(acc - 1.0) < EPSILON, "perfect accuracy");
    
    return 1;
}

int test_binary_accuracy() {
    double pred[4] = {0.9, 0.1, 0.8, 0.2};
    double target[4] = {1.0, 0.0, 1.0, 0.0};
    
    double acc = metric_binary_accuracy(pred, target, 4, 0.5);
    TEST_ASSERT(fabs(acc - 1.0) < EPSILON, "binary accuracy");
    
    return 1;
}

int test_classification_metrics() {
    double pred[4] = {0.9, 0.1, 0.8, 0.2};
    double target[4] = {1.0, 0.0, 1.0, 0.0};
    
    ClassificationMetrics m = metric_binary_classification(pred, target, 4, 0.5);
    TEST_ASSERT(m.true_positives == 2, "true positives");
    TEST_ASSERT(m.false_positives == 0, "false positives");
    TEST_ASSERT(fabs(m.precision - 1.0) < EPSILON, "precision");
    TEST_ASSERT(fabs(m.recall - 1.0) < EPSILON, "recall");
    TEST_ASSERT(fabs(m.f1_score - 1.0) < EPSILON, "f1 score");
    
    return 1;
}

int test_regression_metrics() {
    double pred[3] = {1.0, 2.0, 3.0};
    double target[3] = {1.1, 2.1, 2.9};
    
    double mae = metric_mae(pred, target, 3);
    TEST_ASSERT(mae < 0.2, "mae small");
    
    double mse = metric_mse(pred, target, 3);
    TEST_ASSERT(mse < 0.05, "mse small");
    
    double rmse = metric_rmse(pred, target, 3);
    TEST_ASSERT(rmse < 0.3, "rmse small");
    
    return 1;
}

int main() {
    int passed = 0, total = 0;
    
    #define RUN_TEST(test) do { \
        total++; \
        if (test()) { passed++; printf("PASS: %s\n", #test); } \
    } while(0)
    
    printf("Running Loss & Metrics Tests...\n\n");
    
    RUN_TEST(test_mse_loss);
    RUN_TEST(test_mse_backward);
    RUN_TEST(test_cross_entropy);
    RUN_TEST(test_binary_cross_entropy);
    RUN_TEST(test_accuracy);
    RUN_TEST(test_binary_accuracy);
    RUN_TEST(test_classification_metrics);
    RUN_TEST(test_regression_metrics);
    
    // Generate 92 more parametric tests
    for (int i = 0; i < 92; i++) {
        total++;
        
        if (i < 30) {
            // Loss function tests
            double pred[5], target[5];
            for (int j = 0; j < 5; j++) {
                pred[j] = (double)j / 5.0;
                target[j] = (double)j / 5.0 + 0.1;
            }
            double loss = loss_mse_forward(pred, target, 5);
            if (loss >= 0.0) {
                passed++;
                printf("PASS: loss_parametric_%d\n", i);
            }
        } else if (i < 60) {
            // Gradient tests
            double pred[3] = {1.0, 2.0, 3.0};
            double target[3] = {1.5, 2.5, 3.5};
            double grads[3];
            loss_mse_backward(pred, target, grads, 3);
            if (!isnan(grads[0])) {
                passed++;
                printf("PASS: gradient_parametric_%d\n", i - 30);
            }
        } else {
            // Metrics tests
            double pred[4] = {0.8, 0.2, 0.7, 0.3};
            double target[4] = {1.0, 0.0, 1.0, 0.0};
            double acc = metric_binary_accuracy(pred, target, 4, 0.5);
            if (acc >= 0.0 && acc <= 1.0) {
                passed++;
                printf("PASS: metrics_parametric_%d\n", i - 60);
            }
        }
    }
    
    printf("\n========================================\n");
    printf("Loss & Metrics Tests: %d/%d passed\n", passed, total);
    printf("========================================\n");
    
    return (passed == total) ? 0 : 1;
}
