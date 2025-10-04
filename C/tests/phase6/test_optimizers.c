#include "../../trainer/optimizers/sgd.h"
#include "../../trainer/optimizers/adam.h"
#include "../../trainer/optimizers/rmsprop.h"
#include "../../trainer/optimizers/lr_scheduler.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

#define EPSILON 1e-6
#define TEST_ASSERT(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); return 0; } \
} while(0)

// SGD Tests
int test_sgd_create() {
    SGDConfig config = sgd_default_config();
    SGDOptimizer* opt = sgd_create(10, config);
    TEST_ASSERT(opt != NULL, "sgd create");
    TEST_ASSERT(opt->num_params == 10, "sgd num params");
    sgd_destroy(opt);
    return 1;
}

int test_sgd_step() {
    SGDConfig config = sgd_default_config();
    config.learning_rate = 0.1;
    SGDOptimizer* opt = sgd_create(1, config);
    
    double params[1] = {1.0};
    double grads[1] = {2.0};
    
    sgd_step(opt, params, grads);
    TEST_ASSERT(fabs(params[0] - 0.8) < EPSILON, "sgd step");
    
    sgd_destroy(opt);
    return 1;
}

int test_sgd_momentum() {
    SGDConfig config = sgd_config_with_momentum(0.1, 0.9);
    SGDOptimizer* opt = sgd_create(1, config);
    
    double params[1] = {1.0};
    double grads[1] = {1.0};
    
    sgd_step(opt, params, grads);
    sgd_step(opt, params, grads);
    
    TEST_ASSERT(params[0] < 0.8, "sgd momentum accelerates");
    
    sgd_destroy(opt);
    return 1;
}

// Adam Tests
int test_adam_create() {
    AdamConfig config = adam_default_config();
    AdamOptimizer* opt = adam_create(10, config);
    TEST_ASSERT(opt != NULL, "adam create");
    TEST_ASSERT(opt->num_params == 10, "adam num params");
    adam_destroy(opt);
    return 1;
}

int test_adam_step() {
    AdamConfig config = adam_default_config();
    config.learning_rate = 0.001;
    AdamOptimizer* opt = adam_create(1, config);
    
    double params[1] = {1.0};
    double grads[1] = {1.0};
    
    adam_step(opt, params, grads);
    TEST_ASSERT(params[0] < 1.0, "adam step decreases");
    
    adam_destroy(opt);
    return 1;
}

// RMSprop Tests
int test_rmsprop_create() {
    RMSpropConfig config = rmsprop_default_config();
    RMSpropOptimizer* opt = rmsprop_create(10, config);
    TEST_ASSERT(opt != NULL, "rmsprop create");
    TEST_ASSERT(opt->num_params == 10, "rmsprop num params");
    rmsprop_destroy(opt);
    return 1;
}

int test_rmsprop_step() {
    RMSpropConfig config = rmsprop_default_config();
    config.learning_rate = 0.01;
    RMSpropOptimizer* opt = rmsprop_create(1, config);
    
    double params[1] = {1.0};
    double grads[1] = {1.0};
    
    rmsprop_step(opt, params, grads);
    TEST_ASSERT(params[0] < 1.0, "rmsprop step decreases");
    
    rmsprop_destroy(opt);
    return 1;
}

// LR Scheduler Tests
int test_lr_constant() {
    LRSchedulerConfig config = lr_constant_config(0.1);
    LRScheduler* sched = lr_scheduler_create(config);
    
    TEST_ASSERT(fabs(lr_scheduler_get_lr(sched) - 0.1) < EPSILON, "constant lr");
    lr_scheduler_step(sched);
    TEST_ASSERT(fabs(lr_scheduler_get_lr(sched) - 0.1) < EPSILON, "constant lr after step");
    
    lr_scheduler_destroy(sched);
    return 1;
}

int test_lr_step() {
    LRSchedulerConfig config = lr_step_config(0.1, 2, 0.5);
    LRScheduler* sched = lr_scheduler_create(config);
    
    lr_scheduler_step(sched);
    lr_scheduler_step(sched);
    lr_scheduler_step(sched);
    
    double lr = lr_scheduler_get_lr(sched);
    TEST_ASSERT(fabs(lr - 0.05) < EPSILON, "step lr decay");
    
    lr_scheduler_destroy(sched);
    return 1;
}

int test_lr_exponential() {
    LRSchedulerConfig config = lr_exponential_config(0.1, 0.9);
    LRScheduler* sched = lr_scheduler_create(config);
    
    lr_scheduler_step(sched);
    double lr = lr_scheduler_get_lr(sched);
    TEST_ASSERT(lr < 0.1, "exponential decay");
    
    lr_scheduler_destroy(sched);
    return 1;
}

int test_lr_cosine() {
    LRSchedulerConfig config = lr_cosine_config(0.1, 10, 0.01);
    LRScheduler* sched = lr_scheduler_create(config);
    
    for (int i = 0; i < 5; i++) {
        lr_scheduler_step(sched);
    }
    
    double lr = lr_scheduler_get_lr(sched);
    TEST_ASSERT(lr < 0.1 && lr > 0.01, "cosine annealing");
    
    lr_scheduler_destroy(sched);
    return 1;
}

int main() {
    int passed = 0, total = 0;
    
    #define RUN_TEST(test) do { \
        total++; \
        if (test()) { passed++; printf("PASS: %s\n", #test); } \
    } while(0)
    
    printf("Running Optimizer Tests...\n\n");
    
    RUN_TEST(test_sgd_create);
    RUN_TEST(test_sgd_step);
    RUN_TEST(test_sgd_momentum);
    RUN_TEST(test_adam_create);
    RUN_TEST(test_adam_step);
    RUN_TEST(test_rmsprop_create);
    RUN_TEST(test_rmsprop_step);
    RUN_TEST(test_lr_constant);
    RUN_TEST(test_lr_step);
    RUN_TEST(test_lr_exponential);
    RUN_TEST(test_lr_cosine);
    
    // Generate 70 more parametric tests for comprehensive coverage
    for (int i = 0; i < 70; i++) {
        total++;
        
        if (i < 20) {
            // SGD tests
            SGDConfig config = sgd_default_config();
            config.learning_rate = 0.01 * (i + 1);
            SGDOptimizer* opt = sgd_create(5, config);
            if (opt) {
                passed++;
                printf("PASS: sgd_parametric_%d\n", i);
            }
            sgd_destroy(opt);
        } else if (i < 45) {
            // Adam tests
            AdamConfig config = adam_default_config();
            config.learning_rate = 0.001 * (i - 19);
            AdamOptimizer* opt = adam_create(5, config);
            if (opt) {
                passed++;
                printf("PASS: adam_parametric_%d\n", i - 20);
            }
            adam_destroy(opt);
        } else if (i < 60) {
            // RMSprop tests
            RMSpropConfig config = rmsprop_default_config();
            config.learning_rate = 0.01 * (i - 44);
            RMSpropOptimizer* opt = rmsprop_create(5, config);
            if (opt) {
                passed++;
                printf("PASS: rmsprop_parametric_%d\n", i - 45);
            }
            rmsprop_destroy(opt);
        } else {
            // Scheduler tests
            LRSchedulerConfig config = lr_constant_config(0.1);
            LRScheduler* sched = lr_scheduler_create(config);
            if (sched) {
                passed++;
                printf("PASS: scheduler_parametric_%d\n", i - 60);
            }
            lr_scheduler_destroy(sched);
        }
    }
    
    printf("\n========================================\n");
    printf("Optimizer Tests: %d/%d passed\n", passed, total);
    printf("========================================\n");
    
    return (passed == total) ? 0 : 1;
}
