
/**
 * @file pbm.h
 * @brief Predictive Bug Modeling - Phase 3 Component
 * 
 * ML-based bug prediction using historical data:
 * - Code complexity metrics analysis
 * - Historical bug pattern learning
 * - Code change frequency tracking
 * - Module dependency analysis
 * - Coding style pattern recognition
 * - Risk scoring and recommendations
 */

#ifndef CRRSS_PBM_H
#define CRRSS_PBM_H

#include "../common/crrss_types.h"
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PBM_MAX_FEATURES 50
#define PBM_MAX_MODEL_NAME 128
#define PBM_MAX_PATH_LEN 512
#define PBM_MAX_RECOMMENDATIONS 10

// Feature vector for ML model
typedef struct {
    // Code complexity features
    uint32_t cyclomatic_complexity;
    uint32_t lines_of_code;
    uint32_t function_count;
    uint32_t nesting_depth;
    uint32_t parameter_count;
    
    // Historical pattern features
    uint32_t bug_history_count;
    double bug_density;  // bugs per KLOC
    uint32_t fix_frequency;  // how often this area gets fixed
    
    // Change frequency features
    uint32_t commit_count;
    uint32_t author_count;
    uint32_t change_churn;  // lines added + deleted
    
    // Dependency features
    uint32_t dependency_count;
    uint32_t coupling_score;
    uint32_t cohesion_score;
    
    // Style features
    double style_consistency;
    uint32_t naming_violations;
    uint32_t formatting_issues;
    
    // Custom features (extensible)
    double custom_features[PBM_MAX_FEATURES];
    uint32_t custom_feature_count;
} pbm_feature_vector_t;

// Bug prediction result
typedef struct {
    char file_path[PBM_MAX_PATH_LEN];
    char function_name[256];
    uint32_t line_number;
    
    double risk_score;  // 0.0 - 1.0
    double confidence;  // 0.0 - 1.0
    
    bug_category_t predicted_category;
    bug_priority_t predicted_priority;
    
    const char* reason;  // Why this prediction was made
    pbm_feature_vector_t features;
} pbm_prediction_t;

// Model type
typedef enum {
    PBM_MODEL_LINEAR_REGRESSION = 0,
    PBM_MODEL_DECISION_TREE = 1,
    PBM_MODEL_RANDOM_FOREST = 2,
    PBM_MODEL_NEURAL_NETWORK = 3,
    PBM_MODEL_ENSEMBLE = 4  // Combination of multiple models
} pbm_model_type_t;

// Training data point
typedef struct {
    pbm_feature_vector_t features;
    bool has_bug;  // Ground truth
    bug_category_t bug_category;
    bug_priority_t bug_priority;
} pbm_training_data_t;

// Model metadata
typedef struct {
    char model_name[PBM_MAX_MODEL_NAME];
    pbm_model_type_t type;
    uint32_t training_samples;
    double accuracy;
    double precision;
    double recall;
    double f1_score;
    time_t trained_at;
    time_t last_updated;
} pbm_model_metadata_t;

// Recommendation
typedef struct {
    char description[512];
    bug_priority_t priority;
    double impact_score;
    const char* suggested_action;
} pbm_recommendation_t;

// Configuration
typedef struct {
    pbm_model_type_t model_type;
    const char* model_path;  // Path to load/save model
    bool enable_online_learning;  // Learn from new data
    bool enable_ensemble;  // Use multiple models
    double confidence_threshold;  // Minimum confidence for predictions
    uint32_t max_predictions;
    const char* training_data_path;
    bool auto_retrain;  // Automatically retrain on new data
    uint32_t retrain_threshold;  // Samples needed before retrain
} pbm_config_t;

// PBM context
typedef struct pbm_context pbm_context_t;

/**
 * @brief Initialize PBM system
 * @param config Configuration options
 * @return PBM context or NULL on failure
 */
pbm_context_t* pbm_initialize(const pbm_config_t* config);

/**
 * @brief Shutdown PBM system
 * @param ctx PBM context
 */
void pbm_shutdown(pbm_context_t* ctx);

/**
 * @brief Extract features from code file
 * @param ctx PBM context
 * @param file_path Source file path
 * @param features Output feature vector
 * @return Status code
 */
crrss_status_t pbm_extract_features(
    pbm_context_t* ctx,
    const char* file_path,
    pbm_feature_vector_t* features
);

/**
 * @brief Predict bugs in a file
 * @param ctx PBM context
 * @param file_path Source file path
 * @param predictions Output predictions array
 * @param max_predictions Maximum predictions to return
 * @param count Output prediction count
 * @return Status code
 */
crrss_status_t pbm_predict_file(
    pbm_context_t* ctx,
    const char* file_path,
    pbm_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* count
);

/**
 * @brief Predict bugs in a directory
 * @param ctx PBM context
 * @param directory_path Directory path
 * @param predictions Output predictions array
 * @param max_predictions Maximum predictions to return
 * @param count Output prediction count
 * @return Status code
 */
crrss_status_t pbm_predict_directory(
    pbm_context_t* ctx,
    const char* directory_path,
    pbm_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* count
);

/**
 * @brief Calculate risk score for a code section
 * @param ctx PBM context
 * @param features Feature vector
 * @param risk_score Output risk score
 * @param confidence Output confidence
 * @return Status code
 */
crrss_status_t pbm_calculate_risk(
    pbm_context_t* ctx,
    const pbm_feature_vector_t* features,
    double* risk_score,
    double* confidence
);

/**
 * @brief Generate recommendations based on predictions
 * @param ctx PBM context
 * @param predictions Input predictions array
 * @param prediction_count Number of predictions
 * @param recommendations Output recommendations array
 * @param max_recommendations Maximum recommendations to return
 * @param count Output recommendation count
 * @return Status code
 */
crrss_status_t pbm_generate_recommendations(
    pbm_context_t* ctx,
    const pbm_prediction_t* predictions,
    uint32_t prediction_count,
    pbm_recommendation_t* recommendations,
    uint32_t max_recommendations,
    uint32_t* count
);

/**
 * @brief Train model with new data
 * @param ctx PBM context
 * @param training_data Training data array
 * @param data_count Number of training samples
 * @return Status code
 */
crrss_status_t pbm_train_model(
    pbm_context_t* ctx,
    const pbm_training_data_t* training_data,
    uint32_t data_count
);

/**
 * @brief Update model with online learning
 * @param ctx PBM context
 * @param features Feature vector
 * @param has_bug Ground truth
 * @param category Bug category
 * @param priority Bug priority
 * @return Status code
 */
crrss_status_t pbm_update_model(
    pbm_context_t* ctx,
    const pbm_feature_vector_t* features,
    bool has_bug,
    bug_category_t category,
    bug_priority_t priority
);

/**
 * @brief Load model from file
 * @param ctx PBM context
 * @param model_path Model file path
 * @return Status code
 */
crrss_status_t pbm_load_model(pbm_context_t* ctx, const char* model_path);

/**
 * @brief Save model to file
 * @param ctx PBM context
 * @param model_path Model file path
 * @return Status code
 */
crrss_status_t pbm_save_model(pbm_context_t* ctx, const char* model_path);

/**
 * @brief Get model metadata
 * @param ctx PBM context
 * @param metadata Output metadata
 * @return Status code
 */
crrss_status_t pbm_get_model_metadata(
    pbm_context_t* ctx,
    pbm_model_metadata_t* metadata
);

/**
 * @brief Evaluate model performance
 * @param ctx PBM context
 * @param test_data Test data array
 * @param data_count Number of test samples
 * @param accuracy Output accuracy
 * @param precision Output precision
 * @param recall Output recall
 * @param f1_score Output F1 score
 * @return Status code
 */
crrss_status_t pbm_evaluate_model(
    pbm_context_t* ctx,
    const pbm_training_data_t* test_data,
    uint32_t data_count,
    double* accuracy,
    double* precision,
    double* recall,
    double* f1_score
);

/**
 * @brief Get feature importance scores
 * @param ctx PBM context
 * @param importance Output importance scores
 * @param feature_count Number of features
 * @return Status code
 */
crrss_status_t pbm_get_feature_importance(
    pbm_context_t* ctx,
    double* importance,
    uint32_t feature_count
);

#ifdef __cplusplus
}
#endif

#endif // CRRSS_PBM_H
