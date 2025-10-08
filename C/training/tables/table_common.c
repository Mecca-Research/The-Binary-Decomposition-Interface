
#include "../include/training_tables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define TABLE_MAGIC 0x42444954  // 'BDIT'
#define TABLE_VERSION 1

// CRC32 lookup table
static uint32_t crc32_table[256];
static bool crc32_initialized = false;

static void init_crc32_table(void) {
    if (crc32_initialized) return;
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_initialized = true;
}

uint32_t table_calculate_checksum(const void *data, size_t size) {
    init_crc32_table();
    
    uint32_t crc = 0xFFFFFFFF;
    const uint8_t *bytes = (const uint8_t *)data;
    
    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ bytes[i]) & 0xFF];
    }
    
    return ~crc;
}

int table_write_header(int fd, const table_header_t *header) {
    if (fd < 0 || !header) return -1;
    
    ssize_t written = write(fd, header, sizeof(table_header_t));
    if (written != sizeof(table_header_t)) {
        return -1;
    }
    
    return 0;
}

int table_read_header(int fd, table_header_t *header) {
    if (fd < 0 || !header) return -1;
    
    ssize_t bytes_read = read(fd, header, sizeof(table_header_t));
    if (bytes_read != sizeof(table_header_t)) {
        return -1;
    }
    
    if (header->magic != TABLE_MAGIC) {
        fprintf(stderr, "Invalid magic number: 0x%08X (expected 0x%08X)\n", 
                header->magic, TABLE_MAGIC);
        return -1;
    }
    
    if (header->version != TABLE_VERSION) {
        fprintf(stderr, "Unsupported version: %u (expected %u)\n", 
                header->version, TABLE_VERSION);
        return -1;
    }
    
    return 0;
}

int table_write_entry(int fd, const training_entry_t *entry) {
    if (fd < 0 || !entry) return -1;
    
    // Write fixed-size header
    ssize_t written = write(fd, entry, sizeof(training_entry_t));
    if (written != sizeof(training_entry_t)) {
        return -1;
    }
    
    // Write variable-size data
    size_t data_size = entry->input_size + entry->output_size;
    if (data_size > 0) {
        written = write(fd, entry->data, data_size);
        if (written != (ssize_t)data_size) {
            return -1;
        }
    }
    
    return 0;
}

int table_read_entry(int fd, training_entry_t **entry) {
    if (fd < 0 || !entry) return -1;
    
    // Read fixed-size header
    training_entry_t temp;
    ssize_t bytes_read = read(fd, &temp, sizeof(training_entry_t));
    if (bytes_read == 0) {
        return 0; // EOF
    }
    if (bytes_read != sizeof(training_entry_t)) {
        return -1;
    }
    
    // Allocate full entry
    size_t data_size = temp.input_size + temp.output_size;
    *entry = malloc(sizeof(training_entry_t) + data_size);
    if (!*entry) {
        return -1;
    }
    
    // Copy header
    memcpy(*entry, &temp, sizeof(training_entry_t));
    
    // Read variable-size data
    if (data_size > 0) {
        bytes_read = read(fd, (*entry)->data, data_size);
        if (bytes_read != (ssize_t)data_size) {
            free(*entry);
            *entry = NULL;
            return -1;
        }
    }
    
    return 1;
}

int table_validate_file(const char *filename) {
    if (!filename) return -1;
    
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    table_header_t header;
    if (table_read_header(fd, &header) < 0) {
        close(fd);
        return -1;
    }
    
    printf("Validating %s:\n", filename);
    printf("  Entry type: %u\n", header.entry_type);
    printf("  Num entries: %u\n", header.num_entries);
    printf("  Total size: %lu bytes\n", header.total_size);
    
    uint32_t count = 0;
    training_entry_t *entry = NULL;
    
    while (table_read_entry(fd, &entry) > 0) {
        count++;
        free(entry);
        entry = NULL;
    }
    
    close(fd);
    
    if (count != header.num_entries) {
        fprintf(stderr, "Entry count mismatch: found %u, expected %u\n", 
                count, header.num_entries);
        return -1;
    }
    
    printf("  Validation: PASSED (%u entries)\n", count);
    return 0;
}
