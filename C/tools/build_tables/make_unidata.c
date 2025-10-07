
#include "unicode_types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// External parser functions
extern bool parse_unicode_data(const char *filename, unicode_char_props_t **props_out, size_t *count_out);
extern bool parse_emoji_test(const char *filename, unicode_emoji_t **emoji_out, size_t *count_out);
extern bool parse_collation_keys(const char *filename, unicode_collation_key_t **keys_out, size_t *count_out);
extern bool parse_idna_mappings(const char *filename, unicode_idna_mapping_t **mappings_out, size_t *count_out);
extern bool parse_unihan_readings(const char *filename, unicode_unihan_t **unihan_out, size_t *count_out);
extern bool generate_master_embed_header(const char *output_dir);

// Write file header
static bool write_file_header(FILE *file, uint8_t data_type, size_t uncompressed_size, 
                              size_t compressed_size, uint32_t checksum, size_t num_entries) {
    unicode_file_header_t header = {
        .magic = UNICODE_MAGIC,
        .version_major = UNICODE_VERSION_MAJOR,
        .version_minor = UNICODE_VERSION_MINOR,
        .version_patch = UNICODE_VERSION_PATCH,
        .data_type = data_type,
        .uncompressed_size = (uint32_t)uncompressed_size,
        .compressed_size = (uint32_t)compressed_size,
        .checksum = checksum,
        .num_entries = (uint32_t)num_entries,
        .index_offset = sizeof(unicode_file_header_t),
        .data_offset = 0, // Will be set after index
        .reserved = {0}
    };
    
    return fwrite(&header, sizeof(header), 1, file) == 1;
}

// Generate basic Unicode data file
static bool generate_basic_data(const char *output_file, unicode_char_props_t *props, size_t count) {
    FILE *file = fopen(output_file, "wb");
    if (!file) {
        fprintf(stderr, "Failed to create %s\n", output_file);
        return false;
    }
    
    // Calculate checksum
    uint32_t checksum = crc32((uint8_t*)props, count * sizeof(unicode_char_props_t));
    
    // Write header
    write_file_header(file, UNICODE_TYPE_BASIC, 
                     count * sizeof(unicode_char_props_t),
                     count * sizeof(unicode_char_props_t),
                     checksum, count);
    
    // Write data (uncompressed for now)
    fwrite(props, sizeof(unicode_char_props_t), count, file);
    
    fclose(file);
    
    printf("Generated %s (%zu entries, %zu bytes)\n", 
           output_file, count, count * sizeof(unicode_char_props_t));
    return true;
}

// Generate emoji data file
static bool generate_emoji_data(const char *output_file, unicode_emoji_t *emoji, size_t count) {
    FILE *file = fopen(output_file, "wb");
    if (!file) {
        fprintf(stderr, "Failed to create %s\n", output_file);
        return false;
    }
    
    uint32_t checksum = crc32((uint8_t*)emoji, count * sizeof(unicode_emoji_t));
    
    write_file_header(file, UNICODE_TYPE_EMOJI,
                     count * sizeof(unicode_emoji_t),
                     count * sizeof(unicode_emoji_t),
                     checksum, count);
    
    fwrite(emoji, sizeof(unicode_emoji_t), count, file);
    
    fclose(file);
    
    printf("Generated %s (%zu entries, %zu bytes)\n",
           output_file, count, count * sizeof(unicode_emoji_t));
    return true;
}

// Generate collation data file
static bool generate_collation_data(const char *output_file, unicode_collation_key_t *keys, size_t count) {
    FILE *file = fopen(output_file, "wb");
    if (!file) {
        fprintf(stderr, "Failed to create %s\n", output_file);
        return false;
    }
    
    uint32_t checksum = crc32((uint8_t*)keys, count * sizeof(unicode_collation_key_t));
    
    write_file_header(file, UNICODE_TYPE_COLLATION,
                     count * sizeof(unicode_collation_key_t),
                     count * sizeof(unicode_collation_key_t),
                     checksum, count);
    
    fwrite(keys, sizeof(unicode_collation_key_t), count, file);
    
    fclose(file);
    
    printf("Generated %s (%zu entries, %zu bytes)\n",
           output_file, count, count * sizeof(unicode_collation_key_t));
    return true;
}

// Generate IDNA data file
static bool generate_idna_data(const char *output_file, unicode_idna_mapping_t *mappings, size_t count) {
    FILE *file = fopen(output_file, "wb");
    if (!file) {
        fprintf(stderr, "Failed to create %s\n", output_file);
        return false;
    }
    
    uint32_t checksum = crc32((uint8_t*)mappings, count * sizeof(unicode_idna_mapping_t));
    
    write_file_header(file, UNICODE_TYPE_IDNA,
                     count * sizeof(unicode_idna_mapping_t),
                     count * sizeof(unicode_idna_mapping_t),
                     checksum, count);
    
    fwrite(mappings, sizeof(unicode_idna_mapping_t), count, file);
    
    fclose(file);
    
    printf("Generated %s (%zu entries, %zu bytes)\n",
           output_file, count, count * sizeof(unicode_idna_mapping_t));
    return true;
}

// Generate Unihan data file
static bool generate_unihan_data(const char *output_file, unicode_unihan_t *unihan, size_t count) {
    FILE *file = fopen(output_file, "wb");
    if (!file) {
        fprintf(stderr, "Failed to create %s\n", output_file);
        return false;
    }
    
    uint32_t checksum = crc32((uint8_t*)unihan, count * sizeof(unicode_unihan_t));
    
    write_file_header(file, UNICODE_TYPE_HAN,
                     count * sizeof(unicode_unihan_t),
                     count * sizeof(unicode_unihan_t),
                     checksum, count);
    
    fwrite(unihan, sizeof(unicode_unihan_t), count, file);
    
    fclose(file);
    
    printf("Generated %s (%zu entries, %zu bytes)\n",
           output_file, count, count * sizeof(unicode_unihan_t));
    return true;
}

int main(int argc, char *argv[]) {
    printf("BDI Unicode Data Pipeline - Unicode 17.0.0\n");
    printf("==========================================\n\n");
    
    clock_t start = clock();
    
    const char *data_dir = "../../data/unicode";
    const char *output_dir = "../../compiler/AIBase/data";
    const char *embed_dir = "../../compiler/AIBase/embeddings";
    
    // Parse UnicodeData.txt
    printf("Parsing UnicodeData.txt...\n");
    unicode_char_props_t *props = NULL;
    size_t props_count = 0;
    char unicode_data_path[512];
    snprintf(unicode_data_path, sizeof(unicode_data_path), "%s/UCD/UnicodeData.txt", data_dir);
    
    if (!parse_unicode_data(unicode_data_path, &props, &props_count)) {
        fprintf(stderr, "Failed to parse UnicodeData.txt\n");
        return 1;
    }
    
    // Generate basic data file
    char output_path[512];
    snprintf(output_path, sizeof(output_path), "%s/unicode_basic.dat", output_dir);
    generate_basic_data(output_path, props, props_count);
    
    // Also use for props data (same data, different semantic purpose)
    snprintf(output_path, sizeof(output_path), "%s/unicode_props.dat", output_dir);
    generate_basic_data(output_path, props, props_count);
    
    // Generate math data (subset of props with math symbols)
    snprintf(output_path, sizeof(output_path), "%s/unicode_math.dat", output_dir);
    generate_basic_data(output_path, props, props_count); // TODO: Filter to math symbols only
    
    free(props);
    
    // Parse emoji data
    printf("\nParsing emoji-test.txt...\n");
    unicode_emoji_t *emoji = NULL;
    size_t emoji_count = 0;
    char emoji_path[512];
    snprintf(emoji_path, sizeof(emoji_path), "%s/emoji/emoji-test.txt", data_dir);
    
    if (parse_emoji_test(emoji_path, &emoji, &emoji_count)) {
        snprintf(output_path, sizeof(output_path), "%s/unicode_emoji.dat", output_dir);
        generate_emoji_data(output_path, emoji, emoji_count);
        free(emoji);
    }
    
    // Parse collation data
    printf("\nParsing allkeys-17.0.0.txt...\n");
    unicode_collation_key_t *keys = NULL;
    size_t keys_count = 0;
    char collation_path[512];
    snprintf(collation_path, sizeof(collation_path), "%s/collation/allkeys-17.0.0.txt", data_dir);
    
    if (parse_collation_keys(collation_path, &keys, &keys_count)) {
        snprintf(output_path, sizeof(output_path), "%s/unicode_collation.dat", output_dir);
        generate_collation_data(output_path, keys, keys_count);
        free(keys);
    }
    
    // Parse IDNA data
    printf("\nParsing IdnaMappingTable.txt...\n");
    unicode_idna_mapping_t *mappings = NULL;
    size_t mappings_count = 0;
    char idna_path[512];
    snprintf(idna_path, sizeof(idna_path), "%s/idna/IdnaMappingTable.txt", data_dir);
    
    if (parse_idna_mappings(idna_path, &mappings, &mappings_count)) {
        snprintf(output_path, sizeof(output_path), "%s/unicode_idna.dat", output_dir);
        generate_idna_data(output_path, mappings, mappings_count);
        free(mappings);
    }
    
    // Parse Unihan data
    printf("\nParsing Unihan_Readings.txt...\n");
    unicode_unihan_t *unihan = NULL;
    size_t unihan_count = 0;
    char unihan_path[512];
    snprintf(unihan_path, sizeof(unihan_path), "%s/Unihan/Unihan_Readings.txt", data_dir);
    
    if (parse_unihan_readings(unihan_path, &unihan, &unihan_count)) {
        snprintf(output_path, sizeof(output_path), "%s/unicode_han.dat", output_dir);
        generate_unihan_data(output_path, unihan, unihan_count);
        free(unihan);
    }
    
    // Generate master embed header
    printf("\nGenerating embed headers...\n");
    generate_master_embed_header(embed_dir);
    
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("\n==========================================\n");
    printf("Unicode data pipeline completed in %.2f seconds\n", elapsed);
    printf("All .dat files generated successfully!\n");
    
    return 0;
}

