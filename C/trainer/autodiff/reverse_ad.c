
#include "reverse_ad.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

GradientTape* tape_create(size_t initial_capacity) {
    GradientTape* tape = malloc(sizeof(GradientTape));
    if (!tape) return NULL;
    
    tape->entries = malloc(initial_capacity * sizeof(TapeEntry));
    if (!tape->entries) {
        free(tape);
        return NULL;
    }
    
    tape->gradients = calloc(initial_capacity, sizeof(double));
    if (!tape->gradients) {
        free(tape->entries);
        free(tape);
        return NULL;
    }
    
    tape->size = 0;
    tape->capacity = initial_capacity;
    tape->num_variables = 0;
    tape->grad_capacity = initial_capacity;
    
    return tape;
}

void tape_destroy(GradientTape* tape) {
    if (tape) {
        free(tape->entries);
        free(tape->gradients);
        free(tape);
    }
}

void tape_clear(GradientTape* tape) {
    if (tape) {
        tape->size = 0;
        tape->num_variables = 0;
        memset(tape->gradients, 0, tape->grad_capacity * sizeof(double));
    }
}

int tape_resize(GradientTape* tape, size_t new_capacity) {
    if (!tape || new_capacity <= tape->capacity) return 0;
    
    TapeEntry* new_entries = realloc(tape->entries, new_capacity * sizeof(TapeEntry));
    if (!new_entries) {
        fprintf(stderr, "Error: Failed to reallocate tape entries buffer\n");
        return -1;
    }
    tape->entries = new_entries;
    tape->capacity = new_capacity;
    
    if (new_capacity > tape->grad_capacity) {
        double* new_grads = realloc(tape->gradients, new_capacity * sizeof(double));
        if (!new_grads) {
            fprintf(stderr, "Error: Failed to reallocate tape gradients buffer\n");
            return -1;
        }
        memset(new_grads + tape->grad_capacity, 0, 
               (new_capacity - tape->grad_capacity) * sizeof(double));
        tape->gradients = new_grads;
        tape->grad_capacity = new_capacity;
    }
    
    return 0;
}

static size_t tape_add_entry(GradientTape* tape, TapeEntry entry) {
    if (tape->size >= tape->capacity) {
        if (tape_resize(tape, tape->capacity * 2) != 0) {
            fprintf(stderr, "Error: Failed to resize tape, cannot add entry\n");
            return (size_t)-1;  // Return error indicator
        }
    }
    
    size_t id = tape->size;
    tape->entries[tape->size++] = entry;
    return id;
}

size_t tape_record_constant(GradientTape* tape, double value) {
    TapeEntry entry = {
        .op = OP_CONSTANT,
        .output_id = tape->size,
        .values = {value, 0.0, 0.0},
        .input_ids = {0, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_variable(GradientTape* tape, double value) {
    TapeEntry entry = {
        .op = OP_VARIABLE,
        .output_id = tape->size,
        .values = {value, 0.0, 0.0},
        .input_ids = {0, 0},
        .param = 0.0
    };
    tape->num_variables++;
    return tape_add_entry(tape, entry);
}

size_t tape_record_add(GradientTape* tape, size_t a_id, size_t b_id, double a_val, double b_val) {
    TapeEntry entry = {
        .op = OP_ADD,
        .output_id = tape->size,
        .values = {a_val + b_val, a_val, b_val},
        .input_ids = {a_id, b_id},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_sub(GradientTape* tape, size_t a_id, size_t b_id, double a_val, double b_val) {
    TapeEntry entry = {
        .op = OP_SUB,
        .output_id = tape->size,
        .values = {a_val - b_val, a_val, b_val},
        .input_ids = {a_id, b_id},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_mul(GradientTape* tape, size_t a_id, size_t b_id, double a_val, double b_val) {
    TapeEntry entry = {
        .op = OP_MUL,
        .output_id = tape->size,
        .values = {a_val * b_val, a_val, b_val},
        .input_ids = {a_id, b_id},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_div(GradientTape* tape, size_t a_id, size_t b_id, double a_val, double b_val) {
    TapeEntry entry = {
        .op = OP_DIV,
        .output_id = tape->size,
        .values = {a_val / b_val, a_val, b_val},
        .input_ids = {a_id, b_id},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_neg(GradientTape* tape, size_t a_id, double a_val) {
    TapeEntry entry = {
        .op = OP_NEG,
        .output_id = tape->size,
        .values = {-a_val, a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_sin(GradientTape* tape, size_t a_id, double a_val) {
    TapeEntry entry = {
        .op = OP_SIN,
        .output_id = tape->size,
        .values = {sin(a_val), a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_cos(GradientTape* tape, size_t a_id, double a_val) {
    TapeEntry entry = {
        .op = OP_COS,
        .output_id = tape->size,
        .values = {cos(a_val), a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_tan(GradientTape* tape, size_t a_id, double a_val) {
    TapeEntry entry = {
        .op = OP_TAN,
        .output_id = tape->size,
        .values = {tan(a_val), a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_exp(GradientTape* tape, size_t a_id, double a_val) {
    TapeEntry entry = {
        .op = OP_EXP,
        .output_id = tape->size,
        .values = {exp(a_val), a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_log(GradientTape* tape, size_t a_id, double a_val) {
    TapeEntry entry = {
        .op = OP_LOG,
        .output_id = tape->size,
        .values = {log(a_val), a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_sqrt(GradientTape* tape, size_t a_id, double a_val) {
    TapeEntry entry = {
        .op = OP_SQRT,
        .output_id = tape->size,
        .values = {sqrt(a_val), a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_pow(GradientTape* tape, size_t a_id, double a_val, double n) {
    TapeEntry entry = {
        .op = OP_POW,
        .output_id = tape->size,
        .values = {pow(a_val, n), a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = n
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_abs(GradientTape* tape, size_t a_id, double a_val) {
    TapeEntry entry = {
        .op = OP_ABS,
        .output_id = tape->size,
        .values = {fabs(a_val), a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_tanh(GradientTape* tape, size_t a_id, double a_val) {
    TapeEntry entry = {
        .op = OP_TANH,
        .output_id = tape->size,
        .values = {tanh(a_val), a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_sigmoid(GradientTape* tape, size_t a_id, double a_val) {
    double sig = 1.0 / (1.0 + exp(-a_val));
    TapeEntry entry = {
        .op = OP_SIGMOID,
        .output_id = tape->size,
        .values = {sig, a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

size_t tape_record_relu(GradientTape* tape, size_t a_id, double a_val) {
    TapeEntry entry = {
        .op = OP_RELU,
        .output_id = tape->size,
        .values = {a_val > 0.0 ? a_val : 0.0, a_val, 0.0},
        .input_ids = {a_id, 0},
        .param = 0.0
    };
    return tape_add_entry(tape, entry);
}

void tape_backward(GradientTape* tape, size_t output_id) {
    if (!tape || output_id >= tape->size) return;
    
    // Initialize all gradients to zero
    memset(tape->gradients, 0, tape->grad_capacity * sizeof(double));
    
    // Set output gradient to 1.0
    tape->gradients[output_id] = 1.0;
    
    // Backward pass through tape in reverse order
    for (size_t i = tape->size; i > 0; i--) {
        size_t idx = i - 1;
        TapeEntry* entry = &tape->entries[idx];
        double grad_out = tape->gradients[entry->output_id];
        
        if (grad_out == 0.0) continue;
        
        switch (entry->op) {
            case OP_ADD:
                tape->gradients[entry->input_ids[0]] += grad_out;
                tape->gradients[entry->input_ids[1]] += grad_out;
                break;
                
            case OP_SUB:
                tape->gradients[entry->input_ids[0]] += grad_out;
                tape->gradients[entry->input_ids[1]] -= grad_out;
                break;
                
            case OP_MUL:
                tape->gradients[entry->input_ids[0]] += grad_out * entry->values[2];
                tape->gradients[entry->input_ids[1]] += grad_out * entry->values[1];
                break;
                
            case OP_DIV:
                tape->gradients[entry->input_ids[0]] += grad_out / entry->values[2];
                tape->gradients[entry->input_ids[1]] -= grad_out * entry->values[1] / 
                                                        (entry->values[2] * entry->values[2]);
                break;
                
            case OP_NEG:
                tape->gradients[entry->input_ids[0]] -= grad_out;
                break;
                
            case OP_SIN:
                tape->gradients[entry->input_ids[0]] += grad_out * cos(entry->values[1]);
                break;
                
            case OP_COS:
                tape->gradients[entry->input_ids[0]] -= grad_out * sin(entry->values[1]);
                break;
                
            case OP_TAN: {
                double cos_val = cos(entry->values[1]);
                tape->gradients[entry->input_ids[0]] += grad_out / (cos_val * cos_val);
                break;
            }
                
            case OP_EXP:
                tape->gradients[entry->input_ids[0]] += grad_out * entry->values[0];
                break;
                
            case OP_LOG:
                tape->gradients[entry->input_ids[0]] += grad_out / entry->values[1];
                break;
                
            case OP_SQRT:
                tape->gradients[entry->input_ids[0]] += grad_out / (2.0 * entry->values[0]);
                break;
                
            case OP_POW:
                tape->gradients[entry->input_ids[0]] += grad_out * entry->param * 
                                                        pow(entry->values[1], entry->param - 1.0);
                break;
                
            case OP_ABS:
                tape->gradients[entry->input_ids[0]] += grad_out * 
                                                        (entry->values[1] >= 0.0 ? 1.0 : -1.0);
                break;
                
            case OP_TANH: {
                double tanh_val = entry->values[0];
                tape->gradients[entry->input_ids[0]] += grad_out * (1.0 - tanh_val * tanh_val);
                break;
            }
                
            case OP_SIGMOID: {
                double sig = entry->values[0];
                tape->gradients[entry->input_ids[0]] += grad_out * sig * (1.0 - sig);
                break;
            }
                
            case OP_RELU:
                tape->gradients[entry->input_ids[0]] += grad_out * (entry->values[1] > 0.0 ? 1.0 : 0.0);
                break;
                
            case OP_CONSTANT:
            case OP_VARIABLE:
                // No backward pass needed
                break;
        }
    }
}

double tape_get_gradient(GradientTape* tape, size_t var_id) {
    if (!tape || var_id >= tape->grad_capacity) return 0.0;
    return tape->gradients[var_id];
}

void tape_print(GradientTape* tape) {
    if (!tape) return;
    printf("GradientTape: %zu entries, %zu variables\n", tape->size, tape->num_variables);
    for (size_t i = 0; i < tape->size; i++) {
        TapeEntry* e = &tape->entries[i];
        printf("  [%zu] op=%d, out=%.4f, grad=%.4f\n", 
               i, e->op, e->values[0], tape->gradients[i]);
    }
}
