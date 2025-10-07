
/**
 * @file kmeans.c
 * @brief K-means Clustering ML Primitive Implementation
 */

#include "kmeans.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

// Default configuration
KMeansConfig kmeans_default_config(size_t n_clusters) {
    return (KMeansConfig){
        .n_clusters = n_clusters,
        .max_iterations = 300,
        .tolerance = 1e-4,
        .random_seed = 42
    };
}

// Create model
KMeansModel* kmeans_create(size_t n_features, KMeansConfig config) {
    if (n_features == 0 || config.n_clusters == 0) {
        return nullptr;
    }
    
    KMeansModel* model = malloc(sizeof(KMeansModel));
    if (!model) return nullptr;
    
    model->centroids = calloc(config.n_clusters * n_features, sizeof(double));
    if (!model->centroids) {
        free(model);
        return nullptr;
    }
    
    model->labels = nullptr;
    model->n_clusters = config.n_clusters;
    model->n_features = n_features;
    model->max_iterations = config.max_iterations;
    model->tolerance = config.tolerance;
    model->n_iterations = 0;
    model->inertia = 0.0;
    model->fitted = false;
    
    return model;
}

// Destroy model
void kmeans_destroy(KMeansModel* model) {
    if (!model) return;
    
    free(model->centroids);
    free(model->labels);
    free(model);
}

// Euclidean distance
double kmeans_euclidean_distance(const double* x1, const double* x2, size_t n_features) {
    double distance = 0.0;
    for (size_t i = 0; i < n_features; i++) {
        double diff = x1[i] - x2[i];
        distance += diff * diff;
    }
    return sqrt(distance);
}

// Initialize centroids using k-means++
void kmeans_initialize_centroids(KMeansModel* model,
                                const double* X,
                                size_t n_samples, size_t n_features,
                                unsigned int seed) {
    if (!model || !X || n_samples < model->n_clusters) {
        return;
    }
    
    // Simple random initialization (k-means++ would be better but more complex)
    srand(seed);
    
    // Select first centroid randomly
    size_t first_idx = rand() % n_samples;
    memcpy(model->centroids, &X[first_idx * n_features], n_features * sizeof(double));
    
    // Select remaining centroids
    for (size_t k = 1; k < model->n_clusters; k++) {
        double* distances = malloc(n_samples * sizeof(double));
        if (!distances) return;
        
        // Compute distance to nearest centroid for each point
        for (size_t i = 0; i < n_samples; i++) {
            double min_dist = DBL_MAX;
            for (size_t j = 0; j < k; j++) {
                double dist = kmeans_euclidean_distance(
                    &X[i * n_features],
                    &model->centroids[j * n_features],
                    n_features
                );
                if (dist < min_dist) {
                    min_dist = dist;
                }
            }
            distances[i] = min_dist * min_dist;
        }
        
        // Select next centroid with probability proportional to distance squared
        double sum = 0.0;
        for (size_t i = 0; i < n_samples; i++) {
            sum += distances[i];
        }
        
        double threshold = ((double)rand() / RAND_MAX) * sum;
        double cumsum = 0.0;
        size_t selected = 0;
        
        for (size_t i = 0; i < n_samples; i++) {
            cumsum += distances[i];
            if (cumsum >= threshold) {
                selected = i;
                break;
            }
        }
        
        memcpy(&model->centroids[k * n_features],
               &X[selected * n_features],
               n_features * sizeof(double));
        
        free(distances);
    }
}

// Find nearest centroid
static size_t find_nearest_centroid(const KMeansModel* model, const double* x) {
    size_t nearest = 0;
    double min_distance = DBL_MAX;
    
    for (size_t k = 0; k < model->n_clusters; k++) {
        double distance = kmeans_euclidean_distance(
            x,
            &model->centroids[k * model->n_features],
            model->n_features
        );
        
        if (distance < min_distance) {
            min_distance = distance;
            nearest = k;
        }
    }
    
    return nearest;
}

// Update centroids
void kmeans_update_centroids(KMeansModel* model,
                            const double* X,
                            size_t n_samples, size_t n_features) {
    if (!model || !X || !model->labels) {
        return;
    }
    
    // Zero out centroids
    memset(model->centroids, 0, model->n_clusters * n_features * sizeof(double));
    
    // Count points in each cluster
    size_t* counts = calloc(model->n_clusters, sizeof(size_t));
    if (!counts) return;
    
    // Sum points in each cluster
    for (size_t i = 0; i < n_samples; i++) {
        size_t cluster = model->labels[i];
        counts[cluster]++;
        
        for (size_t j = 0; j < n_features; j++) {
            model->centroids[cluster * n_features + j] += X[i * n_features + j];
        }
    }
    
    // Compute means
    for (size_t k = 0; k < model->n_clusters; k++) {
        if (counts[k] > 0) {
            for (size_t j = 0; j < n_features; j++) {
                model->centroids[k * n_features + j] /= counts[k];
            }
        }
    }
    
    free(counts);
}

// Compute inertia
double kmeans_compute_inertia(const KMeansModel* model,
                              const double* X,
                              size_t n_samples, size_t n_features) {
    if (!model || !X || !model->labels) {
        return INFINITY;
    }
    
    double inertia = 0.0;
    
    for (size_t i = 0; i < n_samples; i++) {
        size_t cluster = model->labels[i];
        double distance = kmeans_euclidean_distance(
            &X[i * n_features],
            &model->centroids[cluster * n_features],
            n_features
        );
        inertia += distance * distance;
    }
    
    return inertia;
}

// Fit model
bool kmeans_fit(KMeansModel* model,
               const double* X,
               size_t n_samples, size_t n_features) {
    if (!model || !X || n_features != model->n_features || 
        n_samples < model->n_clusters) {
        return false;
    }
    
    // Allocate labels array
    model->labels = malloc(n_samples * sizeof(size_t));
    if (!model->labels) {
        return false;
    }
    
    // Initialize centroids
    kmeans_initialize_centroids(model, X, n_samples, n_features, 42);
    
    // Lloyd's algorithm
    double prev_inertia = INFINITY;
    
    for (size_t iter = 0; iter < model->max_iterations; iter++) {
        // Assign points to nearest centroid
        bool changed = false;
        for (size_t i = 0; i < n_samples; i++) {
            size_t old_label = model->labels[i];
            size_t new_label = find_nearest_centroid(model, &X[i * n_features]);
            model->labels[i] = new_label;
            
            if (old_label != new_label) {
                changed = true;
            }
        }
        
        // Update centroids
        kmeans_update_centroids(model, X, n_samples, n_features);
        
        // Compute inertia
        double current_inertia = kmeans_compute_inertia(model, X, n_samples, n_features);
        
        // Check convergence
        if (fabs(prev_inertia - current_inertia) < model->tolerance) {
            model->n_iterations = iter + 1;
            model->inertia = current_inertia;
            model->fitted = true;
            return true;
        }
        
        prev_inertia = current_inertia;
        
        if (!changed) {
            model->n_iterations = iter + 1;
            model->inertia = current_inertia;
            model->fitted = true;
            return true;
        }
    }
    
    model->n_iterations = model->max_iterations;
    model->inertia = prev_inertia;
    model->fitted = true;
    return true;
}

// Predict single sample
size_t kmeans_predict_single(const KMeansModel* model, const double* x) {
    if (!model || !x || !model->fitted) {
        return 0;
    }
    
    return find_nearest_centroid(model, x);
}

// Predict multiple samples
void kmeans_predict(const KMeansModel* model,
                   const double* X, size_t* predictions,
                   size_t n_samples, size_t n_features) {
    if (!model || !X || !predictions || n_features != model->n_features) {
        return;
    }
    
    for (size_t i = 0; i < n_samples; i++) {
        predictions[i] = kmeans_predict_single(model, &X[i * n_features]);
    }
}
