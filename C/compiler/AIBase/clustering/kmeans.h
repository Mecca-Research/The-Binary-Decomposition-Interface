
/**
 * @file kmeans.h
 * @brief K-means Clustering ML Primitive
 * @details Compiler-native k-means implementation with Lloyd's algorithm
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides ML primitives as first-class compiler features.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */
#ifndef BDI_KMEANS_H
#define BDI_KMEANS_H

#include "../../../c23_compat.h"
#include <stddef.h>
#include <stdbool.h>

// K-means model structure
typedef struct {
    double* centroids;         // Cluster centroids (n_clusters * n_features)
    size_t* labels;           // Cluster assignments for training data
    size_t n_clusters;
    size_t n_features;
    size_t max_iterations;
    double tolerance;
    size_t n_iterations;      // Actual iterations performed
    double inertia;           // Sum of squared distances to centroids
    bool fitted;
} KMeansModel;

// Training configuration
typedef struct {
    size_t n_clusters;
    size_t max_iterations;
    double tolerance;
    unsigned int random_seed;
} KMeansConfig;

// Model lifecycle
KMeansModel* kmeans_create(size_t n_features, KMeansConfig config);
void kmeans_destroy(KMeansModel* model);
KMeansConfig kmeans_default_config(size_t n_clusters);

// Training
bool kmeans_fit(KMeansModel* model,
               const double* X,
               size_t n_samples, size_t n_features);

// Prediction
size_t kmeans_predict_single(const KMeansModel* model, const double* x);
void kmeans_predict(const KMeansModel* model,
                   const double* X, size_t* predictions,
                   size_t n_samples, size_t n_features);

// Distance computation
double kmeans_euclidean_distance(const double* x1, const double* x2, size_t n_features);
double kmeans_compute_inertia(const KMeansModel* model,
                              const double* X,
                              size_t n_samples, size_t n_features);

// Centroid operations
void kmeans_initialize_centroids(KMeansModel* model,
                                const double* X,
                                size_t n_samples, size_t n_features,
                                unsigned int seed);
void kmeans_update_centroids(KMeansModel* model,
                            const double* X,
                            size_t n_samples, size_t n_features);

// Compile-time invariants
static_assert(sizeof(double) == 8, "K-means requires 64-bit doubles");
static_assert(sizeof(size_t) >= 4, "K-means requires at least 32-bit size_t");

#endif // BDI_KMEANS_H
