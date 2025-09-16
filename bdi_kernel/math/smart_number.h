
// ===================================================================
// DESC: Smart Number Library - M→B→H arithmetic with precision management
// ===================================================================
#ifndef AEON_SMART_NUMBER_H
#define AEON_SMART_NUMBER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// --- Smart Number Representation ---
typedef enum {
    SMART_NUM_MACHINE = 0,      // Machine representation (binary)
    SMART_NUM_BINARY = 1,       // Binary decomposition
    SMART_NUM_HUMAN = 2         // Human-readable representation
} smart_num_repr_t;

// --- Precision Levels ---
typedef enum {
    PRECISION_LOW = 8,          // 8-bit precision
    PRECISION_MEDIUM = 16,      // 16-bit precision
    PRECISION_HIGH = 32,        // 32-bit precision
    PRECISION_ULTRA = 64,       // 64-bit precision
    PRECISION_ARBITRARY = 0     // Arbitrary precision
} precision_level_t;

// --- Smart Number Structure ---
typedef struct {
    // Core data
    void* data;                 // Actual numeric data
    size_t data_size;           // Size of data in bytes
    
    // Representation metadata
    smart_num_repr_t repr;      // Current representation
    precision_level_t precision; // Precision level
    
    // Binary decomposition
    uint8_t* binary_decomp;     // Binary decomposition data
    size_t decomp_size;         // Size of decomposition
    uint32_t decomp_levels;     // Number of decomposition levels
    
    // Human representation
    char* human_repr;           // Human-readable string
    size_t human_size;          // Size of human representation
    
    // Metadata
    bool is_signed;             // Signed or unsigned
    bool is_integer;            // Integer or floating-point
    bool is_normalized;         // Normalized representation
    uint32_t exponent_bias;     // Exponent bias for floating-point
    
    // Precision tracking
    uint32_t significant_bits;  // Number of significant bits
    uint32_t fractional_bits;   // Number of fractional bits
    double precision_error;     // Estimated precision error
    
    // Operations history
    uint32_t operation_count;   // Number of operations performed
    double accumulated_error;   // Accumulated numerical error
    
    // Memory management
    bool owns_data;             // True if this structure owns the data
    void (*cleanup_func)(void*); // Custom cleanup function
} smart_number_t;

// --- Arithmetic Operations ---
typedef enum {
    SMART_OP_ADD = 0,
    SMART_OP_SUB = 1,
    SMART_OP_MUL = 2,
    SMART_OP_DIV = 3,
    SMART_OP_MOD = 4,
    SMART_OP_POW = 5,
    SMART_OP_SQRT = 6,
    SMART_OP_LOG = 7,
    SMART_OP_EXP = 8,
    SMART_OP_SIN = 9,
    SMART_OP_COS = 10,
    SMART_OP_TAN = 11
} smart_operation_t;

// --- Binary Decomposition Node ---
typedef struct binary_node {
    uint8_t bit_value;          // 0 or 1
    uint32_t position;          // Bit position
    double weight;              // Positional weight
    struct binary_node* left;   // Left child (less significant)
    struct binary_node* right;  // Right child (more significant)
    struct binary_node* parent; // Parent node
} binary_node_t;

// --- M→B→H Context ---
typedef struct {
    // Machine to Binary conversion
    bool (*m_to_b_converter)(const void* machine_data, size_t size, binary_node_t** root);
    
    // Binary to Human conversion
    bool (*b_to_h_converter)(const binary_node_t* root, char* human_str, size_t max_size);
    
    // Human to Binary conversion
    bool (*h_to_b_converter)(const char* human_str, binary_node_t** root);
    
    // Binary to Machine conversion
    bool (*b_to_m_converter)(const binary_node_t* root, void* machine_data, size_t max_size);
    
    // Precision management
    precision_level_t default_precision;
    double error_threshold;
    bool auto_precision_adjust;
    
    // Statistics
    uint64_t total_conversions;
    uint64_t total_operations;
    double average_error;
} mbh_context_t;

// --- Function Declarations ---

// Smart number creation and destruction
smart_number_t* smart_number_create(precision_level_t precision);
smart_number_t* smart_number_from_int(int64_t value, precision_level_t precision);
smart_number_t* smart_number_from_float(double value, precision_level_t precision);
smart_number_t* smart_number_from_string(const char* str, precision_level_t precision);
smart_number_t* smart_number_copy(const smart_number_t* src);
void smart_number_destroy(smart_number_t* num);

// Representation conversion
int smart_number_to_machine(smart_number_t* num);
int smart_number_to_binary(smart_number_t* num);
int smart_number_to_human(smart_number_t* num);
int smart_number_convert(smart_number_t* num, smart_num_repr_t target_repr);

// Arithmetic operations
smart_number_t* smart_number_add(const smart_number_t* a, const smart_number_t* b);
smart_number_t* smart_number_subtract(const smart_number_t* a, const smart_number_t* b);
smart_number_t* smart_number_multiply(const smart_number_t* a, const smart_number_t* b);
smart_number_t* smart_number_divide(const smart_number_t* a, const smart_number_t* b);
smart_number_t* smart_number_modulo(const smart_number_t* a, const smart_number_t* b);
smart_number_t* smart_number_power(const smart_number_t* base, const smart_number_t* exponent);

// Advanced operations
smart_number_t* smart_number_sqrt(const smart_number_t* num);
smart_number_t* smart_number_log(const smart_number_t* num);
smart_number_t* smart_number_exp(const smart_number_t* num);
smart_number_t* smart_number_sin(const smart_number_t* num);
smart_number_t* smart_number_cos(const smart_number_t* num);
smart_number_t* smart_number_tan(const smart_number_t* num);

// Comparison operations
int smart_number_compare(const smart_number_t* a, const smart_number_t* b);
bool smart_number_equals(const smart_number_t* a, const smart_number_t* b);
bool smart_number_less_than(const smart_number_t* a, const smart_number_t* b);
bool smart_number_greater_than(const smart_number_t* a, const smart_number_t* b);

// Precision management
int smart_number_set_precision(smart_number_t* num, precision_level_t precision);
precision_level_t smart_number_get_precision(const smart_number_t* num);
double smart_number_get_error(const smart_number_t* num);
int smart_number_normalize(smart_number_t* num);

// Binary decomposition operations
binary_node_t* binary_decomp_create_tree(const void* data, size_t size, bool is_signed);
void binary_decomp_destroy_tree(binary_node_t* root);
int binary_decomp_traverse(const binary_node_t* root, void (*visitor)(const binary_node_t*));
binary_node_t* binary_decomp_find_bit(const binary_node_t* root, uint32_t position);

// M→B→H context management
mbh_context_t* mbh_context_create(void);
void mbh_context_destroy(mbh_context_t* ctx);
int mbh_context_set_precision(mbh_context_t* ctx, precision_level_t precision);
int mbh_context_set_error_threshold(mbh_context_t* ctx, double threshold);

// Conversion utilities
int64_t smart_number_to_int64(const smart_number_t* num);
double smart_number_to_double(const smart_number_t* num);
char* smart_number_to_string(const smart_number_t* num);

// Utility functions
bool smart_number_is_zero(const smart_number_t* num);
bool smart_number_is_nan(const smart_number_t* num);
bool smart_number_is_infinite(const smart_number_t* num);
size_t smart_number_get_memory_usage(const smart_number_t* num);

// Statistics and debugging
void smart_number_print_stats(const smart_number_t* num);
void smart_number_print_binary_decomp(const smart_number_t* num);
void smart_number_print_all_representations(const smart_number_t* num);

// Error codes
#define SMART_NUM_SUCCESS           0
#define SMART_NUM_ERROR_NULL_PTR    -1
#define SMART_NUM_ERROR_INVALID     -2
#define SMART_NUM_ERROR_NO_MEMORY   -3
#define SMART_NUM_ERROR_OVERFLOW    -4
#define SMART_NUM_ERROR_UNDERFLOW   -5
#define SMART_NUM_ERROR_DIV_ZERO    -6
#define SMART_NUM_ERROR_PRECISION   -7
#define SMART_NUM_ERROR_CONVERSION  -8

// Global context (can be overridden)
extern mbh_context_t* global_mbh_context;

// Initialization and cleanup
int smart_number_library_init(void);
void smart_number_library_cleanup(void);

#endif // AEON_SMART_NUMBER_H
