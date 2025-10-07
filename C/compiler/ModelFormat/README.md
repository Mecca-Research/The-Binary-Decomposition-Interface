
# BDI Model Format

Binary serialization format for trained ML models in the BDI compiler.

## Components

### bdi_model.h/c
Core model container:
- Model metadata (name, type, version, training stats)
- Model data storage
- Checksum verification
- Integrity checking

### model_serializer.h/c
Binary serialization:
- `.bdi-model` file format
- Compression support (RLE, Huffman, LZ77)
- Efficient loading/saving
- File verification

### model_metadata.h/c
Model registry system:
- Track multiple models
- Model discovery
- Version management
- Registry persistence

## .bdi-model Format

Binary format structure:
```
Header:
  - Magic: 0x4244494D ("BDIM")
  - Version: 1
  - Compression type
  - Checksum
  - Metadata size
  - Data size
  - Compressed size

Metadata:
  - Model name
  - Model type
  - Training statistics
  - Architecture description

Data:
  - Compressed model weights/parameters
```

## Usage

```c
// Create model
BDIModel *model = bdi_model_create("my_model", MODEL_TYPE_QLEARNING);

// Set model data
bdi_model_set_data(model, weights, sizeof(weights));

// Update metadata
ModelMetadata meta = {
    .training_samples = 10000,
    .training_accuracy = 0.95,
    .validation_accuracy = 0.92
};
bdi_model_update_metadata(model, &meta);

// Save to file
model_serializer_save(model, "my_model.bdi-model", COMPRESS_RLE);

// Load from file
BDIModel *loaded = model_serializer_load("my_model.bdi-model");

// Verify integrity
if (bdi_model_verify(loaded)) {
    printf("Model is valid\n");
}

// Model registry
ModelRegistry *registry = model_registry_create();
model_registry_add(registry, "my_model", "my_model.bdi-model", model);
model_registry_print(registry);

// Cleanup
bdi_model_free(model);
bdi_model_free(loaded);
model_registry_free(registry);
```

## Compression

Supported compression types:
- `COMPRESS_NONE`: No compression
- `COMPRESS_RLE`: Run-length encoding (good for sparse data)
- `COMPRESS_HUFFMAN`: Huffman coding (good for general data)
- `COMPRESS_LZ77`: LZ77 compression (good for repetitive data)
