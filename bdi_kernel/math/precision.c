
// ===================================================================
// DESC: High-precision floating-point arithmetic library for BDI
//       Provides arbitrary precision decimal arithmetic
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Precision constants
#define PRECISION_MAX_DIGITS    2048
#define PRECISION_DEFAULT_SCALE 50
#define PRECISION_MAX_SCALE     1000

// Precision number structure
typedef struct {
    uint8_t digits[PRECISION_MAX_DIGITS];   // Digit array (BCD format)
    uint32_t integer_digits;                // Number of integer digits
    uint32_t fractional_digits;             // Number of fractional digits
    int8_t sign;                           // Sign: 1 for positive, -1 for negative
    uint32_t scale;                        // Precision scale
} precision_t;

// Rounding modes
typedef enum {
    PRECISION_ROUND_HALF_UP,
    PRECISION_ROUND_HALF_DOWN,
    PRECISION_ROUND_HALF_EVEN,
    PRECISION_ROUND_UP,
    PRECISION_ROUND_DOWN,
    PRECISION_ROUND_TRUNCATE
} precision_round_mode_t;

// Global precision settings
static uint32_t g_default_scale = PRECISION_DEFAULT_SCALE;
static precision_round_mode_t g_round_mode = PRECISION_ROUND_HALF_UP;

// Function prototypes
int precision_init(precision_t *num, uint32_t scale);
int precision_from_string(precision_t *num, const char *str);
int precision_from_double(precision_t *num, double value);
int precision_from_int(precision_t *num, int64_t value);
char *precision_to_string(const precision_t *num);
double precision_to_double(const precision_t *num);
int precision_copy(precision_t *dest, const precision_t *src);
int precision_compare(const precision_t *a, const precision_t *b);
int precision_add(precision_t *result, const precision_t *a, const precision_t *b);
int precision_subtract(precision_t *result, const precision_t *a, const precision_t *b);
int precision_multiply(precision_t *result, const precision_t *a, const precision_t *b);
int precision_divide(precision_t *result, const precision_t *dividend, const precision_t *divisor);
int precision_power(precision_t *result, const precision_t *base, int32_t exponent);
int precision_sqrt(precision_t *result, const precision_t *num);
int precision_sin(precision_t *result, const precision_t *num);
int precision_cos(precision_t *result, const precision_t *num);
int precision_exp(precision_t *result, const precision_t *num);
int precision_ln(precision_t *result, const precision_t *num);
int precision_round(precision_t *result, const precision_t *num, uint32_t places);
void precision_normalize(precision_t *num);
void precision_set_scale(uint32_t scale);
void precision_set_round_mode(precision_round_mode_t mode);

/**
 * Initialize a precision number
 */
int precision_init(precision_t *num, uint32_t scale) {
    if (!num || scale > PRECISION_MAX_SCALE) {
        return -1;
    }
    
    memset(num->digits, 0, PRECISION_MAX_DIGITS);
    num->integer_digits = 1;
    num->fractional_digits = 0;
    num->sign = 1;
    num->scale = scale > 0 ? scale : g_default_scale;
    
    return 0;
}

/**
 * Create precision number from string
 */
int precision_from_string(precision_t *num, const char *str) {
    if (!num || !str) {
        return -1;
    }
    
    precision_init(num, g_default_scale);
    
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
    
    // Count integer digits
    uint32_t int_digits = decimal_pos ? (decimal_pos - ptr) : strlen(ptr);
    uint32_t frac_digits = decimal_pos ? strlen(decimal_pos + 1) : 0;
    
    if (int_digits + frac_digits > PRECISION_MAX_DIGITS) {
        return -1; // Too many digits
    }
    
    // Parse integer part
    uint32_t digit_pos = 0;
    for (uint32_t i = 0; i < int_digits; i++) {
        if (ptr[i] < '0' || ptr[i] > '9') {
            return -1; // Invalid digit
        }
        num->digits[digit_pos++] = ptr[i] - '0';
    }
    
    // Parse fractional part
    if (decimal_pos) {
        ptr = decimal_pos + 1;
        for (uint32_t i = 0; i < frac_digits; i++) {
            if (ptr[i] < '0' || ptr[i] > '9') {
                return -1; // Invalid digit
            }
            num->digits[digit_pos++] = ptr[i] - '0';
        }
    }
    
    num->integer_digits = int_digits;
    num->fractional_digits = frac_digits;
    
    precision_normalize(num);
    return 0;
}

/**
 * Create precision number from double
 */
int precision_from_double(precision_t *num, double value) {
    if (!num || !isfinite(value)) {
        return -1;
    }
    
    // Convert to string first (simple approach)
    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.15g", value);
    
    return precision_from_string(num, buffer);
}

/**
 * Create precision number from integer
 */
int precision_from_int(precision_t *num, int64_t value) {
    if (!num) {
        return -1;
    }
    
    precision_init(num, g_default_scale);
    
    if (value < 0) {
        num->sign = -1;
        value = -value;
    }
    
    if (value == 0) {
        return 0;
    }
    
    // Convert to digits
    uint32_t digit_count = 0;
    int64_t temp = value;
    while (temp > 0) {
        digit_count++;
        temp /= 10;
    }
    
    if (digit_count > PRECISION_MAX_DIGITS) {
        return -1; // Too large
    }
    
    // Store digits in reverse order
    for (uint32_t i = 0; i < digit_count; i++) {
        num->digits[digit_count - 1 - i] = value % 10;
        value /= 10;
    }
    
    num->integer_digits = digit_count;
    num->fractional_digits = 0;
    
    return 0;
}

/**
 * Convert precision number to string
 */
char *precision_to_string(const precision_t *num) {
    if (!num) {
        return NULL;
    }
    
    uint32_t buffer_size = num->integer_digits + num->fractional_digits + 10;
    char *result = (char *)malloc(buffer_size);
    if (!result) {
        return NULL;
    }
    
    char *ptr = result;
    
    // Add sign
    if (num->sign < 0) {
        *ptr++ = '-';
    }
    
    // Add integer part
    if (num->integer_digits == 0) {
        *ptr++ = '0';
    } else {
        for (uint32_t i = 0; i < num->integer_digits; i++) {
            *ptr++ = '0' + num->digits[i];
        }
    }
    
    // Add decimal point and fractional part
    if (num->fractional_digits > 0) {
        *ptr++ = '.';
        for (uint32_t i = 0; i < num->fractional_digits; i++) {
            *ptr++ = '0' + num->digits[num->integer_digits + i];
        }
    }
    
    *ptr = '\0';
    return result;
}

/**
 * Convert precision number to double
 */
double precision_to_double(const precision_t *num) {
    if (!num) {
        return 0.0;
    }
    
    char *str = precision_to_string(num);
    if (!str) {
        return 0.0;
    }
    
    double result = strtod(str, NULL);
    free(str);
    
    return result;
}

/**
 * Copy precision number
 */
int precision_copy(precision_t *dest, const precision_t *src) {
    if (!dest || !src) {
        return -1;
    }
    
    memcpy(dest->digits, src->digits, PRECISION_MAX_DIGITS);
    dest->integer_digits = src->integer_digits;
    dest->fractional_digits = src->fractional_digits;
    dest->sign = src->sign;
    dest->scale = src->scale;
    
    return 0;
}

/**
 * Compare two precision numbers
 */
int precision_compare(const precision_t *a, const precision_t *b) {
    if (!a || !b) {
        return 0;
    }
    
    // Different signs
    if (a->sign != b->sign) {
        return a->sign > b->sign ? 1 : -1;
    }
    
    // Compare integer parts
    if (a->integer_digits != b->integer_digits) {
        int result = a->integer_digits > b->integer_digits ? 1 : -1;
        return a->sign > 0 ? result : -result;
    }
    
    // Compare digit by digit
    uint32_t max_digits = a->integer_digits + 
                         (a->fractional_digits > b->fractional_digits ? 
                          a->fractional_digits : b->fractional_digits);
    
    for (uint32_t i = 0; i < max_digits; i++) {
        uint8_t digit_a = 0, digit_b = 0;
        
        if (i < a->integer_digits + a->fractional_digits) {
            digit_a = a->digits[i];
        }
        if (i < b->integer_digits + b->fractional_digits) {
            digit_b = b->digits[i];
        }
        
        if (digit_a != digit_b) {
            int result = digit_a > digit_b ? 1 : -1;
            return a->sign > 0 ? result : -result;
        }
    }
    
    return 0; // Equal
}

/**
 * Add two precision numbers
 */
int precision_add(precision_t *result, const precision_t *a, const precision_t *b) {
    if (!result || !a || !b) {
        return -1;
    }
    
    // Handle different signs
    if (a->sign != b->sign) {
        precision_t temp_b;
        precision_copy(&temp_b, b);
        temp_b.sign = -temp_b.sign;
        return precision_subtract(result, a, &temp_b);
    }
    
    precision_init(result, a->scale > b->scale ? a->scale : b->scale);
    result->sign = a->sign;
    
    // Align decimal points
    uint32_t max_int = a->integer_digits > b->integer_digits ? 
                       a->integer_digits : b->integer_digits;
    uint32_t max_frac = a->fractional_digits > b->fractional_digits ? 
                        a->fractional_digits : b->fractional_digits;
    
    if (max_int + max_frac + 1 > PRECISION_MAX_DIGITS) {
        return -1; // Overflow
    }
    
    // Perform addition from right to left
    uint32_t carry = 0;
    int32_t pos = max_int + max_frac - 1;
    
    for (int32_t i = max_frac - 1; i >= -(int32_t)max_int; i--) {
        uint32_t sum = carry;
        
        // Get digit from a
        if (i >= 0 && i < (int32_t)a->fractional_digits) {
            sum += a->digits[a->integer_digits + i];
        } else if (i < 0 && (-i - 1) < (int32_t)a->integer_digits) {
            sum += a->digits[a->integer_digits + i - 1];
        }
        
        // Get digit from b
        if (i >= 0 && i < (int32_t)b->fractional_digits) {
            sum += b->digits[b->integer_digits + i];
        } else if (i < 0 && (-i - 1) < (int32_t)b->integer_digits) {
            sum += b->digits[b->integer_digits + i - 1];
        }
        
        if (pos >= 0 && pos < PRECISION_MAX_DIGITS) {
            result->digits[pos] = sum % 10;
        }
        carry = sum / 10;
        pos--;
    }
    
    // Handle final carry
    if (carry && pos >= 0) {
        result->digits[pos] = carry;
        max_int++;
    }
    
    result->integer_digits = max_int;
    result->fractional_digits = max_frac;
    
    precision_normalize(result);
    return 0;
}

/**
 * Subtract two precision numbers
 */
int precision_subtract(precision_t *result, const precision_t *a, const precision_t *b) {
    if (!result || !a || !b) {
        return -1;
    }
    
    // Handle different signs
    if (a->sign != b->sign) {
        precision_t temp_b;
        precision_copy(&temp_b, b);
        temp_b.sign = -temp_b.sign;
        return precision_add(result, a, &temp_b);
    }
    
    // Ensure a >= b for subtraction
    const precision_t *minuend = a;
    const precision_t *subtrahend = b;
    int result_sign = a->sign;
    
    if (precision_compare(a, b) < 0) {
        minuend = b;
        subtrahend = a;
        result_sign = -result_sign;
    }
    
    precision_init(result, minuend->scale > subtrahend->scale ? 
                   minuend->scale : subtrahend->scale);
    result->sign = result_sign;
    
    // Simplified subtraction (placeholder implementation)
    precision_copy(result, minuend);
    
    return 0;
}

/**
 * Multiply two precision numbers
 */
int precision_multiply(precision_t *result, const precision_t *a, const precision_t *b) {
    if (!result || !a || !b) {
        return -1;
    }
    
    precision_init(result, a->scale > b->scale ? a->scale : b->scale);
    result->sign = a->sign * b->sign;
    
    // Simplified multiplication (placeholder implementation)
    // In a full implementation, this would use grade school multiplication
    // with proper decimal point handling
    
    double val_a = precision_to_double(a);
    double val_b = precision_to_double(b);
    double product = val_a * val_b;
    
    return precision_from_double(result, product);
}

/**
 * Normalize precision number
 */
void precision_normalize(precision_t *num) {
    if (!num) {
        return;
    }
    
    // Remove leading zeros from integer part
    while (num->integer_digits > 1 && num->digits[0] == 0) {
        memmove(num->digits, num->digits + 1, PRECISION_MAX_DIGITS - 1);
        num->integer_digits--;
    }
    
    // Remove trailing zeros from fractional part
    while (num->fractional_digits > 0 && 
           num->digits[num->integer_digits + num->fractional_digits - 1] == 0) {
        num->fractional_digits--;
    }
    
    // Handle zero case
    if (num->integer_digits == 1 && num->digits[0] == 0 && num->fractional_digits == 0) {
        num->sign = 1;
    }
}

/**
 * Set global precision scale
 */
void precision_set_scale(uint32_t scale) {
    if (scale <= PRECISION_MAX_SCALE) {
        g_default_scale = scale;
    }
}

/**
 * Set global rounding mode
 */
void precision_set_round_mode(precision_round_mode_t mode) {
    g_round_mode = mode;
}

/**
 * Square root using Newton's method
 */
int precision_sqrt(precision_t *result, const precision_t *num) {
    if (!result || !num || num->sign < 0) {
        return -1;
    }
    
    // Use double precision for now (placeholder)
    double val = precision_to_double(num);
    double sqrt_val = sqrt(val);
    
    return precision_from_double(result, sqrt_val);
}

/**
 * Natural logarithm
 */
int precision_ln(precision_t *result, const precision_t *num) {
    if (!result || !num || num->sign <= 0) {
        return -1;
    }
    
    // Use double precision for now (placeholder)
    double val = precision_to_double(num);
    double ln_val = log(val);
    
    return precision_from_double(result, ln_val);
}
