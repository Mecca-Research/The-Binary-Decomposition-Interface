/**
 * @file ml_codegen_qlearning.c
 * @brief Q-Learning Code Generation Extension
 */

#include "ml_codegen.h"
#include "../AIBase/reinforcement/q_learning.h"
#include <stdlib.h>
#include <string.h>

// Serialize Q-Learning Model
SerializedModel* serialize_qlearning(const QLearningModel* model) {
    if (!model || !model->fitted) {
        return nullptr;
    }
    
    SerializedModel* serialized = malloc(sizeof(SerializedModel));
    if (!serialized) return nullptr;
    
    // Calculate size: metadata + Q-table entries
    size_t q_table_size = model->q_table_size * sizeof(QTableEntry);
    size_t data_size = sizeof(size_t) * 3 + // n_states, n_actions, q_table_size
                      sizeof(double) * 5 +   // alpha, gamma, epsilon, epsilon_decay, epsilon_min
                      q_table_size;
    
    serialized->model_type = 4; // Q-learning
    serialized->data_size = data_size;
    serialized->data = malloc(data_size);
    
    if (!serialized->data) {
        free(serialized);
        return nullptr;
    }
    
    // Pack data
    uint8_t* ptr = serialized->data;
    
    memcpy(ptr, &model->n_states, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->n_actions, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->q_table_size, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(ptr, &model->alpha, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(ptr, &model->gamma, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(ptr, &model->epsilon, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(ptr, &model->epsilon_decay, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(ptr, &model->epsilon_min, sizeof(double));
    ptr += sizeof(double);
    
    // Export Q-table
    qlearning_export_q_table(model, (QTableEntry*)ptr, model->q_table_size);
    
    return serialized;
}

// Deserialize Q-Learning Model
QLearningModel* deserialize_qlearning(const SerializedModel* data) {
    if (!data || data->model_type != 4) {
        return nullptr;
    }
    
    const uint8_t* ptr = data->data;
    
    size_t n_states, n_actions, q_table_size;
    memcpy(&n_states, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(&n_actions, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    memcpy(&q_table_size, ptr, sizeof(size_t));
    ptr += sizeof(size_t);
    
    QLearningConfig config = qlearning_default_config();
    
    memcpy(&config.alpha, ptr, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(&config.gamma, ptr, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(&config.epsilon, ptr, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(&config.epsilon_decay, ptr, sizeof(double));
    ptr += sizeof(double);
    
    memcpy(&config.epsilon_min, ptr, sizeof(double));
    ptr += sizeof(double);
    
    QLearningModel* model = qlearning_create(n_states, n_actions, config);
    if (!model) return nullptr;
    
    // Import Q-table entries
    const QTableEntry* entries = (const QTableEntry*)ptr;
    for (size_t i = 0; i < q_table_size; i++) {
        qlearning_set_q_value(model, entries[i].state, entries[i].action, entries[i].q_value);
    }
    
    model->fitted = true;
    
    return model;
}

// Emit Q-Learning Model
bool emit_qlearning_model(Chunk* chunk, const QLearningModel* model) {
    if (!chunk || !model) {
        return false;
    }
    
    SerializedModel* serialized = serialize_qlearning(model);
    if (!serialized) {
        return false;
    }
    
    // Emit bytecode for Q-learning model
    // This would integrate with the VM's bytecode format
    
    free(serialized->data);
    free(serialized);
    
    return true;
}
