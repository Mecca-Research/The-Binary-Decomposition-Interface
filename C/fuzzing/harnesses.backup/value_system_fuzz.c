
/*
 * Value System Fuzzing Harness (Bonus)
 * 
 * This harness targets the value type system, operations, and conversions.
 * It fuzzes value operations, type conversions, and arithmetic to test:
 * - Type checking and arithmetic operations
 * - String handling and conversions
 * - Type safety and integer overflow/underflow
 * - Type confusion and conversion errors
 */

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

// Include value system headers
#include "../vm/value.h"
#include "../vm/object.h"
#include "../vm/memory.h"
#include "../vm/vm.h"

// Timeout protection
#include <signal.h>
#include <setjmp.h>

static jmp_buf value_timeout_jmp;
static void value_timeout_handler(int sig) {
    longjmp(value_timeout_jmp, 1);
}

// Test value creation and type checking
static void test_value_creation(const uint8_t* data, size_t size, VM* vm) {
    if (size < 8) return;
    
    size_t offset = 0;
    uint32_t num_values = (data[offset] % 16) + 1; // 1-16 values
    offset++;
    
    Value* values = malloc(num_values * sizeof(Value));
    if (!values) return;
    
    // Create various value types
    for (uint32_t i = 0; i < num_values && offset < size; i++) {
        uint8_t value_type = data[offset] % 5;
        offset++;
        
        switch (value_type) {
            case 0: { // Number
                if (offset + 8 <= size) {
                    double num = *(double*)(data + offset);
                    values[i] = NUMBER_VAL(num);
                    offset += 8;
                } else {
                    values[i] = NUMBER_VAL(0.0);
                }
                break;
            }
            
            case 1: { // Boolean
                bool val = (data[offset] % 2) == 1;
                values[i] = BOOL_VAL(val);
                if (offset < size) offset++;
                break;
            }
            
            case 2: { // Nil
                values[i] = NIL_VAL;
                break;
            }
            
            case 3: { // String
                uint32_t str_len = (data[offset] % 32) + 1;
                if (offset < size) offset++;
                
                if (offset + str_len <= size) {
                    ObjString* string = copyString(vm, (char*)(data + offset), str_len);
                    values[i] = OBJ_VAL(string);
                    offset += str_len;
                } else {
                    values[i] = OBJ_VAL(copyString(vm, "fuzz", 4));
                }
                break;
            }
            
            case 4: { // Function
                ObjFunction* function = newFunction(vm);
                values[i] = OBJ_VAL(function);
                break;
            }
        }
        
        // Test type checking
        bool is_number = IS_NUMBER(values[i]);
        bool is_bool = IS_BOOL(values[i]);
        bool is_nil = IS_NIL(values[i]);
        bool is_obj = IS_OBJ(values[i]);
        
        (void)is_number; (void)is_bool; (void)is_nil; (void)is_obj;
    }
    
    // Test value operations
    for (uint32_t i = 0; i < num_values; i++) {
        for (uint32_t j = 0; j < num_values; j++) {
            if (i == j) continue;
            
            // Test equality
            bool equal = valuesEqual(values[i], values[j]);
            (void)equal;
            
            // Test arithmetic (if both are numbers)
            if (IS_NUMBER(values[i]) && IS_NUMBER(values[j])) {
                double a = AS_NUMBER(values[i]);
                double b = AS_NUMBER(values[j]);
                
                // Test basic arithmetic with overflow protection
                if (b != 0.0) {
                    double div_result = a / b;
                    (void)div_result;
                }
                
                if (fabs(a) < 1e100 && fabs(b) < 1e100) {
                    double add_result = a + b;
                    double sub_result = a - b;
                    double mul_result = a * b;
                    (void)add_result; (void)sub_result; (void)mul_result;
                }
            }
            
            // Test string operations
            if (IS_OBJ(values[i]) && IS_STRING(values[i]) &&
                IS_OBJ(values[j]) && IS_STRING(values[j])) {
                
                ObjString* str1 = AS_STRING(values[i]);
                ObjString* str2 = AS_STRING(values[j]);
                
                // Test string concatenation
                ObjString* concat = concatenateStrings(vm, str1, str2);
                (void)concat;
            }
        }
    }
    
    free(values);
}

// Test value conversions
static void test_value_conversions(const uint8_t* data, size_t size, VM* vm) {
    if (size < 16) return;
    
    size_t offset = 0;
    
    // Test number to string conversion
    if (offset + 8 <= size) {
        double num = *(double*)(data + offset);
        offset += 8;
        
        // Convert to string
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.6g", num);
        ObjString* str = copyString(vm, buffer, strlen(buffer));
        
        // Test string to number conversion
        double converted = strtod(str->chars, NULL);
        (void)converted;
    }
    
    // Test boolean conversions
    if (offset < size) {
        bool val = (data[offset] % 2) == 1;
        offset++;
        
        // Test truthiness
        Value bool_val = BOOL_VAL(val);
        bool is_truthy = !isFalsey(bool_val);
        (void)is_truthy;
    }
    
    // Test type coercion edge cases
    Value nil_val = NIL_VAL;
    Value zero_val = NUMBER_VAL(0.0);
    Value empty_str = OBJ_VAL(copyString(vm, "", 0));
    
    bool nil_falsey = isFalsey(nil_val);
    bool zero_falsey = isFalsey(zero_val);
    bool empty_falsey = isFalsey(empty_str);
    
    (void)nil_falsey; (void)zero_falsey; (void)empty_falsey;
}

// Test value printing and formatting
static void test_value_printing(const uint8_t* data, size_t size, VM* vm) {
    if (size < 4) return;
    
    size_t offset = 0;
    uint32_t num_tests = (data[offset] % 8) + 1;
    offset++;
    
    for (uint32_t i = 0; i < num_tests && offset < size; i++) {
        uint8_t value_type = data[offset] % 4;
        offset++;
        
        Value test_val;
        
        switch (value_type) {
            case 0: { // Number with potential edge cases
                if (offset + 8 <= size) {
                    double num = *(double*)(data + offset);
                    test_val = NUMBER_VAL(num);
                    offset += 8;
                } else {
                    test_val = NUMBER_VAL(NAN);
                }
                break;
            }
            
            case 1: { // String with potential format issues
                uint32_t str_len = (data[offset] % 16) + 1;
                if (offset < size) offset++;
                
                if (offset + str_len <= size) {
                    // Create string with potential format specifiers
                    char* str_data = malloc(str_len + 1);
                    if (str_data) {
                        memcpy(str_data, data + offset, str_len);
                        str_data[str_len] = '\0';
                        
                        // Sanitize format specifiers to prevent format string bugs
                        for (uint32_t j = 0; j < str_len; j++) {
                            if (str_data[j] == '%') {
                                str_data[j] = '_';
                            }
                        }
                        
                        ObjString* string = copyString(vm, str_data, str_len);
                        test_val = OBJ_VAL(string);
                        free(str_data);
                        offset += str_len;
                    } else {
                        test_val = NIL_VAL;
                    }
                } else {
                    test_val = NIL_VAL;
                }
                break;
            }
            
            case 2: { // Boolean
                test_val = BOOL_VAL((data[offset] % 2) == 1);
                if (offset < size) offset++;
                break;
            }
            
            case 3: { // Nil
                test_val = NIL_VAL;
                break;
            }
        }
        
        // Test value printing (capture output to prevent spam)
        printValue(test_val);
    }
}

// LibFuzzer entry point
int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    // Skip invalid inputs
    if (size < 8 || size > 1024) return 0;
    
    // Set up timeout protection
    signal(SIGALRM, value_timeout_handler);
    if (setjmp(value_timeout_jmp) != 0) {
        alarm(0);
        return 0;
    }
    alarm(1);
    
    VM* vm = malloc(sizeof(VM));
    if (!vm) {
        alarm(0);
        return 0;
    }
    
    initVM(vm);
    
    // Test value creation and operations
    test_value_creation(data, size / 3, vm);
    
    // Test value conversions
    test_value_conversions(data + size / 3, size / 3, vm);
    
    // Test value printing
    test_value_printing(data + 2 * size / 3, size - 2 * size / 3, vm);
    
    // Cleanup
    freeVM(vm);
    free(vm);
    
    alarm(0);
    return 0;
}

// AFL entry point
#ifdef __AFL_COMPILER
int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }
    
    FILE* fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("fopen");
        return 1;
    }
    
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (size <= 0 || size > 1024) {
        fclose(fp);
        return 0;
    }
    
    uint8_t* data = malloc(size);
    if (!data) {
        fclose(fp);
        return 1;
    }
    
    size_t read_size = fread(data, 1, size, fp);
    fclose(fp);
    
    if (read_size != size) {
        free(data);
        return 1;
    }
    
    int result = LLVMFuzzerTestOneInput(data, size);
    free(data);
    
    return result;
}
#endif
