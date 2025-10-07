
#include "model_metadata.h"
#include "model_serializer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_REGISTRY_CAPACITY 50

ModelRegistry* model_registry_create(void) {
    ModelRegistry *registry = calloc(1, sizeof(ModelRegistry));
    if (!registry) {
        return NULL;
    }

    registry->entries = calloc(INITIAL_REGISTRY_CAPACITY, sizeof(ModelRegistryEntry));
    if (!registry->entries) {
        free(registry);
        return NULL;
    }

    registry->entry_capacity = INITIAL_REGISTRY_CAPACITY;
    registry->entry_count = 0;

    return registry;
}

void model_registry_free(ModelRegistry *registry) {
    if (!registry) return;
    free(registry->entries);
    free(registry);
}

bool model_registry_add(ModelRegistry *registry, const char *name, const char *filename, const BDIModel *model) {
    if (!registry || !name || !filename) {
        return false;
    }

    // Check if model already exists
    for (size_t i = 0; i < registry->entry_count; i++) {
        if (strcmp(registry->entries[i].name, name) == 0) {
            return false; // Already exists
        }
    }

    // Expand capacity if needed
    if (registry->entry_count >= registry->entry_capacity) {
        size_t new_capacity = registry->entry_capacity * 2;
        ModelRegistryEntry *new_entries = realloc(registry->entries,
                                                  new_capacity * sizeof(ModelRegistryEntry));
        if (!new_entries) {
            return false;
        }
        registry->entries = new_entries;
        registry->entry_capacity = new_capacity;
    }

    // Add new entry
    ModelRegistryEntry *entry = &registry->entries[registry->entry_count++];
    strncpy(entry->name, name, sizeof(entry->name) - 1);
    strncpy(entry->filename, filename, sizeof(entry->filename) - 1);
    entry->last_modified = time(NULL);
    entry->file_size = model_serializer_get_file_size(filename);

    if (model) {
        entry->type = model->metadata.type;
        entry->accuracy = model->metadata.validation_accuracy;
    }

    return true;
}

bool model_registry_remove(ModelRegistry *registry, const char *name) {
    if (!registry || !name) {
        return false;
    }

    for (size_t i = 0; i < registry->entry_count; i++) {
        if (strcmp(registry->entries[i].name, name) == 0) {
            // Shift remaining entries
            memmove(&registry->entries[i], &registry->entries[i + 1],
                   (registry->entry_count - i - 1) * sizeof(ModelRegistryEntry));
            registry->entry_count--;
            return true;
        }
    }

    return false;
}

const ModelRegistryEntry* model_registry_find(const ModelRegistry *registry, const char *name) {
    if (!registry || !name) {
        return NULL;
    }

    for (size_t i = 0; i < registry->entry_count; i++) {
        if (strcmp(registry->entries[i].name, name) == 0) {
            return &registry->entries[i];
        }
    }

    return NULL;
}

const ModelRegistryEntry* model_registry_list(const ModelRegistry *registry, size_t *count) {
    if (!registry) {
        if (count) *count = 0;
        return NULL;
    }

    if (count) {
        *count = registry->entry_count;
    }

    return registry->entries;
}

bool model_registry_save(const ModelRegistry *registry, const char *filename) {
    if (!registry || !filename) {
        return false;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return false;
    }

    // Write count
    if (fwrite(&registry->entry_count, sizeof(size_t), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    // Write entries
    if (registry->entry_count > 0) {
        if (fwrite(registry->entries, sizeof(ModelRegistryEntry),
                   registry->entry_count, fp) != registry->entry_count) {
            fclose(fp);
            return false;
        }
    }

    fclose(fp);
    return true;
}

ModelRegistry* model_registry_load(const char *filename) {
    if (!filename) {
        return NULL;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    ModelRegistry *registry = calloc(1, sizeof(ModelRegistry));
    if (!registry) {
        fclose(fp);
        return NULL;
    }

    // Read count
    if (fread(&registry->entry_count, sizeof(size_t), 1, fp) != 1) {
        free(registry);
        fclose(fp);
        return NULL;
    }

    // Allocate entries
    if (registry->entry_count > 0) {
        registry->entries = calloc(registry->entry_count, sizeof(ModelRegistryEntry));
        if (!registry->entries) {
            free(registry);
            fclose(fp);
            return NULL;
        }

        if (fread(registry->entries, sizeof(ModelRegistryEntry),
                  registry->entry_count, fp) != registry->entry_count) {
            free(registry->entries);
            free(registry);
            fclose(fp);
            return NULL;
        }
    }

    registry->entry_capacity = registry->entry_count;

    fclose(fp);
    return registry;
}

void model_registry_print(const ModelRegistry *registry) {
    if (!registry) return;

    printf("\n=== Model Registry ===\n");
    printf("Total Models: %zu\n\n", registry->entry_count);

    for (size_t i = 0; i < registry->entry_count; i++) {
        const ModelRegistryEntry *entry = &registry->entries[i];
        
        const char *type_str = "UNKNOWN";
        switch (entry->type) {
            case MODEL_TYPE_LINEAR_REGRESSION: type_str = "LINEAR_REGRESSION"; break;
            case MODEL_TYPE_DECISION_TREE: type_str = "DECISION_TREE"; break;
            case MODEL_TYPE_SVM: type_str = "SVM"; break;
            case MODEL_TYPE_KMEANS: type_str = "KMEANS"; break;
            case MODEL_TYPE_QLEARNING: type_str = "QLEARNING"; break;
            case MODEL_TYPE_NEURAL_NETWORK: type_str = "NEURAL_NETWORK"; break;
            default: break;
        }

        printf("[%zu] %s\n", i + 1, entry->name);
        printf("    Type: %s\n", type_str);
        printf("    File: %s\n", entry->filename);
        printf("    Size: %zu bytes\n", entry->file_size);
        printf("    Accuracy: %.2f%%\n", entry->accuracy * 100.0);
        printf("    Modified: %s", ctime(&entry->last_modified));
        printf("\n");
    }
}
