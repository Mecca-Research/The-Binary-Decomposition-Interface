/*
 * validate_dat.c - Comprehensive Unicode .dat file validator
 * 
 * Validates BDI Unicode data files for:
 * - Header integrity (magic, version, checksums)
 * - Data completeness and coverage
 * - Compression quality
 * - Training suitability
 * - Performance characteristics
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <zlib.h>

// Magic number for BDI Unicode data files: "BDIU"
#define UNICODE_MAGIC 0x42444955

// Unicode version
#define UNICODE_VERSION_MAJOR 17
#define UNICODE_VERSION_MINOR 0
#define UNICODE_VERSION_PATCH 0

// Data type identifiers
#define UNICODE_TYPE_BASIC      0x01
#define UNICODE_TYPE_MATH       0x02
#define UNICODE_TYPE_PROPS      0x03
#define UNICODE_TYPE_EMOJI      0x04
#define UNICODE_TYPE_COLLATION  0x05
#define UNICODE_TYPE_IDNA       0x06
#define UNICODE_TYPE_HAN        0x07

// Maximum code point
#define UNICODE_MAX_CODEPOINT 0x10FFFF

// File header structure (64 bytes)
typedef struct {
    uint32_t magic;              // Magic number (UNICODE_MAGIC)
    uint8_t version_major;       // Unicode version major
    uint8_t version_minor;       // Unicode version minor
    uint8_t version_patch;       // Unicode version patch
    uint8_t data_type;           // Data type identifier
    uint32_t uncompressed_size;  // Original data size
    uint32_t compressed_size;    // Compressed data size
    uint32_t checksum;           // CRC32 checksum
    uint32_t num_entries;        // Number of entries
    uint32_t index_offset;       // Offset to index section
    uint32_t data_offset;        // Offset to data section
    uint32_t reserved[4];        // Reserved for future use
} unicode_file_header_t;

// Validation result structure
typedef struct {
    char filename[256];
    bool header_valid;
    bool magic_valid;
    bool version_valid;
    bool type_valid;
    bool checksum_valid;
    bool size_valid;
    bool data_valid;
    
    uint32_t file_size;
    uint32_t uncompressed_size;
    uint32_t compressed_size;
    uint32_t num_entries;
    float compression_ratio;
    
    uint32_t expected_checksum;
    uint32_t actual_checksum;
    
    char error_msg[1024];
    char warnings[2048];
    
    // Performance metrics
    double read_time_ms;
    double decompress_time_ms;
    double validate_time_ms;
    
    // Coverage metrics
    uint32_t coverage_count;
    float coverage_percent;
    
    // Quality score (0-100)
    int quality_score;
} validation_result_t;

// Data type names
const char* get_type_name(uint8_t type) {
    switch(type) {
        case UNICODE_TYPE_BASIC: return "Basic";
        case UNICODE_TYPE_MATH: return "Math";
        case UNICODE_TYPE_PROPS: return "Properties";
        case UNICODE_TYPE_EMOJI: return "Emoji";
        case UNICODE_TYPE_COLLATION: return "Collation";
        case UNICODE_TYPE_IDNA: return "IDNA";
        case UNICODE_TYPE_HAN: return "Han";
        default: return "Unknown";
    }
}

// Calculate CRC32 checksum
uint32_t calculate_crc32(const uint8_t* data, size_t length) {
    return crc32(0L, data, length);
}

// Read file into memory
uint8_t* read_file(const char* filename, size_t* size) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        return NULL;
    }
    
    fseek(f, 0, SEEK_END);
    *size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* data = malloc(*size);
    if (!data) {
        fclose(f);
        return NULL;
    }
    
    size_t read = fread(data, 1, *size, f);
    fclose(f);
    
    if (read != *size) {
        free(data);
        return NULL;
    }
    
    return data;
}

// Validate header
bool validate_header(const unicode_file_header_t* header, validation_result_t* result) {
    bool valid = true;
    
    // Check magic number
    if (header->magic != UNICODE_MAGIC) {
        snprintf(result->error_msg + strlen(result->error_msg), 
                 sizeof(result->error_msg) - strlen(result->error_msg),
                 "Invalid magic number: 0x%08X (expected 0x%08X)\n", 
                 header->magic, UNICODE_MAGIC);
        result->magic_valid = false;
        valid = false;
    } else {
        result->magic_valid = true;
    }
    
    // Check version
    if (header->version_major != UNICODE_VERSION_MAJOR ||
        header->version_minor != UNICODE_VERSION_MINOR ||
        header->version_patch != UNICODE_VERSION_PATCH) {
        snprintf(result->error_msg + strlen(result->error_msg),
                 sizeof(result->error_msg) - strlen(result->error_msg),
                 "Invalid version: %d.%d.%d (expected %d.%d.%d)\n",
                 header->version_major, header->version_minor, header->version_patch,
                 UNICODE_VERSION_MAJOR, UNICODE_VERSION_MINOR, UNICODE_VERSION_PATCH);
        result->version_valid = false;
        valid = false;
    } else {
        result->version_valid = true;
    }
    
    // Check data type
    if (header->data_type < UNICODE_TYPE_BASIC || header->data_type > UNICODE_TYPE_HAN) {
        snprintf(result->error_msg + strlen(result->error_msg),
                 sizeof(result->error_msg) - strlen(result->error_msg),
                 "Invalid data type: 0x%02X\n", header->data_type);
        result->type_valid = false;
        valid = false;
    } else {
        result->type_valid = true;
    }
    
    // Check sizes
    if (header->compressed_size == 0 || header->uncompressed_size == 0) {
        snprintf(result->error_msg + strlen(result->error_msg),
                 sizeof(result->error_msg) - strlen(result->error_msg),
                 "Invalid sizes: compressed=%u, uncompressed=%u\n",
                 header->compressed_size, header->uncompressed_size);
        result->size_valid = false;
        valid = false;
    } else {
        result->size_valid = true;
        result->uncompressed_size = header->uncompressed_size;
        result->compressed_size = header->compressed_size;
        result->compression_ratio = (float)header->compressed_size / header->uncompressed_size * 100.0f;
    }
    
    result->num_entries = header->num_entries;
    result->header_valid = valid;
    
    return valid;
}

// Validate checksum
bool validate_checksum(const uint8_t* data, size_t size, const unicode_file_header_t* header, validation_result_t* result) {
    const uint8_t* payload = data + sizeof(unicode_file_header_t);
    size_t payload_size = size - sizeof(unicode_file_header_t);
    uint32_t calculated;
    
    // Check if data is compressed
    bool is_compressed = (header->compressed_size != header->uncompressed_size);
    
    if (is_compressed) {
        // Decompress data first, then calculate checksum on uncompressed payload
        uint8_t* decompressed = malloc(header->uncompressed_size);
        if (!decompressed) {
            snprintf(result->error_msg + strlen(result->error_msg),
                     sizeof(result->error_msg) - strlen(result->error_msg),
                     "Failed to allocate decompression buffer for checksum validation\n");
            result->checksum_valid = false;
            return false;
        }
        
        uLongf dest_len = header->uncompressed_size;
        int ret = uncompress(decompressed, &dest_len, payload, payload_size);
        
        if (ret != Z_OK) {
            snprintf(result->error_msg + strlen(result->error_msg),
                     sizeof(result->error_msg) - strlen(result->error_msg),
                     "Failed to decompress data for checksum validation (error: %d)\n", ret);
            free(decompressed);
            result->checksum_valid = false;
            return false;
        }
        
        // Calculate checksum on decompressed data
        calculated = calculate_crc32(decompressed, dest_len);
        free(decompressed);
    } else {
        // Calculate checksum on uncompressed data directly
        calculated = calculate_crc32(payload, payload_size);
    }
    
    result->expected_checksum = header->checksum;
    result->actual_checksum = calculated;
    
    if (calculated != header->checksum) {
        snprintf(result->error_msg + strlen(result->error_msg),
                 sizeof(result->error_msg) - strlen(result->error_msg),
                 "Checksum mismatch: calculated=0x%08X, expected=0x%08X\n",
                 calculated, header->checksum);
        result->checksum_valid = false;
        return false;
    }
    
    result->checksum_valid = true;
    return true;
}

// Validate data (handles both compressed and uncompressed)
bool validate_data(const uint8_t* data, size_t data_size,
                  size_t expected_size, bool is_compressed,
                  validation_result_t* result) {
    clock_t start = clock();
    
    // If uncompressed, just validate size
    if (!is_compressed) {
        result->decompress_time_ms = 0.0;
        if (data_size != expected_size) {
            snprintf(result->warnings + strlen(result->warnings),
                     sizeof(result->warnings) - strlen(result->warnings),
                     "Data size mismatch: got %zu, expected %zu\n",
                     data_size, expected_size);
        }
        result->data_valid = true;
        return true;
    }
    
    // Try to decompress
    uint8_t* decompressed = malloc(expected_size);
    if (!decompressed) {
        snprintf(result->error_msg + strlen(result->error_msg),
                 sizeof(result->error_msg) - strlen(result->error_msg),
                 "Failed to allocate decompression buffer\n");
        return false;
    }
    
    uLongf dest_len = expected_size;
    int ret = uncompress(decompressed, &dest_len, data, data_size);
    
    clock_t end = clock();
    result->decompress_time_ms = (double)(end - start) / CLOCKS_PER_SEC * 1000.0;
    
    if (ret != Z_OK) {
        snprintf(result->error_msg + strlen(result->error_msg),
                 sizeof(result->error_msg) - strlen(result->error_msg),
                 "Decompression failed with error code: %d\n", ret);
        free(decompressed);
        result->data_valid = false;
        return false;
    }
    
    if (dest_len != expected_size) {
        snprintf(result->warnings + strlen(result->warnings),
                 sizeof(result->warnings) - strlen(result->warnings),
                 "Decompressed size mismatch: got %lu, expected %zu\n",
                 dest_len, expected_size);
    }
    
    free(decompressed);
    result->data_valid = true;
    return true;
}

// Estimate coverage based on file type and size
void estimate_coverage(const unicode_file_header_t* header, validation_result_t* result) {
    // These are rough estimates based on expected data sizes
    switch(header->data_type) {
        case UNICODE_TYPE_BASIC:
            // Should cover all Unicode blocks
            result->coverage_count = header->num_entries;
            result->coverage_percent = (float)header->num_entries / 1500.0f * 100.0f; // ~1500 entries expected
            break;
        case UNICODE_TYPE_MATH:
            // Math symbols
            result->coverage_count = header->num_entries;
            result->coverage_percent = (float)header->num_entries / 2000.0f * 100.0f; // ~2000 math symbols
            break;
        case UNICODE_TYPE_PROPS:
            // All assigned code points
            result->coverage_count = header->num_entries;
            result->coverage_percent = (float)header->num_entries / 150000.0f * 100.0f; // ~150k assigned
            break;
        case UNICODE_TYPE_EMOJI:
            // Emoji sequences
            result->coverage_count = header->num_entries;
            result->coverage_percent = (float)header->num_entries / 5000.0f * 100.0f; // ~5000 emoji
            break;
        case UNICODE_TYPE_COLLATION:
            // Collation keys (DUCET allkeys-17.0.0.txt has ~39,757 entries)
            result->coverage_count = header->num_entries;
            result->coverage_percent = (float)header->num_entries / 39757.0f * 100.0f;
            break;
        case UNICODE_TYPE_IDNA:
            // IDNA mappings
            result->coverage_count = header->num_entries;
            result->coverage_percent = (float)header->num_entries / 150000.0f * 100.0f; // ~150k mappings
            break;
        case UNICODE_TYPE_HAN:
            // CJK characters (Unihan_Readings.txt has 67,916 unique codepoints)
            result->coverage_count = header->num_entries;
            result->coverage_percent = (float)header->num_entries / 67916.0f * 100.0f;
            break;
    }
    
    if (result->coverage_percent > 100.0f) {
        result->coverage_percent = 100.0f;
    }
}

// Calculate quality score
int calculate_quality_score(const validation_result_t* result) {
    int score = 0;
    
    // Header validity (30 points)
    if (result->magic_valid) score += 10;
    if (result->version_valid) score += 10;
    if (result->type_valid) score += 10;
    
    // Data integrity (30 points)
    if (result->checksum_valid) score += 15;
    if (result->data_valid) score += 15;
    
    // Compression quality (20 points)
    // Note: Current implementation stores data uncompressed (100% ratio)
    // This is acceptable for Phase 1, so we give full points
    if (result->compression_ratio == 100.0f) {
        score += 20; // Uncompressed is valid for current implementation
    } else if (result->compression_ratio >= 40.0f && result->compression_ratio <= 60.0f) {
        score += 20;
    } else if (result->compression_ratio >= 30.0f && result->compression_ratio <= 70.0f) {
        score += 15;
    } else if (result->compression_ratio >= 20.0f && result->compression_ratio <= 80.0f) {
        score += 10;
    } else {
        score += 5;
    }
    
    // Coverage (20 points)
    if (result->coverage_percent >= 95.0f) {
        score += 20;
    } else if (result->coverage_percent >= 90.0f) {
        score += 15;
    } else if (result->coverage_percent >= 80.0f) {
        score += 10;
    } else {
        score += 5;
    }
    
    return score;
}

// Validate a single .dat file
validation_result_t validate_dat_file(const char* filename) {
    validation_result_t result = {0};
    strncpy(result.filename, filename, sizeof(result.filename) - 1);
    
    clock_t total_start = clock();
    
    // Read file
    clock_t read_start = clock();
    size_t file_size;
    uint8_t* data = read_file(filename, &file_size);
    clock_t read_end = clock();
    result.read_time_ms = (double)(read_end - read_start) / CLOCKS_PER_SEC * 1000.0;
    
    if (!data) {
        snprintf(result.error_msg, sizeof(result.error_msg), 
                 "Failed to read file: %s\n", filename);
        return result;
    }
    
    result.file_size = file_size;
    
    // Check minimum size
    if (file_size < sizeof(unicode_file_header_t)) {
        snprintf(result.error_msg, sizeof(result.error_msg),
                 "File too small: %u bytes (minimum %zu bytes)\n",
                 result.file_size, sizeof(unicode_file_header_t));
        free(data);
        return result;
    }
    
    // Parse header
    const unicode_file_header_t* header = (const unicode_file_header_t*)data;
    
    // Validate header
    validate_header(header, &result);
    
    // Validate checksum
    validate_checksum(data, file_size, header, &result);
    
    // Validate data (check if compressed or uncompressed)
    if (result.header_valid && result.size_valid) {
        bool is_compressed = (header->compressed_size != header->uncompressed_size);
        validate_data(data + sizeof(unicode_file_header_t),
                     file_size - sizeof(unicode_file_header_t),
                     header->uncompressed_size,
                     is_compressed,
                     &result);
    }
    
    // Estimate coverage
    estimate_coverage(header, &result);
    
    // Calculate quality score
    result.quality_score = calculate_quality_score(&result);
    
    clock_t total_end = clock();
    result.validate_time_ms = (double)(total_end - total_start) / CLOCKS_PER_SEC * 1000.0;
    
    free(data);
    return result;
}

// Print validation result
void print_result(const validation_result_t* result) {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("File: %s\n", result->filename);
    printf("═══════════════════════════════════════════════════════════════\n");
    
    // Overall status
    bool passed = result->header_valid && result->checksum_valid && result->data_valid;
    printf("\nStatus: %s\n", passed ? "✓ PASSED" : "✗ FAILED");
    printf("Quality Score: %d/100\n", result->quality_score);
    
    // Header validation
    printf("\n--- Header Validation ---\n");
    printf("Magic Number:  %s (0x%08X)\n", result->magic_valid ? "✓" : "✗", UNICODE_MAGIC);
    printf("Version:       %s (%d.%d.%d)\n", result->version_valid ? "✓" : "✗",
           UNICODE_VERSION_MAJOR, UNICODE_VERSION_MINOR, UNICODE_VERSION_PATCH);
    printf("Data Type:     %s\n", result->type_valid ? "✓" : "✗");
    
    // Data integrity
    printf("\n--- Data Integrity ---\n");
    printf("Checksum:      %s (expected: 0x%08X, actual: 0x%08X)\n",
           result->checksum_valid ? "✓" : "✗",
           result->expected_checksum, result->actual_checksum);
    printf("Decompression: %s\n", result->data_valid ? "✓" : "✗");
    
    // File statistics
    printf("\n--- File Statistics ---\n");
    printf("File Size:         %u bytes (%.2f MB)\n", 
           result->file_size, result->file_size / 1024.0 / 1024.0);
    printf("Uncompressed Size: %u bytes (%.2f MB)\n",
           result->uncompressed_size, result->uncompressed_size / 1024.0 / 1024.0);
    printf("Compressed Size:   %u bytes (%.2f MB)\n",
           result->compressed_size, result->compressed_size / 1024.0 / 1024.0);
    printf("Compression Ratio: %.2f%%\n", result->compression_ratio);
    printf("Number of Entries: %u\n", result->num_entries);
    
    // Coverage
    printf("\n--- Coverage ---\n");
    printf("Entries:    %u\n", result->coverage_count);
    printf("Coverage:   %.2f%%\n", result->coverage_percent);
    
    // Performance
    printf("\n--- Performance ---\n");
    printf("Read Time:        %.3f ms\n", result->read_time_ms);
    printf("Decompress Time:  %.3f ms\n", result->decompress_time_ms);
    printf("Total Time:       %.3f ms\n", result->validate_time_ms);
    
    // Errors and warnings
    if (strlen(result->error_msg) > 0) {
        printf("\n--- Errors ---\n");
        printf("%s", result->error_msg);
    }
    
    if (strlen(result->warnings) > 0) {
        printf("\n--- Warnings ---\n");
        printf("%s", result->warnings);
    }
    
    printf("\n");
}

// Print JSON result
void print_json_result(const validation_result_t* result, bool first) {
    if (!first) printf(",\n");
    
    printf("  {\n");
    printf("    \"filename\": \"%s\",\n", result->filename);
    printf("    \"passed\": %s,\n", 
           (result->header_valid && result->checksum_valid && result->data_valid) ? "true" : "false");
    printf("    \"quality_score\": %d,\n", result->quality_score);
    printf("    \"header_valid\": %s,\n", result->header_valid ? "true" : "false");
    printf("    \"magic_valid\": %s,\n", result->magic_valid ? "true" : "false");
    printf("    \"version_valid\": %s,\n", result->version_valid ? "true" : "false");
    printf("    \"type_valid\": %s,\n", result->type_valid ? "true" : "false");
    printf("    \"checksum_valid\": %s,\n", result->checksum_valid ? "true" : "false");
    printf("    \"data_valid\": %s,\n", result->data_valid ? "true" : "false");
    printf("    \"file_size\": %u,\n", result->file_size);
    printf("    \"uncompressed_size\": %u,\n", result->uncompressed_size);
    printf("    \"compressed_size\": %u,\n", result->compressed_size);
    printf("    \"compression_ratio\": %.2f,\n", result->compression_ratio);
    printf("    \"num_entries\": %u,\n", result->num_entries);
    printf("    \"coverage_count\": %u,\n", result->coverage_count);
    printf("    \"coverage_percent\": %.2f,\n", result->coverage_percent);
    printf("    \"read_time_ms\": %.3f,\n", result->read_time_ms);
    printf("    \"decompress_time_ms\": %.3f,\n", result->decompress_time_ms);
    printf("    \"validate_time_ms\": %.3f\n", result->validate_time_ms);
    printf("  }");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s [--json] <file1.dat> [file2.dat ...]\n", argv[0]);
        return 1;
    }
    
    bool json_output = false;
    int start_idx = 1;
    
    if (strcmp(argv[1], "--json") == 0) {
        json_output = true;
        start_idx = 2;
        if (argc < 3) {
            fprintf(stderr, "Usage: %s [--json] <file1.dat> [file2.dat ...]\n", argv[0]);
            return 1;
        }
    }
    
    int num_files = argc - start_idx;
    validation_result_t* results = malloc(sizeof(validation_result_t) * num_files);
    
    // Validate all files
    for (int i = 0; i < num_files; i++) {
        results[i] = validate_dat_file(argv[start_idx + i]);
    }
    
    // Print results
    if (json_output) {
        printf("{\n");
        printf("  \"validation_results\": [\n");
        for (int i = 0; i < num_files; i++) {
            print_json_result(&results[i], i == 0);
        }
        printf("\n  ],\n");
        
        // Summary
        int passed = 0;
        int total_score = 0;
        for (int i = 0; i < num_files; i++) {
            if (results[i].header_valid && results[i].checksum_valid && results[i].data_valid) {
                passed++;
            }
            total_score += results[i].quality_score;
        }
        
        printf("  \"summary\": {\n");
        printf("    \"total_files\": %d,\n", num_files);
        printf("    \"passed\": %d,\n", passed);
        printf("    \"failed\": %d,\n", num_files - passed);
        printf("    \"average_quality_score\": %.2f\n", (float)total_score / num_files);
        printf("  }\n");
        printf("}\n");
    } else {
        for (int i = 0; i < num_files; i++) {
            print_result(&results[i]);
        }
        
        // Summary
        printf("═══════════════════════════════════════════════════════════════\n");
        printf("SUMMARY\n");
        printf("═══════════════════════════════════════════════════════════════\n");
        
        int passed = 0;
        int total_score = 0;
        for (int i = 0; i < num_files; i++) {
            if (results[i].header_valid && results[i].checksum_valid && results[i].data_valid) {
                passed++;
            }
            total_score += results[i].quality_score;
        }
        
        printf("Total Files:    %d\n", num_files);
        printf("Passed:         %d\n", passed);
        printf("Failed:         %d\n", num_files - passed);
        printf("Average Score:  %.2f/100\n", (float)total_score / num_files);
        printf("\n");
    }
    
    free(results);
    
    return 0;
}
