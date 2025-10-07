
#include "model_serializer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint32_t magic;
    uint32_t version;
    CompressionType compression;
    uint32_t checksum;
    size_t metadata_size;
    size_t data_size;
    size_t compressed_size;
} ModelFileHeader;

// Simple RLE compression
static size_t compress_rle(const uint8_t *input, size_t input_size, uint8_t **output) {
    // Worst case: 2x size (every byte different)
    *output = malloc(input_size * 2);
    if (!*output) return 0;

    size_t out_idx = 0;
    size_t i = 0;

    while (i < input_size) {
        uint8_t current = input[i];
        size_t count = 1;

        while (i + count < input_size && input[i + count] == current && count < 255) {
            count++;
        }

        (*output)[out_idx++] = (uint8_t)count;
        (*output)[out_idx++] = current;
        i += count;
    }

    return out_idx;
}

static size_t decompress_rle(const uint8_t *input, size_t input_size, uint8_t **output, size_t expected_size) {
    *output = malloc(expected_size);
    if (!*output) return 0;

    size_t out_idx = 0;
    size_t i = 0;

    while (i < input_size && out_idx < expected_size) {
        uint8_t count = input[i++];
        uint8_t value = input[i++];

        for (uint8_t j = 0; j < count && out_idx < expected_size; j++) {
            (*output)[out_idx++] = value;
        }
    }

    return out_idx;
}

bool model_serializer_save(const BDIModel *model, const char *filename, CompressionType compression) {
    if (!model || !filename) {
        return false;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return false;
    }

    // Prepare data for compression
    uint8_t *compressed_data = NULL;
    size_t compressed_size = model->model_data_size;

    if (compression == COMPRESS_RLE && model->model_data) {
        compressed_size = compress_rle(model->model_data, model->model_data_size, &compressed_data);
        if (compressed_size == 0) {
            compressed_data = NULL;
            compressed_size = model->model_data_size;
        }
    } else {
        compressed_data = model->model_data;
    }

    // Write header
    ModelFileHeader header = {
        .magic = BDI_MODEL_MAGIC,
        .version = BDI_MODEL_VERSION,
        .compression = compression,
        .checksum = model->checksum,
        .metadata_size = sizeof(ModelMetadata),
        .data_size = model->model_data_size,
        .compressed_size = compressed_size
    };

    if (fwrite(&header, sizeof(header), 1, fp) != 1) {
        if (compression == COMPRESS_RLE && compressed_data != model->model_data) {
            free(compressed_data);
        }
        fclose(fp);
        return false;
    }

    // Write metadata
    if (fwrite(&model->metadata, sizeof(ModelMetadata), 1, fp) != 1) {
        if (compression == COMPRESS_RLE && compressed_data != model->model_data) {
            free(compressed_data);
        }
        fclose(fp);
        return false;
    }

    // Write model data
    if (compressed_data && compressed_size > 0) {
        if (fwrite(compressed_data, 1, compressed_size, fp) != compressed_size) {
            if (compression == COMPRESS_RLE && compressed_data != model->model_data) {
                free(compressed_data);
            }
            fclose(fp);
            return false;
        }
    }

    if (compression == COMPRESS_RLE && compressed_data != model->model_data) {
        free(compressed_data);
    }

    fclose(fp);
    return true;
}

BDIModel* model_serializer_load(const char *filename) {
    if (!filename) {
        return NULL;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    // Read header
    ModelFileHeader header;
    if (fread(&header, sizeof(header), 1, fp) != 1) {
        fclose(fp);
        return NULL;
    }

    // Verify magic and version
    if (header.magic != BDI_MODEL_MAGIC || header.version != BDI_MODEL_VERSION) {
        fclose(fp);
        return NULL;
    }

    // Create model
    BDIModel *model = calloc(1, sizeof(BDIModel));
    if (!model) {
        fclose(fp);
        return NULL;
    }

    // Read metadata
    if (fread(&model->metadata, sizeof(ModelMetadata), 1, fp) != 1) {
        free(model);
        fclose(fp);
        return NULL;
    }

    // Read and decompress model data
    if (header.compressed_size > 0) {
        uint8_t *compressed_data = malloc(header.compressed_size);
        if (!compressed_data) {
            free(model);
            fclose(fp);
            return NULL;
        }

        if (fread(compressed_data, 1, header.compressed_size, fp) != header.compressed_size) {
            free(compressed_data);
            free(model);
            fclose(fp);
            return NULL;
        }

        if (header.compression == COMPRESS_RLE) {
            uint8_t *decompressed_data;
            size_t decompressed_size = decompress_rle(compressed_data, header.compressed_size,
                                                     &decompressed_data, header.data_size);
            free(compressed_data);

            if (decompressed_size != header.data_size) {
                free(decompressed_data);
                free(model);
                fclose(fp);
                return NULL;
            }

            model->model_data = decompressed_data;
            model->model_data_size = decompressed_size;
        } else {
            model->model_data = compressed_data;
            model->model_data_size = header.compressed_size;
        }
    }

    model->checksum = header.checksum;

    fclose(fp);

    // Verify integrity
    if (!bdi_model_verify(model)) {
        bdi_model_free(model);
        return NULL;
    }

    return model;
}

size_t model_serializer_get_file_size(const char *filename) {
    if (!filename) return 0;

    FILE *fp = fopen(filename, "rb");
    if (!fp) return 0;

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fclose(fp);

    return size;
}

bool model_serializer_verify_file(const char *filename) {
    BDIModel *model = model_serializer_load(filename);
    if (!model) return false;

    bool valid = bdi_model_verify(model);
    bdi_model_free(model);

    return valid;
}
