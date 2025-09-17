
// ===================================================================
// DESC: Multi-Base Hybrid (MBH) Arithmetic implementation for BDI
//       Provides arbitrary precision arithmetic with multiple base support
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// MBH Constants
#define MBH_MAX_DIGITS      1024
#define MBH_DEFAULT_BASE    10
#define MBH_MAX_BASE        36
#define MBH_MIN_BASE        2

// MBH Number structure
typedef struct {
    uint8_t digits[MBH_MAX_DIGITS];     // Digit array (least significant first)
    uint32_t length;                    // Number of significant digits
    uint32_t base;                      // Number base (2-36)
    int8_t sign;                        // Sign: 1 for positive, -1 for negative
    uint32_t decimal_point;             // Position of decimal point (0 = integer)
} mbh_number_t;

// Function prototypes
int mbh_init(mbh_number_t *num, uint32_t base);
int mbh_from_string(mbh_number_t *num, const char *str, uint32_t base);
int mbh_from_int(mbh_number_t *num, int64_t value, uint32_t base);
char *mbh_to_string(const mbh_number_t *num);
int mbh_copy(mbh_number_t *dest, const mbh_number_t *src);
int mbh_compare(const mbh_number_t *a, const mbh_number_t *b);
int mbh_add(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b);
int mbh_subtract(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b);
int mbh_multiply(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b);
int mbh_divide(mbh_number_t *quotient, mbh_number_t *remainder, 
               const mbh_number_t *dividend, const mbh_number_t *divisor);
int mbh_power(mbh_number_t *result, const mbh_number_t *base, const mbh_number_t *exponent);
int mbh_convert_base(mbh_number_t *result, const mbh_number_t *num, uint32_t new_base);
void mbh_normalize(mbh_number_t *num);
uint8_t char_to_digit(char c);
char digit_to_char(uint8_t digit);

/**
 * Initialize an MBH number
 */
int mbh_init(mbh_number_t *num, uint32_t base) {
    if (!num || base < MBH_MIN_BASE || base > MBH_MAX_BASE) {
        return -1;
    }
    
    memset(num->digits, 0, MBH_MAX_DIGITS);
    num->length = 1;
    num->base = base;
    num->sign = 1;
    num->decimal_point = 0;
    
    return 0;
}

/**
 * Create MBH number from string
 */
int mbh_from_string(mbh_number_t *num, const char *str, uint32_t base) {
    if (!num || !str || base < MBH_MIN_BASE || base > MBH_MAX_BASE) {
        return -1;
    }
    
    mbh_init(num, base);
    
    const char *ptr = str;
    
    // Handle sign
    if (*ptr == '-') {
        num->sign = -1;
        ptr++;
    } else if (*ptr == '+') {
        ptr++;
    }
    
    // Find decimal point
    const char *decimal_pos = strchr(ptr, '.');
    uint32_t decimal_offset = 0;
    
    if (decimal_pos) {
        decimal_offset = strlen(decimal_pos) - 1;
        num->decimal_point = decimal_offset;
    }
    
    // Parse digits
    uint32_t digit_count = 0;
    while (*ptr && digit_count < MBH_MAX_DIGITS) {
        if (*ptr == '.') {
            ptr++;
            continue;
        }
        
        uint8_t digit = char_to_digit(*ptr);
        if (digit >= base) {
            return -1; // Invalid digit for base
        }
        
        // Store digits in reverse order (least significant first)
        num->digits[digit_count] = digit;
        digit_count++;
        ptr++;
    }
    
    // Reverse the digits to correct order
    for (uint32_t i = 0; i < digit_count / 2; i++) {
        uint8_t temp = num->digits[i];
        num->digits[i] = num->digits[digit_count - 1 - i];
        num->digits[digit_count - 1 - i] = temp;
    }
    
    num->length = digit_count;
    mbh_normalize(num);
    
    return 0;
}

/**
 * Create MBH number from integer
 */
int mbh_from_int(mbh_number_t *num, int64_t value, uint32_t base) {
    if (!num || base < MBH_MIN_BASE || base > MBH_MAX_BASE) {
        return -1;
    }
    
    mbh_init(num, base);
    
    if (value < 0) {
        num->sign = -1;
        value = -value;
    }
    
    if (value == 0) {
        return 0;
    }
    
    uint32_t digit_count = 0;
    while (value > 0 && digit_count < MBH_MAX_DIGITS) {
        num->digits[digit_count] = value % base;
        value /= base;
        digit_count++;
    }
    
    num->length = digit_count;
    return 0;
}

/**
 * Convert MBH number to string
 */
char *mbh_to_string(const mbh_number_t *num) {
    if (!num) {
        return NULL;
    }
    
    // Calculate required buffer size
    uint32_t buffer_size = num->length + 10; // Extra space for sign, decimal, null terminator
    char *result = (char *)malloc(buffer_size);
    if (!result) {
        return NULL;
    }
    
    char *ptr = result;
    
    // Add sign
    if (num->sign < 0) {
        *ptr++ = '-';
    }
    
    // Add digits
    for (int32_t i = num->length - 1; i >= 0; i--) {
        if (num->decimal_point > 0 && i == (int32_t)(num->length - num->decimal_point - 1)) {
            *ptr++ = '.';
        }
        *ptr++ = digit_to_char(num->digits[i]);
    }
    
    *ptr = '\0';
    return result;
}

/**
 * Copy MBH number
 */
int mbh_copy(mbh_number_t *dest, const mbh_number_t *src) {
    if (!dest || !src) {
        return -1;
    }
    
    memcpy(dest->digits, src->digits, MBH_MAX_DIGITS);
    dest->length = src->length;
    dest->base = src->base;
    dest->sign = src->sign;
    dest->decimal_point = src->decimal_point;
    
    return 0;
}

/**
 * Compare two MBH numbers
 */
int mbh_compare(const mbh_number_t *a, const mbh_number_t *b) {
    if (!a || !b) {
        return 0;
    }
    
    // Different signs
    if (a->sign != b->sign) {
        return a->sign > b->sign ? 1 : -1;
    }
    
    // Same sign, compare magnitudes
    if (a->length != b->length) {
        int result = a->length > b->length ? 1 : -1;
        return a->sign > 0 ? result : -result;
    }
    
    // Same length, compare digit by digit
    for (int32_t i = a->length - 1; i >= 0; i--) {
        if (a->digits[i] != b->digits[i]) {
            int result = a->digits[i] > b->digits[i] ? 1 : -1;
            return a->sign > 0 ? result : -result;
        }
    }
    
    return 0; // Equal
}

/**
 * Add two MBH numbers
 */
int mbh_add(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b) {
    if (!result || !a || !b || a->base != b->base) {
        return -1;
    }
    
    // Handle different signs
    if (a->sign != b->sign) {
        mbh_number_t temp_b;
        mbh_copy(&temp_b, b);
        temp_b.sign = -temp_b.sign;
        return mbh_subtract(result, a, &temp_b);
    }
    
    mbh_init(result, a->base);
    result->sign = a->sign;
    
    uint32_t max_len = a->length > b->length ? a->length : b->length;
    uint32_t carry = 0;
    
    for (uint32_t i = 0; i < max_len || carry; i++) {
        if (i >= MBH_MAX_DIGITS) {
            return -1; // Overflow
        }
        
        uint32_t sum = carry;
        if (i < a->length) sum += a->digits[i];
        if (i < b->length) sum += b->digits[i];
        
        result->digits[i] = sum % a->base;
        carry = sum / a->base;
        result->length = i + 1;
    }
    
    mbh_normalize(result);
    return 0;
}

/**
 * Subtract two MBH numbers
 */
int mbh_subtract(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b) {
    if (!result || !a || !b || a->base != b->base) {
        return -1;
    }
    
    // Handle different signs
    if (a->sign != b->sign) {
        mbh_number_t temp_b;
        mbh_copy(&temp_b, b);
        temp_b.sign = -temp_b.sign;
        return mbh_add(result, a, &temp_b);
    }
    
    // Ensure a >= b for subtraction
    const mbh_number_t *minuend = a;
    const mbh_number_t *subtrahend = b;
    int result_sign = a->sign;
    
    if (mbh_compare(a, b) < 0) {
        minuend = b;
        subtrahend = a;
        result_sign = -result_sign;
    }
    
    mbh_init(result, a->base);
    result->sign = result_sign;
    
    int32_t borrow = 0;
    for (uint32_t i = 0; i < minuend->length; i++) {
        int32_t diff = minuend->digits[i] - borrow;
        if (i < subtrahend->length) {
            diff -= subtrahend->digits[i];
        }
        
        if (diff < 0) {
            diff += a->base;
            borrow = 1;
        } else {
            borrow = 0;
        }
        
        result->digits[i] = diff;
        result->length = i + 1;
    }
    
    mbh_normalize(result);
    return 0;
}

/**
 * Multiply two MBH numbers
 */
int mbh_multiply(mbh_number_t *result, const mbh_number_t *a, const mbh_number_t *b) {
    if (!result || !a || !b || a->base != b->base) {
        return -1;
    }
    
    mbh_init(result, a->base);
    result->sign = a->sign * b->sign;
    
    if (a->length + b->length > MBH_MAX_DIGITS) {
        return -1; // Overflow
    }
    
    // Grade school multiplication
    for (uint32_t i = 0; i < a->length; i++) {
        uint32_t carry = 0;
        for (uint32_t j = 0; j < b->length || carry; j++) {
            uint32_t pos = i + j;
            if (pos >= MBH_MAX_DIGITS) {
                return -1; // Overflow
            }
            
            uint32_t product = result->digits[pos] + carry;
            if (j < b->length) {
                product += a->digits[i] * b->digits[j];
            }
            
            result->digits[pos] = product % a->base;
            carry = product / a->base;
            
            if (pos >= result->length) {
                result->length = pos + 1;
            }
        }
    }
    
    mbh_normalize(result);
    return 0;
}

/**
 * Convert character to digit
 */
uint8_t char_to_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'Z') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'z') {
        return c - 'a' + 10;
    }
    return 255; // Invalid
}

/**
 * Convert digit to character
 */
char digit_to_char(uint8_t digit) {
    if (digit < 10) {
        return '0' + digit;
    } else if (digit < 36) {
        return 'A' + digit - 10;
    }
    return '?'; // Invalid
}

/**
 * Normalize MBH number (remove leading zeros)
 */
void mbh_normalize(mbh_number_t *num) {
    if (!num) {
        return;
    }
    
    // Remove leading zeros
    while (num->length > 1 && num->digits[num->length - 1] == 0) {
        num->length--;
    }
    
    // Handle zero case
    if (num->length == 1 && num->digits[0] == 0) {
        num->sign = 1;
        num->decimal_point = 0;
    }
}

/**
 * Convert between bases
 */
int mbh_convert_base(mbh_number_t *result, const mbh_number_t *num, uint32_t new_base) {
    if (!result || !num || new_base < MBH_MIN_BASE || new_base > MBH_MAX_BASE) {
        return -1;
    }
    
    // For now, convert through decimal representation
    // A more efficient implementation would use direct base conversion
    char *str = mbh_to_string(num);
    if (!str) {
        return -1;
    }
    
    // Convert to base 10 first, then to target base
    mbh_number_t temp;
    if (mbh_from_string(&temp, str, num->base) != 0) {
        free(str);
        return -1;
    }
    
    free(str);
    
    // Simple base conversion (placeholder implementation)
    mbh_copy(result, &temp);
    result->base = new_base;
    
    return 0;
}
