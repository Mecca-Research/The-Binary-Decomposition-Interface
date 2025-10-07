
#include "bdi_model.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

BDIModel* bdi_model_create(const char *name, ModelType type) {
    BDIModel *model = calloc(1, sizeof(BDIModel));
    if (!model) {
        return NULL;
    }

    if (name) {
        strncpy(model->metadata.name, name, sizeof(model->metadata.name) - 1);
    }

    model->metadata.type = type;
    model->metadata.version = BDI_MODEL_VERSION;
    model->metadata.created_at = time(NULL);

    return model;
}

void bdi_model_free(BDIModel *model) {
    if (!model) return;
    free(model->model_data);
    free(model);
}

bool bdi_model_set_data(BDIModel *model, const void *data, size_t size) {
    if (!model || !data || size == 0) {
        return false;
    }

    // Free existing data
    free(model->model_data);

    // Allocate and copy new data
    model->model_data = malloc(size);
    if (!model->model_data) {
        model->model_data_size = 0;
        return false;
    }

    memcpy(model->model_data, data, size);
    model->model_data_size = size;

    // Update checksum
    model->checksum = bdi_model_calculate_checksum(model);

    return true;
}

const void* bdi_model_get_data(const BDIModel *model, size_t *size) {
    if (!model) {
        if (size) *size = 0;
        return NULL;
    }

    if (size) {
        *size = model->model_data_size;
    }

    return model->model_data;
}

void bdi_model_update_metadata(BDIModel *model, const ModelMetadata *metadata) {
    if (!model || !metadata) return;
    
    // Preserve created_at
    time_t created = model->metadata.created_at;
    model->metadata = *metadata;
    model->metadata.created_at = created;
}

uint32_t bdi_model_calculate_checksum(const BDIModel *model) {
    if (!model || !model->model_data) {
        return 0;
    }

    // Simple CRC32-like checksum
    uint32_t checksum = 0xFFFFFFFF;
    const uint8_t *data = model->model_data;

    for (size_t i = 0; i < model->model_data_size; i++) {
        checksum ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (checksum & 1) {
                checksum = (checksum >> 1) ^ 0xEDB88320;
            } else {
                checksum >>= 1;
            }
        }
    }

    return ~checksum;
}

bool bdi_model_verify(const BDIModel *model) {
    if (!model) {
        return false;
    }

    uint32_t calculated = bdi_model_calculate_checksum(model);
    return calculated == model->checksum;
}
