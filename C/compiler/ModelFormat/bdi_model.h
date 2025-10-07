
#ifndef BDI_MODEL_H
#define BDI_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

// .bdi-model format magic number
#define BDI_MODEL_MAGIC 0x4244494D  // "BDIM"
#define BDI_MODEL_VERSION 1

// Model types
typedef enum {
    MODEL_TYPE_LINEAR_REGRESSION,
    MODEL_TYPE_DECISION_TREE,
    MODEL_TYPE_SVM,
    MODEL_TYPE_KMEANS,
    MODEL_TYPE_QLEARNING,
    MODEL_TYPE_NEURAL_NETWORK,
    MODEL_TYPE_CUSTOM
} ModelType;

// Model metadata
typedef struct {
    char name[64];
    char description[256];
    ModelType type;
    uint32_t version;
    time_t created_at;
    time_t trained_at;
    uint64_t training_samples;
    uint64_t training_iterations;
    double training_accuracy;
    double validation_accuracy;
    char architecture[128];
} ModelMetadata;

// Model data container
typedef struct {
    ModelMetadata metadata;
    void *model_data;
    size_t model_data_size;
    uint32_t checksum;
} BDIModel;

// Create new model
BDIModel* bdi_model_create(const char *name, ModelType type);

// Free model
void bdi_model_free(BDIModel *model);

// Set model data
bool bdi_model_set_data(BDIModel *model, const void *data, size_t size);

// Get model data
const void* bdi_model_get_data(const BDIModel *model, size_t *size);

// Update metadata
void bdi_model_update_metadata(BDIModel *model, const ModelMetadata *metadata);

// Calculate checksum
uint32_t bdi_model_calculate_checksum(const BDIModel *model);

// Verify model integrity
bool bdi_model_verify(const BDIModel *model);

#endif // BDI_MODEL_H
