
#ifndef BDI_SEMANTIC_TAGS_H
#define BDI_SEMANTIC_TAGS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Semantic tag types
typedef enum {
    TAG_STREAMING_INTENSIVE,
    TAG_MEMORY_HEAVY,
    TAG_COMPUTE_BOUND,
    TAG_IO_BOUND,
    TAG_CACHE_FRIENDLY,
    TAG_PARALLEL_SAFE,
    TAG_REALTIME_CRITICAL
} SemanticTagType;

// Semantic tag
typedef struct {
    SemanticTagType type;
    uint32_t function_id;
    char function_name[64];
    double intensity;  // 0.0 to 1.0
    char metadata[128];
} SemanticTag;

// Tagged bytecode
typedef struct {
    uint8_t *bytecode;
    size_t bytecode_size;
    SemanticTag *tags;
    size_t tag_count;
} TaggedBytecode;

// Initialize semantic tags
bool semantic_tags_init(void);

// Cleanup semantic tags
void semantic_tags_cleanup(void);

// Create semantic tag
SemanticTag semantic_tags_create(SemanticTagType type, uint32_t func_id, 
                                const char *func_name, double intensity);

// Add tag to bytecode
bool semantic_tags_add(TaggedBytecode *bytecode, const SemanticTag *tag);

// Get tags for function
const SemanticTag* semantic_tags_get(const TaggedBytecode *bytecode, 
                                    uint32_t func_id, size_t *count);

// Serialize tags
bool semantic_tags_serialize(const TaggedBytecode *bytecode, const char *filename);

// Deserialize tags
TaggedBytecode* semantic_tags_deserialize(const char *filename);

// Free tagged bytecode
void semantic_tags_free(TaggedBytecode *bytecode);

#endif // BDI_SEMANTIC_TAGS_H
