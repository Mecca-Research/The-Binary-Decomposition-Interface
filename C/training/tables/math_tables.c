
#include "../include/training_tables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>

#define MAX_NUM 10000
#define SELECTIVE_MAX 100000

// Helper to create entry
static training_entry_t *create_math_entry(uint32_t type, uint32_t difficulty, 
                                           const void *input, size_t input_size,
                                           const void *output, size_t output_size) {
    training_entry_t *entry = malloc(sizeof(training_entry_t) + input_size + output_size);
    if (!entry) return NULL;
    
    entry->entry_type = type;
    entry->difficulty = difficulty;
    entry->input_size = input_size;
    entry->output_size = output_size;
    
    memcpy(entry->data, input, input_size);
    memcpy(entry->data + input_size, output, output_size);
    
    return entry;
}

// GCD using Euclidean algorithm
static uint64_t gcd(uint64_t a, uint64_t b) {
    while (b != 0) {
        uint64_t temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

// LCM
static uint64_t lcm(uint64_t a, uint64_t b) {
    if (a == 0 || b == 0) return 0;
    return (a / gcd(a, b)) * b;
}

// Check if prime
static bool is_prime(uint64_t n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    
    for (uint64_t i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}

// Modular exponentiation
static uint64_t mod_pow(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t result = 1;
    base %= mod;
    
    while (exp > 0) {
        if (exp & 1) {
            result = (result * base) % mod;
        }
        base = (base * base) % mod;
        exp >>= 1;
    }
    
    return result;
}

int generate_math_arithmetic_table(const char *filename) {
    printf("Generating arithmetic table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_MATH_ARITHMETIC,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    // Reserve space for header
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // Addition (0-1000)
    for (uint32_t a = 0; a <= 1000; a++) {
        for (uint32_t b = 0; b <= 1000; b++) {
            uint32_t input[2] = {a, b};
            uint32_t output = a + b;
            uint32_t difficulty = (a + b) / 250;
            if (difficulty > 7) difficulty = 7;
            
            training_entry_t *entry = create_math_entry(
                ENTRY_MATH_ARITHMETIC, difficulty,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Subtraction (0-1000)
    for (uint32_t a = 0; a <= 1000; a++) {
        for (uint32_t b = 0; b <= a; b++) {
            uint32_t input[2] = {a, b};
            uint32_t output = a - b;
            uint32_t difficulty = a / 250;
            if (difficulty > 7) difficulty = 7;
            
            training_entry_t *entry = create_math_entry(
                ENTRY_MATH_ARITHMETIC, difficulty,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Multiplication (0-500)
    for (uint32_t a = 0; a <= 500; a++) {
        for (uint32_t b = 0; b <= 500; b++) {
            uint32_t input[2] = {a, b};
            uint32_t output = a * b;
            uint32_t difficulty = (a * b) / 10000;
            if (difficulty > 7) difficulty = 7;
            
            training_entry_t *entry = create_math_entry(
                ENTRY_MATH_ARITHMETIC, difficulty,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Division (0-1000, divisor 1-100)
    for (uint32_t a = 0; a <= 1000; a++) {
        for (uint32_t b = 1; b <= 100; b++) {
            uint32_t input[2] = {a, b};
            uint32_t output[2] = {a / b, a % b}; // quotient and remainder
            uint32_t difficulty = a / 250;
            if (difficulty > 7) difficulty = 7;
            
            training_entry_t *entry = create_math_entry(
                ENTRY_MATH_ARITHMETIC, difficulty,
                input, sizeof(input), output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Exponentiation (base 0-20, exp 0-10)
    for (uint32_t a = 0; a <= 20; a++) {
        for (uint32_t b = 0; b <= 10; b++) {
            uint32_t input[2] = {a, b};
            uint64_t output = (uint64_t)pow(a, b);
            uint32_t difficulty = b;
            if (difficulty > 7) difficulty = 7;
            
            training_entry_t *entry = create_math_entry(
                ENTRY_MATH_ARITHMETIC, difficulty,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Update header
    header.num_entries = count;
    header.total_size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    table_write_header(fd, &header);
    
    close(fd);
    printf("Generated %u arithmetic entries\n", count);
    return 0;
}

int generate_math_modular_table(const char *filename) {
    printf("Generating modular arithmetic table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_MATH_MODULAR,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    uint32_t moduli[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47};
    
    // Modular addition
    for (size_t m_idx = 0; m_idx < sizeof(moduli)/sizeof(moduli[0]); m_idx++) {
        uint32_t mod = moduli[m_idx];
        for (uint32_t a = 0; a < mod * 10 && a < 500; a++) {
            for (uint32_t b = 0; b < mod * 10 && b < 500; b++) {
                uint32_t input[3] = {a, b, mod};
                uint32_t output = (a + b) % mod;
                uint32_t difficulty = m_idx / 2;
                if (difficulty > 7) difficulty = 7;
                
                training_entry_t *entry = create_math_entry(
                    ENTRY_MATH_MODULAR, difficulty,
                    input, sizeof(input), &output, sizeof(output)
                );
                
                if (entry) {
                    table_write_entry(fd, entry);
                    free(entry);
                    count++;
                }
            }
        }
    }
    
    // Modular multiplication
    for (size_t m_idx = 0; m_idx < sizeof(moduli)/sizeof(moduli[0]); m_idx++) {
        uint32_t mod = moduli[m_idx];
        for (uint32_t a = 0; a < mod * 5 && a < 200; a++) {
            for (uint32_t b = 0; b < mod * 5 && b < 200; b++) {
                uint32_t input[3] = {a, b, mod};
                uint32_t output = ((uint64_t)a * b) % mod;
                uint32_t difficulty = m_idx / 2 + 1;
                if (difficulty > 7) difficulty = 7;
                
                training_entry_t *entry = create_math_entry(
                    ENTRY_MATH_MODULAR, difficulty,
                    input, sizeof(input), &output, sizeof(output)
                );
                
                if (entry) {
                    table_write_entry(fd, entry);
                    free(entry);
                    count++;
                }
            }
        }
    }
    
    // Modular exponentiation
    for (size_t m_idx = 0; m_idx < sizeof(moduli)/sizeof(moduli[0]); m_idx++) {
        uint32_t mod = moduli[m_idx];
        for (uint32_t base = 0; base < 50; base++) {
            for (uint32_t exp = 0; exp < 20; exp++) {
                uint32_t input[3] = {base, exp, mod};
                uint32_t output = mod_pow(base, exp, mod);
                uint32_t difficulty = m_idx / 2 + 2;
                if (difficulty > 7) difficulty = 7;
                
                training_entry_t *entry = create_math_entry(
                    ENTRY_MATH_MODULAR, difficulty,
                    input, sizeof(input), &output, sizeof(output)
                );
                
                if (entry) {
                    table_write_entry(fd, entry);
                    free(entry);
                    count++;
                }
            }
        }
    }
    
    header.num_entries = count;
    header.total_size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    table_write_header(fd, &header);
    
    close(fd);
    printf("Generated %u modular arithmetic entries\n", count);
    return 0;
}

int generate_math_number_theory_table(const char *filename) {
    printf("Generating number theory table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_MATH_NUMBER_THEORY,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // GCD (0-1000)
    for (uint32_t a = 0; a <= 1000; a++) {
        for (uint32_t b = 0; b <= 1000; b++) {
            uint32_t input[2] = {a, b};
            uint64_t output = gcd(a, b);
            uint32_t difficulty = (a + b) / 250;
            if (difficulty > 7) difficulty = 7;
            
            training_entry_t *entry = create_math_entry(
                ENTRY_MATH_NUMBER_THEORY, difficulty,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // LCM (0-500)
    for (uint32_t a = 1; a <= 500; a++) {
        for (uint32_t b = 1; b <= 500; b++) {
            uint32_t input[2] = {a, b};
            uint64_t output = lcm(a, b);
            uint32_t difficulty = (a + b) / 125 + 1;
            if (difficulty > 7) difficulty = 7;
            
            training_entry_t *entry = create_math_entry(
                ENTRY_MATH_NUMBER_THEORY, difficulty,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Coprimality test (0-1000)
    for (uint32_t a = 1; a <= 1000; a++) {
        for (uint32_t b = 1; b <= 1000; b++) {
            uint32_t input[2] = {a, b};
            uint32_t output = (gcd(a, b) == 1) ? 1 : 0;
            uint32_t difficulty = (a + b) / 250 + 2;
            if (difficulty > 7) difficulty = 7;
            
            training_entry_t *entry = create_math_entry(
                ENTRY_MATH_NUMBER_THEORY, difficulty,
                input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
        }
    }
    
    // Prime test (0-10000)
    for (uint32_t n = 0; n <= MAX_NUM; n++) {
        uint32_t input = n;
        uint32_t output = is_prime(n) ? 1 : 0;
        uint32_t difficulty = n / 1250 + 3;
        if (difficulty > 7) difficulty = 7;
        
        training_entry_t *entry = create_math_entry(
            ENTRY_MATH_NUMBER_THEORY, difficulty,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    header.num_entries = count;
    header.total_size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    table_write_header(fd, &header);
    
    close(fd);
    printf("Generated %u number theory entries\n", count);
    return 0;
}

int generate_math_sequences_table(const char *filename) {
    printf("Generating sequences table: %s\n", filename);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("Failed to create file");
        return -1;
    }
    
    table_header_t header = {
        .magic = 0x42444954,
        .version = 1,
        .entry_type = ENTRY_MATH_SEQUENCES,
        .num_entries = 0,
        .total_size = 0,
        .checksum = 0
    };
    
    lseek(fd, sizeof(table_header_t), SEEK_SET);
    
    uint32_t count = 0;
    
    // Fibonacci sequence (first 50 terms)
    uint64_t fib[50];
    fib[0] = 0;
    fib[1] = 1;
    for (int i = 2; i < 50; i++) {
        fib[i] = fib[i-1] + fib[i-2];
    }
    
    for (uint32_t n = 0; n < 50; n++) {
        uint32_t input = n;
        uint64_t output = fib[n];
        uint32_t difficulty = n / 7;
        if (difficulty > 7) difficulty = 7;
        
        training_entry_t *entry = create_math_entry(
            ENTRY_MATH_SEQUENCES, difficulty,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Prime numbers (first 1000 primes)
    uint32_t prime_count = 0;
    for (uint32_t n = 2; n <= 10000 && prime_count < 1000; n++) {
        if (is_prime(n)) {
            uint32_t input = prime_count;
            uint32_t output = n;
            uint32_t difficulty = prime_count / 125 + 1;
            if (difficulty > 7) difficulty = 7;
            
            training_entry_t *entry = create_math_entry(
                ENTRY_MATH_SEQUENCES, difficulty,
                &input, sizeof(input), &output, sizeof(output)
            );
            
            if (entry) {
                table_write_entry(fd, entry);
                free(entry);
                count++;
            }
            prime_count++;
        }
    }
    
    // Perfect squares (0-1000)
    for (uint32_t n = 0; n <= 1000; n++) {
        uint32_t input = n;
        uint64_t output = (uint64_t)n * n;
        uint32_t difficulty = n / 125;
        if (difficulty > 7) difficulty = 7;
        
        training_entry_t *entry = create_math_entry(
            ENTRY_MATH_SEQUENCES, difficulty,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Triangular numbers (0-500)
    for (uint32_t n = 0; n <= 500; n++) {
        uint32_t input = n;
        uint64_t output = ((uint64_t)n * (n + 1)) / 2;
        uint32_t difficulty = n / 62 + 1;
        if (difficulty > 7) difficulty = 7;
        
        training_entry_t *entry = create_math_entry(
            ENTRY_MATH_SEQUENCES, difficulty,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Factorials (0-20)
    uint64_t factorial = 1;
    for (uint32_t n = 0; n <= 20; n++) {
        if (n > 0) factorial *= n;
        
        uint32_t input = n;
        uint64_t output = factorial;
        uint32_t difficulty = n / 3 + 2;
        if (difficulty > 7) difficulty = 7;
        
        training_entry_t *entry = create_math_entry(
            ENTRY_MATH_SEQUENCES, difficulty,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    // Catalan numbers (0-30)
    uint64_t catalan[31];
    catalan[0] = 1;
    for (int n = 1; n <= 30; n++) {
        catalan[n] = 0;
        for (int i = 0; i < n; i++) {
            catalan[n] += catalan[i] * catalan[n-1-i];
        }
    }
    
    for (uint32_t n = 0; n <= 30; n++) {
        uint32_t input = n;
        uint64_t output = catalan[n];
        uint32_t difficulty = n / 4 + 3;
        if (difficulty > 7) difficulty = 7;
        
        training_entry_t *entry = create_math_entry(
            ENTRY_MATH_SEQUENCES, difficulty,
            &input, sizeof(input), &output, sizeof(output)
        );
        
        if (entry) {
            table_write_entry(fd, entry);
            free(entry);
            count++;
        }
    }
    
    header.num_entries = count;
    header.total_size = lseek(fd, 0, SEEK_CUR);
    lseek(fd, 0, SEEK_SET);
    table_write_header(fd, &header);
    
    close(fd);
    printf("Generated %u sequence entries\n", count);
    return 0;
}
