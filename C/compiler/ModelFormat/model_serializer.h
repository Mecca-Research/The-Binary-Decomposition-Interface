
#ifndef BDI_MODEL_SERIALIZER_H
#define BDI_MODEL_SERIALIZER_H

#include "bdi_model.h"
#include <stdbool.h>

// Compression options
typedef enum {
    COMPRESS_NONE,
    COMPRESS_RLE,      // Run-length encoding
    COMPRESS_HUFFMAN,  // Huffman coding
    COMPRESS_LZ77      // LZ77 compression
} CompressionType;

// Save model to .bdi-model file
bool model_serializer_save(const BDIModel *model, const char *filename, CompressionType compression);

// Load model from .bdi-model file
BDIModel* model_serializer_load(const char *filename);

// Get file size
size_t model_serializer_get_file_size(const char *filename);

// Verify file integrity
bool model_serializer_verify_file(const char *filename);

#endif // BDI_MODEL_SERIALIZER_H
