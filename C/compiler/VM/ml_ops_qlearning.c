/**
 * @file ml_ops_qlearning.c
 * @brief Q-Learning VM Operations Extension
 */

#include "ml_ops.h"
#include "../AIBase/reinforcement/q_learning.h"
#include <stdlib.h>

// Q-Learning VM opcodes
#define OP_QLEARN_CREATE       0x40
#define OP_QLEARN_UPDATE       0x41
#define OP_QLEARN_SELECT_ACTION 0x42
#define OP_QLEARN_GET_Q_VALUE  0x43
#define OP_QLEARN_TRAIN_EPISODE 0x44

// Execute Q-learning action selection
bool ml_vm_execute_qlearning_select_action(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    if (!vm || !ml_ctx) return false;
    
    // Get Q-learning model
    if (model_idx >= ml_ctx->num_models) return false;
    
    MLModelHandle* handle = &ml_ctx->models[model_idx];
    if (handle->model_type != 4) return false; // Not Q-learning
    
    QLearningModel* model = (QLearningModel*)handle->model;
    if (!model) return false;
    
    // Pop state from stack
    size_t state = (size_t)vm_stack_pop(vm);
    
    // Select action using epsilon-greedy policy
    size_t action = qlearning_select_action(model, state);
    
    // Push action to stack
    vm_stack_push(vm, (double)action);
    
    return true;
}

// Execute Q-learning greedy action selection (no exploration)
bool ml_vm_execute_qlearning_predict(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    if (!vm || !ml_ctx) return false;
    
    // Get Q-learning model
    if (model_idx >= ml_ctx->num_models) return false;
    
    MLModelHandle* handle = &ml_ctx->models[model_idx];
    if (handle->model_type != 4) return false;
    
    QLearningModel* model = (QLearningModel*)handle->model;
    if (!model) return false;
    
    // Pop state from stack
    size_t state = (size_t)vm_stack_pop(vm);
    
    // Select best action (greedy)
    size_t action = qlearning_select_greedy_action(model, state);
    
    // Push action to stack
    vm_stack_push(vm, (double)action);
    
    return true;
}

// Execute Q-learning update
bool ml_vm_execute_qlearning_update(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    if (!vm || !ml_ctx) return false;
    
    // Get Q-learning model
    if (model_idx >= ml_ctx->num_models) return false;
    
    MLModelHandle* handle = &ml_ctx->models[model_idx];
    if (handle->model_type != 4) return false;
    
    QLearningModel* model = (QLearningModel*)handle->model;
    if (!model) return false;
    
    // Pop experience from stack: done, next_state, reward, action, state
    bool done = (bool)vm_stack_pop(vm);
    size_t next_state = (size_t)vm_stack_pop(vm);
    double reward = vm_stack_pop(vm);
    size_t action = (size_t)vm_stack_pop(vm);
    size_t state = (size_t)vm_stack_pop(vm);
    
    // Create experience
    Experience exp = {
        .state = state,
        .action = action,
        .reward = reward,
        .next_state = next_state,
        .done = done
    };
    
    // Update Q-value
    qlearning_update(model, &exp);
    
    return true;
}

// Execute Q-learning get Q-value
bool ml_vm_execute_qlearning_get_q_value(VM* vm, MLVMContext* ml_ctx, uint8_t model_idx) {
    if (!vm || !ml_ctx) return false;
    
    // Get Q-learning model
    if (model_idx >= ml_ctx->num_models) return false;
    
    MLModelHandle* handle = &ml_ctx->models[model_idx];
    if (handle->model_type != 4) return false;
    
    QLearningModel* model = (QLearningModel*)handle->model;
    if (!model) return false;
    
    // Pop action and state from stack
    size_t action = (size_t)vm_stack_pop(vm);
    size_t state = (size_t)vm_stack_pop(vm);
    
    // Get Q-value
    double q_value = qlearning_get_q_value(model, state, action);
    
    // Push Q-value to stack
    vm_stack_push(vm, q_value);
    
    return true;
}

// Register Q-learning model in VM context
bool ml_vm_register_qlearning(MLVMContext* ctx, QLearningModel* model) {
    if (!ctx || !model) return false;
    
    if (ctx->num_models >= ctx->capacity) {
        // Resize
        size_t new_capacity = ctx->capacity * 2;
        MLModelHandle* new_models = realloc(ctx->models, new_capacity * sizeof(MLModelHandle));
        if (!new_models) return false;
        
        ctx->models = new_models;
        ctx->capacity = new_capacity;
    }
    
    ctx->models[ctx->num_models].model = model;
    ctx->models[ctx->num_models].model_type = 4; // Q-learning
    ctx->num_models++;
    
    return true;
}

// Get Q-learning model from VM context
QLearningModel* ml_vm_get_qlearning(MLVMContext* ctx, uint8_t model_idx) {
    if (!ctx || model_idx >= ctx->num_models) return nullptr;
    
    MLModelHandle* handle = &ctx->models[model_idx];
    if (handle->model_type != 4) return nullptr;
    
    return (QLearningModel*)handle->model;
}
