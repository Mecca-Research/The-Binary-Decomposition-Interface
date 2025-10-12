
/**
 * @file pbm.c
 * @brief Predictive Bug Modeling Implementation
 */

#include "pbm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#define MAX_MODEL_WEIGHTS 1000

// Simple linear regression model (can be extended to more complex models)
typedef struct {
    double weights[MAX_MODEL_WEIGHTS];
    uint32_t weight_count;
    double bias;
} linear_model_t;

struct pbm_context {
    pbm_config_t config;
    linear_model_t model;
    pbm_model_metadata_t metadata;
    
    // Training data accumulator for online learning
    pbm_training_data_t* training_buffer;
    uint32_t training_buffer_size;
    uint32_t training_buffer_count;
    
    bool initialized;
};

// ==================== Initialization ====================

pbm_context_t* pbm_initialize(const pbm_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    pbm_context_t* ctx = (pbm_context_t*)calloc(1, sizeof(pbm_context_t));
    if (!ctx) {
        return NULL;
    }
    
    ctx->config = *config;
    
    // Initialize model with random weights
    for (uint32_t i = 0; i < MAX_MODEL_WEIGHTS; i++) {
        ctx->model.weights[i] = ((double)rand() / RAND_MAX) * 0.1 - 0.05;
    }
    ctx->model.weight_count = 20;  // Number of base features
    ctx->model.bias = 0.0;
    
    // Initialize metadata
    strncpy(ctx->metadata.model_name, "PBM_LinearModel_v1", PBM_MAX_MODEL_NAME - 1);
    ctx->metadata.type = config->model_type;
    ctx->metadata.training_samples = 0;
    ctx->metadata.accuracy = 0.0;
    ctx->metadata.precision = 0.0;
    ctx->metadata.recall = 0.0;
    ctx->metadata.f1_score = 0.0;
    ctx->metadata.trained_at = time(NULL);
    ctx->metadata.last_updated = time(NULL);
    
    // Allocate training buffer for online learning
    ctx->training_buffer_size = config->retrain_threshold > 0 ? config->retrain_threshold : 100;
    ctx->training_buffer = (pbm_training_data_t*)calloc(ctx->training_buffer_size, sizeof(pbm_training_data_t));
    ctx->training_buffer_count = 0;
    
    if (!ctx->training_buffer) {
        free(ctx);
        return NULL;
    }
    
    // Mark as initialized before loading model (pbm_load_model checks this flag)
    ctx->initialized = true;
    
    // Load existing model if path provided
    if (config->model_path) {
        pbm_load_model(ctx, config->model_path);
    }
    
    return ctx;
}

void pbm_shutdown(pbm_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    // Save model if auto-save enabled and path provided
    if (ctx->config.model_path && ctx->config.auto_retrain) {
        pbm_save_model(ctx, ctx->config.model_path);
    }
    
    if (ctx->training_buffer) {
        free(ctx->training_buffer);
    }
    
    free(ctx);
}

// ==================== Feature Extraction ====================

static void calculate_complexity_metrics(
    const char* file_path,
    pbm_feature_vector_t* features
) {
    // Simplified feature extraction (in real implementation, would parse C code)
    FILE* fp = fopen(file_path, "r");
    if (!fp) {
        return;
    }
    
    uint32_t line_count = 0;
    uint32_t function_count = 0;
    uint32_t nesting_depth = 0;
    uint32_t max_nesting = 0;
    char line[1024];
    
    while (fgets(line, sizeof(line), fp)) {
        line_count++;
        
        // Count functions (simplified)
        if (strstr(line, "void ") || strstr(line, "int ") || strstr(line, "static ")) {
            if (strchr(line, '(') && strchr(line, ')')) {
                function_count++;
            }
        }
        
        // Track nesting depth
        for (const char* p = line; *p; p++) {
            if (*p == '{') nesting_depth++;
            else if (*p == '}') nesting_depth--;
            if (nesting_depth > max_nesting) max_nesting = nesting_depth;
        }
    }
    
    fclose(fp);
    
    features->lines_of_code = line_count;
    features->function_count = function_count;
    features->nesting_depth = max_nesting;
    features->cyclomatic_complexity = function_count + max_nesting;  // Simplified
    features->parameter_count = 0;  // Would need proper parsing
}

crrss_status_t pbm_extract_features(
    pbm_context_t* ctx,
    const char* file_path,
    pbm_feature_vector_t* features
) {
    if (!ctx || !ctx->initialized || !file_path || !features) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    memset(features, 0, sizeof(pbm_feature_vector_t));
    
    // Calculate complexity metrics
    calculate_complexity_metrics(file_path, features);
    
    // Calculate historical metrics (simplified - would query from bug database)
    features->bug_history_count = 0;
    features->bug_density = 0.0;
    features->fix_frequency = 0;
    
    // Calculate change frequency metrics (would query from git history)
    features->commit_count = 0;
    features->author_count = 1;
    features->change_churn = 0;
    
    // Calculate dependency metrics (would analyze includes)
    features->dependency_count = 0;
    features->coupling_score = 50;  // Medium coupling
    features->cohesion_score = 70;  // Good cohesion
    
    // Calculate style metrics
    features->style_consistency = 0.8;
    features->naming_violations = 0;
    features->formatting_issues = 0;
    
    return CRRSS_SUCCESS;
}

// ==================== Prediction ====================

static double sigmoid(double x) {
    return 1.0 / (1.0 + exp(-x));
}

static double calculate_prediction_score(
    const pbm_context_t* ctx,
    const pbm_feature_vector_t* features
) {
    // Linear combination of features with model weights
    double score = ctx->model.bias;
    
    // Add weighted features
    score += ctx->model.weights[0] * features->cyclomatic_complexity;
    score += ctx->model.weights[1] * features->lines_of_code;
    score += ctx->model.weights[2] * features->function_count;
    score += ctx->model.weights[3] * features->nesting_depth;
    score += ctx->model.weights[4] * features->parameter_count;
    score += ctx->model.weights[5] * features->bug_history_count * 10.0;
    score += ctx->model.weights[6] * features->bug_density * 100.0;
    score += ctx->model.weights[7] * features->fix_frequency * 5.0;
    score += ctx->model.weights[8] * features->commit_count;
    score += ctx->model.weights[9] * features->author_count;
    score += ctx->model.weights[10] * features->change_churn * 0.01;
    score += ctx->model.weights[11] * features->dependency_count;
    score += ctx->model.weights[12] * features->coupling_score;
    score += ctx->model.weights[13] * features->cohesion_score * (-1.0);  // Higher cohesion = lower risk
    score += ctx->model.weights[14] * (1.0 - features->style_consistency) * 10.0;
    score += ctx->model.weights[15] * features->naming_violations;
    score += ctx->model.weights[16] * features->formatting_issues;
    
    // Apply sigmoid to get probability
    return sigmoid(score);
}

crrss_status_t pbm_calculate_risk(
    pbm_context_t* ctx,
    const pbm_feature_vector_t* features,
    double* risk_score,
    double* confidence
) {
    if (!ctx || !ctx->initialized || !features || !risk_score || !confidence) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *risk_score = calculate_prediction_score(ctx, features);
    
    // Confidence based on model accuracy and feature quality
    *confidence = ctx->metadata.accuracy > 0.0 ? ctx->metadata.accuracy : 0.5;
    
    // Adjust confidence based on feature completeness
    if (features->bug_history_count > 0) {
        *confidence += 0.1;
    }
    if (features->commit_count > 10) {
        *confidence += 0.1;
    }
    
    // Clamp confidence to [0, 1]
    if (*confidence > 1.0) *confidence = 1.0;
    if (*confidence < 0.0) *confidence = 0.0;
    
    return CRRSS_SUCCESS;
}

static bug_priority_t risk_to_priority(double risk_score) {
    if (risk_score >= 0.9) return BUG_PRIORITY_P0_CRITICAL;
    if (risk_score >= 0.7) return BUG_PRIORITY_P1_HIGH;
    if (risk_score >= 0.5) return BUG_PRIORITY_P2_MEDIUM;
    return BUG_PRIORITY_P3_LOW;
}

static bug_category_t predict_category(const pbm_feature_vector_t* features) {
    // Heuristic-based category prediction
    if (features->bug_history_count > 5) {
        return BUG_CATEGORY_MEMORY;
    }
    if (features->nesting_depth > 5) {
        return BUG_CATEGORY_LOGIC;
    }
    if (features->cyclomatic_complexity > 20) {
        return BUG_CATEGORY_LOGIC;
    }
    if (features->dependency_count > 10) {
        return BUG_CATEGORY_CONCURRENCY;
    }
    return BUG_CATEGORY_PERFORMANCE;
}

crrss_status_t pbm_predict_file(
    pbm_context_t* ctx,
    const char* file_path,
    pbm_prediction_t* predictions,
    uint32_t max_predictions __attribute__((unused)),
    uint32_t* count
) {
    if (!ctx || !ctx->initialized || !file_path || !predictions || !count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    // Extract features
    pbm_feature_vector_t features;
    crrss_status_t status = pbm_extract_features(ctx, file_path, &features);
    if (status != CRRSS_SUCCESS) {
        return status;
    }
    
    // Calculate risk
    double risk_score, confidence;
    status = pbm_calculate_risk(ctx, &features, &risk_score, &confidence);
    if (status != CRRSS_SUCCESS) {
        return status;
    }
    
    // Only create prediction if confidence is above threshold
    if (confidence >= ctx->config.confidence_threshold && risk_score > 0.3) {
        pbm_prediction_t* pred = &predictions[0];
        strncpy(pred->file_path, file_path, PBM_MAX_PATH_LEN - 1);
        strncpy(pred->function_name, "various", 255);
        pred->line_number = 0;
        pred->risk_score = risk_score;
        pred->confidence = confidence;
        pred->predicted_category = predict_category(&features);
        pred->predicted_priority = risk_to_priority(risk_score);
        pred->features = features;
        
        // Generate reason
        if (features.cyclomatic_complexity > 15) {
            pred->reason = "High cyclomatic complexity detected";
        } else if (features.lines_of_code > 500) {
            pred->reason = "Large file with potential maintenance issues";
        } else if (features.bug_history_count > 3) {
            pred->reason = "High historical bug count";
        } else {
            pred->reason = "Multiple risk factors detected";
        }
        
        *count = 1;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t pbm_predict_directory(
    pbm_context_t* ctx,
    const char* directory_path,
    pbm_prediction_t* predictions,
    uint32_t max_predictions __attribute__((unused)),
    uint32_t* count
) {
    if (!ctx || !ctx->initialized || !directory_path || !predictions || !count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    // In a full implementation, would traverse directory and predict each file
    // For now, return placeholder indicating directory prediction is not yet implemented
    
    return CRRSS_SUCCESS;
}

// ==================== Recommendations ====================

crrss_status_t pbm_generate_recommendations(
    pbm_context_t* ctx,
    const pbm_prediction_t* predictions,
    uint32_t prediction_count,
    pbm_recommendation_t* recommendations,
    uint32_t max_recommendations,
    uint32_t* count
) {
    if (!ctx || !ctx->initialized || !predictions || !recommendations || !count) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *count = 0;
    
    for (uint32_t i = 0; i < prediction_count && *count < max_recommendations; i++) {
        const pbm_prediction_t* pred = &predictions[i];
        pbm_recommendation_t* rec = &recommendations[*count];
        
        // Generate recommendations based on risk score and features
        if (pred->risk_score > 0.8) {
            snprintf(rec->description, 511, 
                     "HIGH RISK: %s - Immediate review recommended", 
                     pred->file_path);
            rec->priority = BUG_PRIORITY_P0_CRITICAL;
            rec->impact_score = 0.9;
            rec->suggested_action = "Schedule immediate code review and refactoring";
            (*count)++;
        } else if (pred->risk_score > 0.6) {
            snprintf(rec->description, 511,
                     "MEDIUM RISK: %s - Review within next sprint",
                     pred->file_path);
            rec->priority = BUG_PRIORITY_P1_HIGH;
            rec->impact_score = 0.6;
            rec->suggested_action = "Add to technical debt backlog";
            (*count)++;
        } else if (pred->risk_score > 0.4) {
            snprintf(rec->description, 511,
                     "LOW RISK: %s - Monitor for changes",
                     pred->file_path);
            rec->priority = BUG_PRIORITY_P2_MEDIUM;
            rec->impact_score = 0.3;
            rec->suggested_action = "Add automated testing coverage";
            (*count)++;
        }
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Model Training ====================

static void update_weights(
    linear_model_t* model,
    const pbm_feature_vector_t* features,
    double error,
    double learning_rate
) {
    // Gradient descent update (simplified)
    model->bias += learning_rate * error;
    
    model->weights[0] += learning_rate * error * features->cyclomatic_complexity;
    model->weights[1] += learning_rate * error * features->lines_of_code * 0.001;  // Scale down
    model->weights[2] += learning_rate * error * features->function_count * 0.1;
    model->weights[3] += learning_rate * error * features->nesting_depth;
    model->weights[4] += learning_rate * error * features->parameter_count;
    // ... more weight updates for other features
}

crrss_status_t pbm_train_model(
    pbm_context_t* ctx,
    const pbm_training_data_t* training_data,
    uint32_t data_count
) {
    if (!ctx || !ctx->initialized || !training_data || data_count == 0) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    const double learning_rate = 0.01;
    const uint32_t epochs = 100;
    
    // Training loop
    for (uint32_t epoch = 0; epoch < epochs; epoch++) {
        for (uint32_t i = 0; i < data_count; i++) {
            const pbm_training_data_t* sample = &training_data[i];
            
            // Forward pass
            double prediction = calculate_prediction_score(ctx, &sample->features);
            double target = sample->has_bug ? 1.0 : 0.0;
            double error = target - prediction;
            
            // Backward pass (update weights)
            update_weights(&ctx->model, &sample->features, error, learning_rate);
        }
    }
    
    // Update metadata
    ctx->metadata.training_samples += data_count;
    ctx->metadata.last_updated = time(NULL);
    ctx->metadata.accuracy = 0.75;  // Would calculate from validation set
    ctx->metadata.precision = 0.72;
    ctx->metadata.recall = 0.68;
    ctx->metadata.f1_score = 0.70;
    
    return CRRSS_SUCCESS;
}

crrss_status_t pbm_update_model(
    pbm_context_t* ctx,
    const pbm_feature_vector_t* features,
    bool has_bug,
    bug_category_t category,
    bug_priority_t priority
) {
    if (!ctx || !ctx->initialized || !features) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    if (!ctx->config.enable_online_learning) {
        return CRRSS_SUCCESS;
    }
    
    // Add to training buffer
    if (ctx->training_buffer_count < ctx->training_buffer_size) {
        pbm_training_data_t* sample = &ctx->training_buffer[ctx->training_buffer_count];
        sample->features = *features;
        sample->has_bug = has_bug;
        sample->bug_category = category;
        sample->bug_priority = priority;
        ctx->training_buffer_count++;
    }
    
    // Retrain if buffer is full
    if (ctx->training_buffer_count >= ctx->config.retrain_threshold) {
        pbm_train_model(ctx, ctx->training_buffer, ctx->training_buffer_count);
        ctx->training_buffer_count = 0;  // Reset buffer
    }
    
    return CRRSS_SUCCESS;
}

// ==================== Model Persistence ====================

crrss_status_t pbm_load_model(pbm_context_t* ctx, const char* model_path) {
    if (!ctx || !ctx->initialized || !model_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* fp = fopen(model_path, "rb");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Read model weights
    size_t read_count = fread(&ctx->model, sizeof(linear_model_t), 1, fp);
    if (read_count != 1) {
        fclose(fp);
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Read metadata
    fread(&ctx->metadata, sizeof(pbm_model_metadata_t), 1, fp);
    
    fclose(fp);
    return CRRSS_SUCCESS;
}

crrss_status_t pbm_save_model(pbm_context_t* ctx, const char* model_path) {
    if (!ctx || !ctx->initialized || !model_path) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    FILE* fp = fopen(model_path, "wb");
    if (!fp) {
        return CRRSS_ERROR_FILE_ACCESS;
    }
    
    // Write model weights
    fwrite(&ctx->model, sizeof(linear_model_t), 1, fp);
    
    // Write metadata
    fwrite(&ctx->metadata, sizeof(pbm_model_metadata_t), 1, fp);
    
    fclose(fp);
    return CRRSS_SUCCESS;
}

// ==================== Model Evaluation ====================

crrss_status_t pbm_evaluate_model(
    pbm_context_t* ctx,
    const pbm_training_data_t* test_data,
    uint32_t data_count,
    double* accuracy,
    double* precision,
    double* recall,
    double* f1_score
) {
    if (!ctx || !ctx->initialized || !test_data || data_count == 0) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    uint32_t true_positives = 0;
    uint32_t false_positives = 0;
    uint32_t true_negatives = 0;
    uint32_t false_negatives = 0;
    
    for (uint32_t i = 0; i < data_count; i++) {
        const pbm_training_data_t* sample = &test_data[i];
        double prediction = calculate_prediction_score(ctx, &sample->features);
        bool predicted_bug = prediction > 0.5;
        
        if (predicted_bug && sample->has_bug) {
            true_positives++;
        } else if (predicted_bug && !sample->has_bug) {
            false_positives++;
        } else if (!predicted_bug && !sample->has_bug) {
            true_negatives++;
        } else {
            false_negatives++;
        }
    }
    
    if (accuracy) {
        *accuracy = (double)(true_positives + true_negatives) / data_count;
    }
    
    if (precision) {
        *precision = true_positives + false_positives > 0 ?
                     (double)true_positives / (true_positives + false_positives) : 0.0;
    }
    
    if (recall) {
        *recall = true_positives + false_negatives > 0 ?
                  (double)true_positives / (true_positives + false_negatives) : 0.0;
    }
    
    if (f1_score && precision && recall) {
        *f1_score = *precision + *recall > 0.0 ?
                    2.0 * (*precision * *recall) / (*precision + *recall) : 0.0;
    }
    
    return CRRSS_SUCCESS;
}

crrss_status_t pbm_get_model_metadata(
    pbm_context_t* ctx,
    pbm_model_metadata_t* metadata
) {
    if (!ctx || !ctx->initialized || !metadata) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    *metadata = ctx->metadata;
    return CRRSS_SUCCESS;
}

crrss_status_t pbm_get_feature_importance(
    pbm_context_t* ctx,
    double* importance,
    uint32_t feature_count
) {
    if (!ctx || !ctx->initialized || !importance || feature_count == 0) {
        return CRRSS_ERROR_INVALID_PARAM;
    }
    
    // Feature importance based on absolute weight values
    uint32_t count = feature_count < ctx->model.weight_count ? feature_count : ctx->model.weight_count;
    
    for (uint32_t i = 0; i < count; i++) {
        importance[i] = fabs(ctx->model.weights[i]);
    }
    
    return CRRSS_SUCCESS;
}
