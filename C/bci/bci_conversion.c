
// BCI Conversion Utilities Implementation
#include "bci_conversion.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

// Binary to decimal string
size_t binary_to_decimal_str(uint64_t value, char *buffer, size_t buffer_size) {
    if (buffer_size == 0) return 0;
    int written = snprintf(buffer, buffer_size, "%llu", (unsigned long long)value);
    return (written > 0 && (size_t)written < buffer_size) ? (size_t)written : 0;
}

// Decimal string to binary
bool decimal_str_to_binary(const char *str, uint64_t *out_value) {
    if (!str || !out_value) return false;
    
    uint64_t result = 0;
    const char *p = str;
    
    // Skip leading whitespace
    while (isspace(*p)) p++;
    
    // Check for empty string
    if (*p == '\0') return false;
    
    // Parse digits
    while (*p) {
        if (!isdigit(*p)) return false;
        
        uint64_t digit = *p - '0';
        
        // Check for overflow
        if (result > (UINT64_MAX - digit) / 10) return false;
        
        result = result * 10 + digit;
        p++;
    }
    
    *out_value = result;
    return true;
}

// Binary to hexadecimal string
size_t binary_to_hex_str(uint64_t value, char *buffer, size_t buffer_size, bool uppercase) {
    if (buffer_size == 0) return 0;
    const char *format = uppercase ? "0x%llX" : "0x%llx";
    int written = snprintf(buffer, buffer_size, format, (unsigned long long)value);
    return (written > 0 && (size_t)written < buffer_size) ? (size_t)written : 0;
}

// Hexadecimal string to binary
bool hex_str_to_binary(const char *str, uint64_t *out_value) {
    if (!str || !out_value) return false;
    
    uint64_t result = 0;
    const char *p = str;
    
    // Skip leading whitespace
    while (isspace(*p)) p++;
    
    // Skip optional "0x" or "0X" prefix
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
    }
    
    // Check for empty string
    if (*p == '\0') return false;
    
    // Parse hex digits
    while (*p) {
        int digit;
        if (isdigit(*p)) {
            digit = *p - '0';
        } else if (*p >= 'a' && *p <= 'f') {
            digit = *p - 'a' + 10;
        } else if (*p >= 'A' && *p <= 'F') {
            digit = *p - 'A' + 10;
        } else {
            return false;
        }
        
        // Check for overflow
        if (result > (UINT64_MAX >> 4)) return false;
        
        result = (result << 4) | digit;
        p++;
    }
    
    *out_value = result;
    return true;
}

// Binary to binary string representation
size_t binary_to_binstr(uint64_t value, char *buffer, size_t buffer_size, int bits) {
    if (buffer_size < 3 || bits <= 0 || bits > 64) return 0;
    
    size_t pos = 0;
    buffer[pos++] = '0';
    buffer[pos++] = 'b';
    
    for (int i = bits - 1; i >= 0 && pos < buffer_size - 1; i--) {
        buffer[pos++] = (value & (1ULL << i)) ? '1' : '0';
    }
    
    buffer[pos] = '\0';
    return pos;
}

// Binary string to binary
bool binstr_to_binary(const char *str, uint64_t *out_value) {
    if (!str || !out_value) return false;
    
    uint64_t result = 0;
    const char *p = str;
    
    // Skip leading whitespace
    while (isspace(*p)) p++;
    
    // Skip optional "0b" or "0B" prefix
    if (p[0] == '0' && (p[1] == 'b' || p[1] == 'B')) {
        p += 2;
    }
    
    // Check for empty string
    if (*p == '\0') return false;
    
    // Parse binary digits
    int bit_count = 0;
    while (*p) {
        if (*p != '0' && *p != '1') return false;
        if (bit_count >= 64) return false; // Overflow
        
        result = (result << 1) | (*p - '0');
        bit_count++;
        p++;
    }
    
    *out_value = result;
    return true;
}

// Binary to octal string
size_t binary_to_octal_str(uint64_t value, char *buffer, size_t buffer_size) {
    if (buffer_size == 0) return 0;
    int written = snprintf(buffer, buffer_size, "0%llo", (unsigned long long)value);
    return (written > 0 && (size_t)written < buffer_size) ? (size_t)written : 0;
}

// Octal string to binary
bool octal_str_to_binary(const char *str, uint64_t *out_value) {
    if (!str || !out_value) return false;
    
    uint64_t result = 0;
    const char *p = str;
    
    // Skip leading whitespace
    while (isspace(*p)) p++;
    
    // Check for empty string before processing
    if (*p == '\0') return false;
    
    // Skip optional "0" prefix
    if (*p == '0') {
        p++;
        // If just "0", return 0
        if (*p == '\0') {
            *out_value = 0;
            return true;
        }
    }
    
    // Parse octal digits
    while (*p) {
        if (*p < '0' || *p > '7') return false;
        
        int digit = *p - '0';
        
        // Check for overflow
        if (result > (UINT64_MAX >> 3)) return false;
        
        result = (result << 3) | digit;
        p++;
    }
    
    *out_value = result;
    return true;
}

// Format binary with separators
size_t binary_to_binstr_formatted(uint64_t value, char *buffer, size_t buffer_size,
                                   int bits, char separator, int group_size) {
    if (buffer_size < 3 || bits <= 0 || bits > 64 || group_size <= 0) return 0;
    
    size_t pos = 0;
    buffer[pos++] = '0';
    buffer[pos++] = 'b';
    
    int bit_count = 0;
    for (int i = bits - 1; i >= 0 && pos < buffer_size - 1; i--) {
        if (bit_count > 0 && bit_count % group_size == 0 && pos < buffer_size - 1) {
            buffer[pos++] = separator;
        }
        buffer[pos++] = (value & (1ULL << i)) ? '1' : '0';
        bit_count++;
    }
    
    buffer[pos] = '\0';
    return pos;
}
