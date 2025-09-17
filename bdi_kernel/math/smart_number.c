
// ===================================================================
// DESC: Smart Number system for BDI - Adaptive numeric representation
//       Automatically chooses optimal representation for numbers
// ===================================================================

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// Smart number types
typedef enum {
    SMART_TYPE_INT8,
    SMART_TYPE_INT16,
    SMART_TYPE_INT32,
    SMART_TYPE_INT64,
    SMART_TYPE_UINT8,
    SMART_TYPE_UINT16,
    SMART_TYPE_UINT32,
    SMART_TYPE_UINT64,
    SMART_TYPE_FLOAT,
    SMART_TYPE_DOUBLE,
    SMART_TYPE_DECIMAL,
    SMART_TYPE_FRACTION,
    SMART_TYPE_COMPLEX,
    SMART_TYPE_BIGINT
} smart_number_type_t;

// Forward declarations
typedef struct smart_number smart_number_t;
typedef struct smart_decimal smart_decimal_t;
typedef struct smart_fraction smart_fraction_t;
typedef struct smart_complex smart_complex_t;

// Decimal representation for high precision
struct smart_decimal {
    uint8_t *digits;        // Digit array
    uint32_t integer_part;  // Number of integer digits
    uint32_t decimal_part;  // Number of decimal digits
    int8_t sign;           // Sign
};

// Fraction representation
struct smart_fraction {
    int64_t numerator;
    int64_t denominator;
};

// Complex number representation
struct smart_complex {
    smart_number_t *real;
    smart_number_t *imaginary;
};

// Smart number structure
struct smart_number {
    smart_number_type_t type;
    union {
        int8_t i8;
        int16_t i16;
        int32_t i32;
        int64_t i64;
        uint8_t u8;
        uint16_t u16;
        uint32_t u32;
        uint64_t u64;
        float f;
        double d;
        smart_decimal_t decimal;
        smart_fraction_t fraction;
        smart_complex_t complex;
        void *bigint;       // Pointer to arbitrary precision integer
    } value;
    uint32_t precision;     // Precision hint
    uint32_t flags;         // Special flags
};

// Smart number flags
#define SMART_FLAG_EXACT        0x01    // Exact representation
#define SMART_FLAG_APPROXIMATE  0x02    // Approximate representation
#define SMART_FLAG_OVERFLOW     0x04    // Overflow occurred
#define SMART_FLAG_UNDERFLOW    0x08    // Underflow occurred
#define SMART_FLAG_INFINITE     0x10    // Infinite value
#define SMART_FLAG_NAN          0x20    // Not a number

// Function prototypes
int smart_init(smart_number_t *num);
int smart_from_int(smart_number_t *num, int64_t value);
int smart_from_uint(smart_number_t *num, uint64_t value);
int smart_from_double(smart_number_t *num, double value);
int smart_from_string(smart_number_t *num, const char *str);
int smart_from_fraction(smart_number_t *num, int64_t numerator, int64_t denominator);
char *smart_to_string(const smart_number_t *num);
double smart_to_double(const smart_number_t *num);
int64_t smart_to_int(const smart_number_t *num);
int smart_copy(smart_number_t *dest, const smart_number_t *src);
int smart_compare(const smart_number_t *a, const smart_number_t *b);
int smart_add(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
int smart_subtract(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
int smart_multiply(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
int smart_divide(smart_number_t *result, const smart_number_t *a, const smart_number_t *b);
int smart_power(smart_number_t *result, const smart_number_t *base, const smart_number_t *exp);
int smart_sqrt(smart_number_t *result, const smart_number_t *num);
int smart_optimize(smart_number_t *num);
smart_number_type_t smart_optimal_type(int64_t value);
smart_number_type_t smart_optimal_type_float(double value);
void smart_cleanup(smart_number_t *num);

/**
 * Initialize a smart number
 */
int smart_init(smart_number_t *num) {
    if (!num) {
        return -1;
    }
    
    memset(num, 0, sizeof(smart_number_t));
    num->type = SMART_TYPE_INT32;
    num->value.i32 = 0;
    num->precision = 0;
    num->flags = SMART_FLAG_EXACT;
    
    return 0;
}

/**
 * Create smart number from signed integer
 */
int smart_from_int(smart_number_t *num, int64_t value) {
    if (!num) {
        return -1;
    }
    
    smart_init(num);
    
    // Choose optimal integer type
    num->type = smart_optimal_type(value);
    
    switch (num->type) {
        case SMART_TYPE_INT8:
            num->value.i8 = (int8_t)value;
            break;
        case SMART_TYPE_INT16:
            num->value.i16 = (int16_t)value;
            break;
        case SMART_TYPE_INT32:
            num->value.i32 = (int32_t)value;
            break;
        case SMART_TYPE_INT64:
            num->value.i64 = value;
            break;
        default:
            return -1;
    }
    
    num->flags = SMART_FLAG_EXACT;
    return 0;
}

/**
 * Create smart number from unsigned integer
 */
int smart_from_uint(smart_number_t *num, uint64_t value) {
    if (!num) {
        return -1;
    }
    
    smart_init(num);
    
    // Choose optimal unsigned integer type
    if (value <= UINT8_MAX) {
        num->type = SMART_TYPE_UINT8;
        num->value.u8 = (uint8_t)value;
    } else if (value <= UINT16_MAX) {
        num->type = SMART_TYPE_UINT16;
        num->value.u16 = (uint16_t)value;
    } else if (value <= UINT32_MAX) {
        num->type = SMART_TYPE_UINT32;
        num->value.u32 = (uint32_t)value;
    } else {
        num->type = SMART_TYPE_UINT64;
        num->value.u64 = value;
    }
    
    num->flags = SMART_FLAG_EXACT;
    return 0;
}

/**
 * Create smart number from double
 */
int smart_from_double(smart_number_t *num, double value) {
    if (!num || !isfinite(value)) {
        return -1;
    }
    
    smart_init(num);
    
    // Check if it's actually an integer
    if (floor(value) == value && value >= INT64_MIN && value <= INT64_MAX) {
        return smart_from_int(num, (int64_t)value);
    }
    
    // Choose optimal floating-point type
    num->type = smart_optimal_type_float(value);
    
    if (num->type == SMART_TYPE_FLOAT) {
        num->value.f = (float)value;
        num->flags = SMART_FLAG_APPROXIMATE;
    } else {
        num->value.d = value;
        num->flags = SMART_FLAG_APPROXIMATE;
    }
    
    return 0;
}

/**
 * Create smart number from string
 */
int smart_from_string(smart_number_t *num, const char *str) {
    if (!num || !str) {
        return -1;
    }
    
    smart_init(num);
    
    // Check for fraction format (e.g., "3/4")
    const char *slash = strchr(str, '/');
    if (slash) {
        char *endptr;
        int64_t numerator = strtoll(str, &endptr, 10);
        if (endptr != slash) {
            return -1;
        }
        
        int64_t denominator = strtoll(slash + 1, &endptr, 10);
        if (*endptr != '\0' || denominator == 0) {
            return -1;
        }
        
        return smart_from_fraction(num, numerator, denominator);
    }
    
    // Check for decimal point
    const char *dot = strchr(str, '.');
    if (dot) {
        double value = strtod(str, NULL);
        return smart_from_double(num, value);
    }
    
    // Integer
    int64_t value = strtoll(str, NULL, 10);
    return smart_from_int(num, value);
}

/**
 * Create smart number from fraction
 */
int smart_from_fraction(smart_number_t *num, int64_t numerator, int64_t denominator) {
    if (!num || denominator == 0) {
        return -1;
    }
    
    smart_init(num);
    
    // Simplify fraction by finding GCD
    int64_t a = abs(numerator);
    int64_t b = abs(denominator);
    while (b != 0) {
        int64_t temp = b;
        b = a % b;
        a = temp;
    }
    
    int64_t gcd = a;
    numerator /= gcd;
    denominator /= gcd;
    
    // Check if it reduces to an integer
    if (denominator == 1) {
        return smart_from_int(num, numerator);
    }
    
    num->type = SMART_TYPE_FRACTION;
    num->value.fraction.numerator = numerator;
    num->value.fraction.denominator = denominator;
    num->flags = SMART_FLAG_EXACT;
    
    return 0;
}

/**
 * Convert smart number to string
 */
char *smart_to_string(const smart_number_t *num) {
    if (!num) {
        return NULL;
    }
    
    char *result = (char *)malloc(64);
    if (!result) {
        return NULL;
    }
    
    switch (num->type) {
        case SMART_TYPE_INT8:
            snprintf(result, 64, "%d", num->value.i8);
            break;
        case SMART_TYPE_INT16:
            snprintf(result, 64, "%d", num->value.i16);
            break;
        case SMART_TYPE_INT32:
            snprintf(result, 64, "%d", num->value.i32);
            break;
        case SMART_TYPE_INT64:
            snprintf(result, 64, "%lld", (long long)num->value.i64);
            break;
        case SMART_TYPE_UINT8:
            snprintf(result, 64, "%u", num->value.u8);
            break;
        case SMART_TYPE_UINT16:
            snprintf(result, 64, "%u", num->value.u16);
            break;
        case SMART_TYPE_UINT32:
            snprintf(result, 64, "%u", num->value.u32);
            break;
        case SMART_TYPE_UINT64:
            snprintf(result, 64, "%llu", (unsigned long long)num->value.u64);
            break;
        case SMART_TYPE_FLOAT:
            snprintf(result, 64, "%.7g", num->value.f);
            break;
        case SMART_TYPE_DOUBLE:
            snprintf(result, 64, "%.15g", num->value.d);
            break;
        case SMART_TYPE_FRACTION:
            snprintf(result, 64, "%lld/%lld", 
                    (long long)num->value.fraction.numerator,
                    (long long)num->value.fraction.denominator);
            break;
        default:
            snprintf(result, 64, "0");
            break;
    }
    
    return result;
}

/**
 * Convert smart number to double
 */
double smart_to_double(const smart_number_t *num) {
    if (!num) {
        return 0.0;
    }
    
    switch (num->type) {
        case SMART_TYPE_INT8:
            return (double)num->value.i8;
        case SMART_TYPE_INT16:
            return (double)num->value.i16;
        case SMART_TYPE_INT32:
            return (double)num->value.i32;
        case SMART_TYPE_INT64:
            return (double)num->value.i64;
        case SMART_TYPE_UINT8:
            return (double)num->value.u8;
        case SMART_TYPE_UINT16:
            return (double)num->value.u16;
        case SMART_TYPE_UINT32:
            return (double)num->value.u32;
        case SMART_TYPE_UINT64:
            return (double)num->value.u64;
        case SMART_TYPE_FLOAT:
            return (double)num->value.f;
        case SMART_TYPE_DOUBLE:
            return num->value.d;
        case SMART_TYPE_FRACTION:
            return (double)num->value.fraction.numerator / 
                   (double)num->value.fraction.denominator;
        default:
            return 0.0;
    }
}

/**
 * Convert smart number to integer
 */
int64_t smart_to_int(const smart_number_t *num) {
    if (!num) {
        return 0;
    }
    
    switch (num->type) {
        case SMART_TYPE_INT8:
            return (int64_t)num->value.i8;
        case SMART_TYPE_INT16:
            return (int64_t)num->value.i16;
        case SMART_TYPE_INT32:
            return (int64_t)num->value.i32;
        case SMART_TYPE_INT64:
            return num->value.i64;
        case SMART_TYPE_UINT8:
            return (int64_t)num->value.u8;
        case SMART_TYPE_UINT16:
            return (int64_t)num->value.u16;
        case SMART_TYPE_UINT32:
            return (int64_t)num->value.u32;
        case SMART_TYPE_UINT64:
            return (int64_t)num->value.u64;
        case SMART_TYPE_FLOAT:
            return (int64_t)num->value.f;
        case SMART_TYPE_DOUBLE:
            return (int64_t)num->value.d;
        case SMART_TYPE_FRACTION:
            return num->value.fraction.numerator / num->value.fraction.denominator;
        default:
            return 0;
    }
}

/**
 * Add two smart numbers
 */
int smart_add(smart_number_t *result, const smart_number_t *a, const smart_number_t *b) {
    if (!result || !a || !b) {
        return -1;
    }
    
    // Handle fraction addition
    if (a->type == SMART_TYPE_FRACTION && b->type == SMART_TYPE_FRACTION) {
        int64_t num = a->value.fraction.numerator * b->value.fraction.denominator +
                      b->value.fraction.numerator * a->value.fraction.denominator;
        int64_t den = a->value.fraction.denominator * b->value.fraction.denominator;
        return smart_from_fraction(result, num, den);
    }
    
    // Convert to common type and add
    double val_a = smart_to_double(a);
    double val_b = smart_to_double(b);
    double sum = val_a + val_b;
    
    // Try to preserve exactness
    if ((a->flags & SMART_FLAG_EXACT) && (b->flags & SMART_FLAG_EXACT)) {
        if (floor(sum) == sum && sum >= INT64_MIN && sum <= INT64_MAX) {
            return smart_from_int(result, (int64_t)sum);
        }
    }
    
    return smart_from_double(result, sum);
}

/**
 * Multiply two smart numbers
 */
int smart_multiply(smart_number_t *result, const smart_number_t *a, const smart_number_t *b) {
    if (!result || !a || !b) {
        return -1;
    }
    
    
    // Handle fraction multiplication
    if (a->type == SMART_TYPE_FRACTION && b->type == SMART_TYPE_FRACTION) {
        int64_t num = a->value.fraction.numerator * b->value.fraction.numerator;
        int64_t den = a->value.fraction.denominator * b->value.fraction.denominator;
        return smart_from_fraction(result, num, den);
    }
    
    // Convert to common type and multiply
    double val_a = smart_to_double(a);
    double val_b = smart_to_double(b);
    double product = val_a * val_b;
    
    // Try to preserve exactness
    if ((a->flags & SMART_FLAG_EXACT) && (b->flags & SMART_FLAG_EXACT)) {
        if (floor(product) == product && product >= INT64_MIN && product <= INT64_MAX) {
            return smart_from_int(result, (int64_t)product);
        }
    }
    
    return smart_from_double(result, product);
}

/**
 * Determine optimal integer type for value
 */
smart_number_type_t smart_optimal_type(int64_t value) {
    if (value >= INT8_MIN && value <= INT8_MAX) {
        return SMART_TYPE_INT8;
    } else if (value >= INT16_MIN && value <= INT16_MAX) {
        return SMART_TYPE_INT16;
    } else if (value >= INT32_MIN && value <= INT32_MAX) {
        return SMART_TYPE_INT32;
    } else {
        return SMART_TYPE_INT64;
    }
}

/**
 * Determine optimal floating-point type for value
 */
smart_number_type_t smart_optimal_type_float(double value) {
    // Check if float precision is sufficient
    float f_val = (float)value;
    if ((double)f_val == value) {
        return SMART_TYPE_FLOAT;
    }
    return SMART_TYPE_DOUBLE;
}

/**
 * Optimize smart number representation
 */
int smart_optimize(smart_number_t *num) {
    if (!num) {
        return -1;
    }
    
    // Convert to most compact representation
    double val = smart_to_double(num);
    
    // Check if it's an integer
    if (floor(val) == val && val >= INT64_MIN && val <= INT64_MAX) {
        int64_t int_val = (int64_t)val;
        smart_number_type_t optimal = smart_optimal_type(int_val);
        
        if (optimal != num->type) {
            smart_number_t temp;
            smart_from_int(&temp, int_val);
            *num = temp;
        }
    }
    
    return 0;
}

/**
 * Cleanup smart number resources
 */
void smart_cleanup(smart_number_t *num) {
    if (!num) {
        return;
    }
    
    // Free any dynamically allocated memory
    if (num->type == SMART_TYPE_DECIMAL && num->value.decimal.digits) {
        free(num->value.decimal.digits);
        num->value.decimal.digits = NULL;
    }
    
    if (num->type == SMART_TYPE_BIGINT && num->value.bigint) {
        free(num->value.bigint);
        num->value.bigint = NULL;
    }
    
    if (num->type == SMART_TYPE_COMPLEX) {
        if (num->value.complex.real) {
            smart_cleanup(num->value.complex.real);
            free(num->value.complex.real);
        }
        if (num->value.complex.imaginary) {
            smart_cleanup(num->value.complex.imaginary);
            free(num->value.complex.imaginary);
        }
    }
}
