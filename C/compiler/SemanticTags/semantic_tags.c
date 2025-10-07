
#include "semantic_tags.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static bool semantic_tags_initialized = false;

bool semantic_tags_init(void) {
    if (semantic_tags_initialized) {
        return true;
    }
    semantic_tags_initialized = true;
    return true;
}

void semantic_tags_cleanup(void) {
    semantic_tags_initialized = false;
}

SemanticTag semantic_tags_create(SemanticTagType type, uint32_t func_id,
                                const char *func_name, double intensity) {
    SemanticTag tag = {
        .type = type,
        .function_id = func_id,
        .intensity = intensity
    };

    if (func_name) {
        strncpy(tag.function_name, func_name, sizeof(tag.function_name) - 1);
    }

    return tag;
}

bool semantic_tags_add(TaggedBytecode *bytecode, const SemanticTag *tag) {
    if (!bytecode || !tag) {
        return false;
    }

    // Expand tag array if needed
    SemanticTag *new_tags = realloc(bytecode->tags,
                                   (bytecode->tag_count + 1) * sizeof(SemanticTag));
    if (!new_tags) {
        return false;
    }

    bytecode->tags = new_tags;
    bytecode->tags[bytecode->tag_count++] = *tag;

    return true;
}

const SemanticTag* semantic_tags_get(const TaggedBytecode *bytecode,
                                    uint32_t func_id, size_t *count) {
    if (!bytecode || !count) {
        return NULL;
    }

    *count = 0;

    // Count matching tags
    for (size_t i = 0; i < bytecode->tag_count; i++) {
        if (bytecode->tags[i].function_id == func_id) {
            (*count)++;
        }
    }

    if (*count == 0) {
        return NULL;
    }

    // Return first matching tag (simplified)
    for (size_t i = 0; i < bytecode->tag_count; i++) {
        if (bytecode->tags[i].function_id == func_id) {
            return &bytecode->tags[i];
        }
    }

    return NULL;
}

bool semantic_tags_serialize(const TaggedBytecode *bytecode, const char *filename) {
    if (!bytecode || !filename) {
        return false;
    }

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return false;
    }

    // Write bytecode size and data
    fwrite(&bytecode->bytecode_size, sizeof(size_t), 1, fp);
    if (bytecode->bytecode_size > 0) {
        fwrite(bytecode->bytecode, 1, bytecode->bytecode_size, fp);
    }

    // Write tag count and tags
    fwrite(&bytecode->tag_count, sizeof(size_t), 1, fp);
    if (bytecode->tag_count > 0) {
        fwrite(bytecode->tags, sizeof(SemanticTag), bytecode->tag_count, fp);
    }

    fclose(fp);
    return true;
}

TaggedBytecode* semantic_tags_deserialize(const char *filename) {
    if (!filename) {
        return NULL;
    }

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        return NULL;
    }

    TaggedBytecode *bytecode = calloc(1, sizeof(TaggedBytecode));
    if (!bytecode) {
        fclose(fp);
        return NULL;
    }

    // Read bytecode
    fread(&bytecode->bytecode_size, sizeof(size_t), 1, fp);
    if (bytecode->bytecode_size > 0) {
        bytecode->bytecode = malloc(bytecode->bytecode_size);
        if (!bytecode->bytecode) {
            free(bytecode);
            fclose(fp);
            return NULL;
        }
        fread(bytecode->bytecode, 1, bytecode->bytecode_size, fp);
    }

    // Read tags
    fread(&bytecode->tag_count, sizeof(size_t), 1, fp);
    if (bytecode->tag_count > 0) {
        bytecode->tags = calloc(bytecode->tag_count, sizeof(SemanticTag));
        if (!bytecode->tags) {
            free(bytecode->bytecode);
            free(bytecode);
            fclose(fp);
            return NULL;
        }
        fread(bytecode->tags, sizeof(SemanticTag), bytecode->tag_count, fp);
    }

    fclose(fp);
    return bytecode;
}

void semantic_tags_free(TaggedBytecode *bytecode) {
    if (!bytecode) return;
    free(bytecode->bytecode);
    free(bytecode->tags);
    free(bytecode);
}
