
// BCI Conversion Utilities - Binary/Decimal/Hex conversions
#ifndef BCI_CONVERSION_H
#define BCI_CONVERSION_H

#include "../c23_compat.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Binary to decimal string conversion
// Returns number of characters written (excluding null terminator)
size_t binary_to_decimal_str(uint64_t value, char *buffer, size_t buffer_size);

// Decimal string to binary conversion
// Returns true on success, false on parse error
NODISCARD bool decimal_str_to_binary(const char *str, uint64_t *out_value);

// Binary to hexadecimal string conversion
size_t binary_to_hex_str(uint64_t value, char *buffer, size_t buffer_size, bool uppercase);

// Hexadecimal string to binary conversion
NODISCARD bool hex_str_to_binary(const char *str, uint64_t *out_value);

// Binary to binary string representation (e.g., "0b10101010")
size_t binary_to_binstr(uint64_t value, char *buffer, size_t buffer_size, int bits);

// Binary string to binary conversion (e.g., "10101010" or "0b10101010")
NODISCARD bool binstr_to_binary(const char *str, uint64_t *out_value);

// Octal conversions
size_t binary_to_octal_str(uint64_t value, char *buffer, size_t buffer_size);
NODISCARD bool octal_str_to_binary(const char *str, uint64_t *out_value);

// Format binary with separators for readability (e.g., "1010_1010")
size_t binary_to_binstr_formatted(uint64_t value, char *buffer, size_t buffer_size, 
                                   int bits, char separator, int group_size);

#endif // BCI_CONVERSION_H
