# CRRSS Phase 3 API Reference

## Error Heatmap Visualization (EHV)

### Types

```c
typedef enum {
    EHV_FORMAT_ASCII,
    EHV_FORMAT_JSON,
    EHV_FORMAT_HTML,
    EHV_FORMAT_CSV
} ehv_format_t;

typedef struct {
    char file_path[512];
    char function_name[256];
    uint32_t line_number;
    bug_category_t category;
    bug_priority_t priority;
    uint32_t frequency;
    double severity_score;
} ehv_error_location_t;
```

### Functions

**ehv_initialize**
```c
ehv_context_t* ehv_initialize(const ehv_config_t* config);
```
Initialize EHV system with configuration.

**ehv_record_error**
```c
crrss_status_t ehv_record_error(
    ehv_context_t* ctx,
    const char* file_path,
    const char* function_name,
    uint32_t line_number,
    bug_category_t category,
    bug_priority_t priority
);
```
Record an error occurrence.

**ehv_identify_hotspots**
```c
crrss_status_t ehv_identify_hotspots(
    ehv_context_t* ctx,
    ehv_hotspot_t* hotspots,
    uint32_t max_hotspots,
    uint32_t* count
);
```
Identify error hotspots.

**ehv_export_visualization**
```c
crrss_status_t ehv_export_visualization(
    ehv_context_t* ctx,
    ehv_format_t format,
    const char* output_path
);
```
Export heatmap visualization.

## Predictive Bug Modeling (PBM)

### Types

```c
typedef struct {
    uint32_t cyclomatic_complexity;
    uint32_t lines_of_code;
    uint32_t function_count;
    // ... more features
} pbm_feature_vector_t;

typedef struct {
    char file_path[512];
    double risk_score;
    double confidence;
    bug_category_t predicted_category;
    bug_priority_t predicted_priority;
    const char* reason;
} pbm_prediction_t;
```

### Functions

**pbm_initialize**
```c
pbm_context_t* pbm_initialize(const pbm_config_t* config);
```

**pbm_extract_features**
```c
crrss_status_t pbm_extract_features(
    pbm_context_t* ctx,
    const char* file_path,
    pbm_feature_vector_t* features
);
```

**pbm_predict_file**
```c
crrss_status_t pbm_predict_file(
    pbm_context_t* ctx,
    const char* file_path,
    pbm_prediction_t* predictions,
    uint32_t max_predictions,
    uint32_t* count
);
```

**pbm_train_model**
```c
crrss_status_t pbm_train_model(
    pbm_context_t* ctx,
    const pbm_training_data_t* training_data,
    uint32_t data_count
);
```

## Dependency Analysis (DEPS)

### Types

```c
typedef struct {
    char source_module[128];
    char target_module[128];
    uint32_t call_count;
    double coupling_strength;
} deps_relationship_t;

typedef struct {
    uint32_t cycle_length;
    char modules[10][128];
    double risk_score;
} deps_circular_dependency_t;
```

### Functions

**deps_analyze_directory**
```c
crrss_status_t deps_analyze_directory(
    deps_context_t* ctx,
    const char* directory_path
);
```

**deps_detect_circular**
```c
crrss_status_t deps_detect_circular(
    deps_context_t* ctx,
    deps_circular_dependency_t* circular_deps,
    uint32_t max_circular,
    uint32_t* count
);
```

**deps_export_visualization**
```c
crrss_status_t deps_export_visualization(
    deps_context_t* ctx,
    deps_format_t format,
    const char* output_path
);
```

## Fix Suggestions

### Types

```c
typedef enum {
    FIX_TYPE_BUFFER_OVERFLOW,
    FIX_TYPE_MEMORY_LEAK,
    FIX_TYPE_NULL_DEREF,
    FIX_TYPE_STYLE,
    FIX_TYPE_PERFORMANCE
} fix_type_t;

typedef struct {
    fix_type_t type;
    char file_path[512];
    uint32_t line_start;
    char original_code[2048];
    char suggested_code[2048];
    double confidence;
} fix_suggestion_t;
```

### Functions

**fix_suggest_for_file**
```c
crrss_status_t fix_suggest_for_file(
    fix_context_t* ctx,
    const char* file_path,
    fix_suggestion_t* suggestions,
    uint32_t max_suggestions,
    uint32_t* count
);
```

**fix_apply_suggestion**
```c
crrss_status_t fix_apply_suggestion(
    fix_context_t* ctx,
    const fix_suggestion_t* suggestion,
    bool create_backup
);
```

## Profile Rotation

### Types

```c
typedef enum {
    PROFILE_CONSERVATIVE,
    PROFILE_AGGRESSIVE,
    PROFILE_BALANCED,
    PROFILE_EXPERIMENTAL
} profile_type_t;

typedef enum {
    TASK_BUG_FIX,
    TASK_REFACTORING,
    TASK_OPTIMIZATION,
    TASK_NEW_FEATURE
} task_type_t;
```

### Functions

**profile_select_for_task**
```c
profile_type_t profile_select_for_task(
    profile_context_t* ctx,
    task_type_t task_type,
    uint32_t code_complexity,
    bool is_critical_module
);
```

## Return Codes

```c
typedef enum {
    CRRSS_STATUS_SUCCESS = 0,
    CRRSS_STATUS_INVALID_ARGUMENT = 1,
    CRRSS_STATUS_OUT_OF_MEMORY = 2,
    CRRSS_STATUS_FILE_ERROR = 3,
    CRRSS_STATUS_NOT_IMPLEMENTED = 4
} crrss_status_t;
```
