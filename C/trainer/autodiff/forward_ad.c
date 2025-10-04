
#include "forward_ad.h"
#include <math.h>
#include <stdio.h>

Dual dual_create(double value, double derivative) {
    return (Dual){.value = value, .derivative = derivative};
}

Dual dual_constant(double value) {
    return (Dual){.value = value, .derivative = 0.0};
}

Dual dual_variable(double value) {
    return (Dual){.value = value, .derivative = 1.0};
}

Dual dual_add(Dual a, Dual b) {
    return (Dual){
        .value = a.value + b.value,
        .derivative = a.derivative + b.derivative
    };
}

Dual dual_sub(Dual a, Dual b) {
    return (Dual){
        .value = a.value - b.value,
        .derivative = a.derivative - b.derivative
    };
}

Dual dual_mul(Dual a, Dual b) {
    return (Dual){
        .value = a.value * b.value,
        .derivative = a.derivative * b.value + a.value * b.derivative
    };
}

Dual dual_div(Dual a, Dual b) {
    double b_sq = b.value * b.value;
    return (Dual){
        .value = a.value / b.value,
        .derivative = (a.derivative * b.value - a.value * b.derivative) / b_sq
    };
}

Dual dual_neg(Dual a) {
    return (Dual){
        .value = -a.value,
        .derivative = -a.derivative
    };
}

Dual dual_sin(Dual a) {
    return (Dual){
        .value = sin(a.value),
        .derivative = a.derivative * cos(a.value)
    };
}

Dual dual_cos(Dual a) {
    return (Dual){
        .value = cos(a.value),
        .derivative = -a.derivative * sin(a.value)
    };
}

Dual dual_tan(Dual a) {
    double cos_val = cos(a.value);
    return (Dual){
        .value = tan(a.value),
        .derivative = a.derivative / (cos_val * cos_val)
    };
}

Dual dual_exp(Dual a) {
    double exp_val = exp(a.value);
    return (Dual){
        .value = exp_val,
        .derivative = a.derivative * exp_val
    };
}

Dual dual_log(Dual a) {
    return (Dual){
        .value = log(a.value),
        .derivative = a.derivative / a.value
    };
}

Dual dual_sqrt(Dual a) {
    double sqrt_val = sqrt(a.value);
    return (Dual){
        .value = sqrt_val,
        .derivative = a.derivative / (2.0 * sqrt_val)
    };
}

Dual dual_pow(Dual a, double n) {
    double pow_val = pow(a.value, n);
    return (Dual){
        .value = pow_val,
        .derivative = a.derivative * n * pow(a.value, n - 1.0)
    };
}

Dual dual_abs(Dual a) {
    return (Dual){
        .value = fabs(a.value),
        .derivative = a.value >= 0.0 ? a.derivative : -a.derivative
    };
}

Dual dual_tanh(Dual a) {
    double tanh_val = tanh(a.value);
    return (Dual){
        .value = tanh_val,
        .derivative = a.derivative * (1.0 - tanh_val * tanh_val)
    };
}

Dual dual_sigmoid(Dual a) {
    double sig = 1.0 / (1.0 + exp(-a.value));
    return (Dual){
        .value = sig,
        .derivative = a.derivative * sig * (1.0 - sig)
    };
}

Dual dual_relu(Dual a) {
    return (Dual){
        .value = a.value > 0.0 ? a.value : 0.0,
        .derivative = a.value > 0.0 ? a.derivative : 0.0
    };
}

bool dual_eq(Dual a, Dual b, double epsilon) {
    return fabs(a.value - b.value) < epsilon && 
           fabs(a.derivative - b.derivative) < epsilon;
}

bool dual_lt(Dual a, Dual b) {
    return a.value < b.value;
}

bool dual_gt(Dual a, Dual b) {
    return a.value > b.value;
}

void dual_print(Dual a) {
    printf("Dual(value=%.6f, derivative=%.6f)\n", a.value, a.derivative);
}
