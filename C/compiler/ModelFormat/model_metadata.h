
#ifndef BDI_MODEL_METADATA_H
#define BDI_MODEL_METADATA_H

#include "bdi_model.h"
#include <stdbool.h>

// Model registry entry
typedef struct {
    char name[64];
    char filename[256];
    ModelType type;
    time_t last_modified;
    size_t file_size;
    double accuracy;
} ModelRegistryEntry;

// Model registry
typedef struct {
    ModelRegistryEntry *entries;
    size_t entry_count;
    size_t entry_capacity;
} ModelRegistry;

// Create model registry
ModelRegistry* model_registry_create(void);

// Free model registry
void model_registry_free(ModelRegistry *registry);

// Add model to registry
bool model_registry_add(ModelRegistry *registry, const char *name, const char *filename, const BDIModel *model);

// Remove model from registry
bool model_registry_remove(ModelRegistry *registry, const char *name);

// Find model in registry
const ModelRegistryEntry* model_registry_find(const ModelRegistry *registry, const char *name);

// List all models
const ModelRegistryEntry* model_registry_list(const ModelRegistry *registry, size_t *count);

// Save registry to file
bool model_registry_save(const ModelRegistry *registry, const char *filename);

// Load registry from file
ModelRegistry* model_registry_load(const char *filename);

// Print registry
void model_registry_print(const ModelRegistry *registry);

#endif // BDI_MODEL_METADATA_H
