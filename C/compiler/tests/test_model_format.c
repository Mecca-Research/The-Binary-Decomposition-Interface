
#include "../ModelFormat/bdi_model.h"
#include "../ModelFormat/model_serializer.h"
#include "../ModelFormat/model_metadata.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_model_creation(void) {
    printf("Testing model creation...\n");

    BDIModel *model = bdi_model_create("test_model", MODEL_TYPE_QLEARNING);
    assert(model != NULL);
    assert(strcmp(model->metadata.name, "test_model") == 0);
    assert(model->metadata.type == MODEL_TYPE_QLEARNING);

    // Set model data
    double weights[] = {1.0, 2.0, 3.0, 4.0};
    assert(bdi_model_set_data(model, weights, sizeof(weights)));
    assert(model->model_data_size == sizeof(weights));

    // Verify model
    assert(bdi_model_verify(model));

    bdi_model_free(model);

    printf("✓ Model creation test passed\n");
}

void test_model_serialization(void) {
    printf("Testing model serialization...\n");

    BDIModel *model = bdi_model_create("serialization_test", MODEL_TYPE_LINEAR_REGRESSION);
    assert(model != NULL);

    double weights[] = {0.5, 1.5, 2.5, 3.5};
    assert(bdi_model_set_data(model, weights, sizeof(weights)));

    ModelMetadata meta = model->metadata;
    meta.training_samples = 1000;
    meta.training_accuracy = 0.95;
    meta.validation_accuracy = 0.92;
    bdi_model_update_metadata(model, &meta);

    // Save model
    assert(model_serializer_save(model, "/tmp/test_model.bdi-model", COMPRESS_RLE));

    // Load model
    BDIModel *loaded = model_serializer_load("/tmp/test_model.bdi-model");
    assert(loaded != NULL);
    assert(strcmp(loaded->metadata.name, "serialization_test") == 0);
    assert(loaded->metadata.training_samples == 1000);
    assert(bdi_model_verify(loaded));

    // Verify file
    assert(model_serializer_verify_file("/tmp/test_model.bdi-model"));

    bdi_model_free(model);
    bdi_model_free(loaded);

    printf("✓ Model serialization test passed\n");
}

void test_model_registry(void) {
    printf("Testing model registry...\n");

    ModelRegistry *registry = model_registry_create();
    assert(registry != NULL);
    assert(registry->entry_count == 0);

    // Create and add models
    BDIModel *model1 = bdi_model_create("model1", MODEL_TYPE_DECISION_TREE);
    model_serializer_save(model1, "/tmp/model1.bdi-model", COMPRESS_NONE);
    assert(model_registry_add(registry, "model1", "/tmp/model1.bdi-model", model1));

    BDIModel *model2 = bdi_model_create("model2", MODEL_TYPE_SVM);
    model_serializer_save(model2, "/tmp/model2.bdi-model", COMPRESS_NONE);
    assert(model_registry_add(registry, "model2", "/tmp/model2.bdi-model", model2));

    assert(registry->entry_count == 2);

    // Find model
    const ModelRegistryEntry *entry = model_registry_find(registry, "model1");
    assert(entry != NULL);
    assert(strcmp(entry->name, "model1") == 0);

    // Print registry
    model_registry_print(registry);

    // Save and load registry
    assert(model_registry_save(registry, "/tmp/test_registry.dat"));
    ModelRegistry *loaded_registry = model_registry_load("/tmp/test_registry.dat");
    assert(loaded_registry != NULL);
    assert(loaded_registry->entry_count == 2);

    bdi_model_free(model1);
    bdi_model_free(model2);
    model_registry_free(registry);
    model_registry_free(loaded_registry);

    printf("✓ Model registry test passed\n");
}

void test_model_compression(void) {
    printf("Testing model compression...\n");

    BDIModel *model = bdi_model_create("compression_test", MODEL_TYPE_KMEANS);
    
    // Create data with repetition (good for RLE)
    double data[100];
    for (int i = 0; i < 100; i++) {
        data[i] = (i % 10) * 1.0;
    }
    
    assert(bdi_model_set_data(model, data, sizeof(data)));

    // Save with compression
    assert(model_serializer_save(model, "/tmp/compressed.bdi-model", COMPRESS_RLE));
    size_t compressed_size = model_serializer_get_file_size("/tmp/compressed.bdi-model");

    // Save without compression
    assert(model_serializer_save(model, "/tmp/uncompressed.bdi-model", COMPRESS_NONE));
    size_t uncompressed_size = model_serializer_get_file_size("/tmp/uncompressed.bdi-model");

    printf("Compressed size: %zu bytes\n", compressed_size);
    printf("Uncompressed size: %zu bytes\n", uncompressed_size);

    // Load and verify
    BDIModel *loaded = model_serializer_load("/tmp/compressed.bdi-model");
    assert(loaded != NULL);
    assert(bdi_model_verify(loaded));

    bdi_model_free(model);
    bdi_model_free(loaded);

    printf("✓ Model compression test passed\n");
}

int main(void) {
    printf("\n=== Running Model Format Tests ===\n\n");

    test_model_creation();
    test_model_serialization();
    test_model_registry();
    test_model_compression();

    printf("\n=== All Model Format Tests Passed ===\n");
    return 0;
}
