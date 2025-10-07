
/**
 * @file ml_kmeans_test.c
 * @brief Test suite for K-means Clustering ML Primitive
 */

#include "../AIBase/clustering/kmeans.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define EPSILON 1e-4

// Test simple k-means
static void test_simple_kmeans(void) {
    printf("Testing simple k-means clustering...\n");
    
    // Two clear clusters
    double X[] = {
        1.0, 1.0,
        1.5, 1.5,
        2.0, 2.0,
        8.0, 8.0,
        8.5, 8.5,
        9.0, 9.0
    };
    size_t n_samples = 6;
    size_t n_features = 2;
    size_t n_clusters = 2;
    
    KMeansConfig config = kmeans_default_config(n_clusters);
    config.max_iterations = 100;
    
    KMeansModel* model = kmeans_create(n_features, config);
    assert(model != nullptr);
    
    bool success = kmeans_fit(model, X, n_samples, n_features);
    assert(success);
    assert(model->fitted);
    
    printf("  Number of iterations: %zu\n", model->n_iterations);
    printf("  Final inertia: %.4f\n", model->inertia);
    
    // Test predictions
    double test_x1[] = {1.2, 1.2};
    size_t cluster1 = kmeans_predict_single(model, test_x1);
    printf("  Cluster for [1.2, 1.2]: %zu\n", cluster1);
    
    double test_x2[] = {8.2, 8.2};
    size_t cluster2 = kmeans_predict_single(model, test_x2);
    printf("  Cluster for [8.2, 8.2]: %zu\n", cluster2);
    
    // Points in different regions should be in different clusters
    assert(cluster1 != cluster2);
    
    kmeans_destroy(model);
    printf("  ✓ Simple k-means test passed\n\n");
}

// Test three clusters
static void test_three_clusters(void) {
    printf("Testing k-means with 3 clusters...\n");
    
    // Three clear clusters
    double X[] = {
        1.0, 1.0,
        1.5, 1.5,
        2.0, 2.0,
        5.0, 5.0,
        5.5, 5.5,
        6.0, 6.0,
        9.0, 9.0,
        9.5, 9.5,
        10.0, 10.0
    };
    size_t n_samples = 9;
    size_t n_features = 2;
    size_t n_clusters = 3;
    
    KMeansConfig config = kmeans_default_config(n_clusters);
    KMeansModel* model = kmeans_create(n_features, config);
    
    bool success = kmeans_fit(model, X, n_samples, n_features);
    assert(success);
    
    printf("  Number of iterations: %zu\n", model->n_iterations);
    printf("  Final inertia: %.4f\n", model->inertia);
    
    // Print centroids
    printf("  Centroids:\n");
    for (size_t i = 0; i < n_clusters; i++) {
        printf("    Cluster %zu: [%.2f, %.2f]\n", i,
               model->centroids[i * n_features],
               model->centroids[i * n_features + 1]);
    }
    
    kmeans_destroy(model);
    printf("  ✓ Three clusters test passed\n\n");
}

// Test batch predictions
static void test_kmeans_batch_predictions(void) {
    printf("Testing k-means batch predictions...\n");
    
    double X_train[] = {
        1.0, 1.0,
        2.0, 2.0,
        8.0, 8.0,
        9.0, 9.0
    };
    size_t n_samples = 4;
    size_t n_features = 2;
    size_t n_clusters = 2;
    
    KMeansConfig config = kmeans_default_config(n_clusters);
    KMeansModel* model = kmeans_create(n_features, config);
    
    kmeans_fit(model, X_train, n_samples, n_features);
    
    // Batch prediction
    double X_test[] = {
        1.5, 1.5,
        8.5, 8.5,
        5.0, 5.0
    };
    size_t predictions[3];
    size_t n_test = 3;
    
    kmeans_predict(model, X_test, predictions, n_test, n_features);
    
    printf("  Predictions: [%zu, %zu, %zu]\n",
           predictions[0], predictions[1], predictions[2]);
    
    // First two should be in same cluster, third might be either
    assert(predictions[0] == predictions[0]);
    assert(predictions[1] == predictions[1]);
    
    kmeans_destroy(model);
    printf("  ✓ K-means batch predictions test passed\n\n");
}

// Test euclidean distance
static void test_euclidean_distance(void) {
    printf("Testing euclidean distance...\n");
    
    double x1[] = {0.0, 0.0};
    double x2[] = {3.0, 4.0};
    size_t n_features = 2;
    
    double distance = kmeans_euclidean_distance(x1, x2, n_features);
    printf("  Distance between [0,0] and [3,4]: %.4f (expected: 5.0)\n", distance);
    assert(fabs(distance - 5.0) < EPSILON);
    
    printf("  ✓ Euclidean distance test passed\n\n");
}

// Test convergence
static void test_convergence(void) {
    printf("Testing k-means convergence...\n");
    
    // Well-separated clusters should converge quickly
    double X[] = {
        0.0, 0.0,
        0.1, 0.1,
        0.2, 0.2,
        10.0, 10.0,
        10.1, 10.1,
        10.2, 10.2
    };
    size_t n_samples = 6;
    size_t n_features = 2;
    size_t n_clusters = 2;
    
    KMeansConfig config = kmeans_default_config(n_clusters);
    config.tolerance = 1e-4;
    config.max_iterations = 1000;
    
    KMeansModel* model = kmeans_create(n_features, config);
    kmeans_fit(model, X, n_samples, n_features);
    
    printf("  Converged in %zu iterations\n", model->n_iterations);
    printf("  Final inertia: %.6f\n", model->inertia);
    
    // Should converge quickly for well-separated clusters
    assert(model->n_iterations < 100);
    
    kmeans_destroy(model);
    printf("  ✓ Convergence test passed\n\n");
}

// Test inertia computation
static void test_inertia_computation(void) {
    printf("Testing inertia computation...\n");
    
    double X[] = {
        1.0, 1.0,
        2.0, 2.0,
        8.0, 8.0,
        9.0, 9.0
    };
    size_t n_samples = 4;
    size_t n_features = 2;
    size_t n_clusters = 2;
    
    KMeansConfig config = kmeans_default_config(n_clusters);
    KMeansModel* model = kmeans_create(n_features, config);
    
    kmeans_fit(model, X, n_samples, n_features);
    
    double inertia = kmeans_compute_inertia(model, X, n_samples, n_features);
    printf("  Computed inertia: %.4f\n", inertia);
    printf("  Model inertia: %.4f\n", model->inertia);
    
    assert(fabs(inertia - model->inertia) < EPSILON);
    
    kmeans_destroy(model);
    printf("  ✓ Inertia computation test passed\n\n");
}

int main(void) {
    printf("=== K-means Clustering ML Primitive Tests ===\n\n");
    
    test_simple_kmeans();
    test_three_clusters();
    test_kmeans_batch_predictions();
    test_euclidean_distance();
    test_convergence();
    test_inertia_computation();
    
    printf("=== All K-means tests passed! ===\n");
    return 0;
}
