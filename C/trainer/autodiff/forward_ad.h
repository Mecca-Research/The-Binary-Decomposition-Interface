
#ifndef BDI_FORWARD_AD_H
#define BDI_FORWARD_AD_H

#include <stddef.h>
#include <stdbool.h>

// Dual number structure for forward-mode automatic differentiation
typedef struct {
    double value;      // Primal value
    double derivative; // Derivative (tangent)
} Dual;

// Dual number creation
Dual dual_create(double value, double derivative);
Dual dual_constant(double value);
Dual dual_variable(double value);

// Basic arithmetic operations
Dual dual_add(Dual a, Dual b);
Dual dual_sub(Dual a, Dual b);
Dual dual_mul(Dual a, Dual b);
Dual dual_div(Dual a, Dual b);
Dual dual_neg(Dual a);

// Mathematical functions
Dual dual_sin(Dual a);
Dual dual_cos(Dual a);
Dual dual_tan(Dual a);
Dual dual_exp(Dual a);
Dual dual_log(Dual a);
Dual dual_sqrt(Dual a);
Dual dual_pow(Dual a, double n);
Dual dual_abs(Dual a);
Dual dual_tanh(Dual a);
Dual dual_sigmoid(Dual a);
Dual dual_relu(Dual a);

// Comparison operations
bool dual_eq(Dual a, Dual b, double epsilon);
bool dual_lt(Dual a, Dual b);
bool dual_gt(Dual a, Dual b);

// Utility functions
void dual_print(Dual a);

#endif // BDI_FORWARD_AD_H
