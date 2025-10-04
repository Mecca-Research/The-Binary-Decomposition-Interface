
#ifndef BDI_REVERSE_AD_H
#define BDI_REVERSE_AD_H

#include <stddef.h>
#include <stdbool.h>

// Operation types for gradient tape
typedef enum {
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_NEG,
    OP_SIN,
    OP_COS,
    OP_TAN,
    OP_EXP,
    OP_LOG,
    OP_SQRT,
    OP_POW,
    OP_ABS,
    OP_TANH,
    OP_SIGMOID,
    OP_RELU,
    OP_CONSTANT,
    OP_VARIABLE
} OpType;

// Tape entry for recording operations
typedef struct {
    OpType op;
    size_t output_id;
    size_t input_ids[2];  // Up to 2 inputs
    double values[3];     // Output value and up to 2 input values
    double param;         // For operations like pow
} TapeEntry;

// Gradient tape structure
typedef struct {
    TapeEntry* entries;
    size_t size;
    size_t capacity;
    double* gradients;
    size_t num_variables;
    size_t grad_capacity;
} GradientTape;

// Tape management
GradientTape* tape_create(size_t initial_capacity);
void tape_destroy(GradientTape* tape);
void tape_clear(GradientTape* tape);
int tape_resize(GradientTape* tape, size_t new_capacity);  // Returns 0 on success, -1 on failure

// Recording operations
size_t tape_record_constant(GradientTape* tape, double value);
size_t tape_record_variable(GradientTape* tape, double value);
size_t tape_record_add(GradientTape* tape, size_t a_id, size_t b_id, double a_val, double b_val);
size_t tape_record_sub(GradientTape* tape, size_t a_id, size_t b_id, double a_val, double b_val);
size_t tape_record_mul(GradientTape* tape, size_t a_id, size_t b_id, double a_val, double b_val);
size_t tape_record_div(GradientTape* tape, size_t a_id, size_t b_id, double a_val, double b_val);
size_t tape_record_neg(GradientTape* tape, size_t a_id, double a_val);
size_t tape_record_sin(GradientTape* tape, size_t a_id, double a_val);
size_t tape_record_cos(GradientTape* tape, size_t a_id, double a_val);
size_t tape_record_tan(GradientTape* tape, size_t a_id, double a_val);
size_t tape_record_exp(GradientTape* tape, size_t a_id, double a_val);
size_t tape_record_log(GradientTape* tape, size_t a_id, double a_val);
size_t tape_record_sqrt(GradientTape* tape, size_t a_id, double a_val);
size_t tape_record_pow(GradientTape* tape, size_t a_id, double a_val, double n);
size_t tape_record_abs(GradientTape* tape, size_t a_id, double a_val);
size_t tape_record_tanh(GradientTape* tape, size_t a_id, double a_val);
size_t tape_record_sigmoid(GradientTape* tape, size_t a_id, double a_val);
size_t tape_record_relu(GradientTape* tape, size_t a_id, double a_val);

// Backward pass
void tape_backward(GradientTape* tape, size_t output_id);
double tape_get_gradient(GradientTape* tape, size_t var_id);

// Utility
void tape_print(GradientTape* tape);

#endif // BDI_REVERSE_AD_H
