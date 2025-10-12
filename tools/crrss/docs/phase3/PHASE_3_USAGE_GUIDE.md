# CRRSS Phase 3 Usage Guide

## Quick Start

### Installation

```bash
cd tools/crrss
mkdir build && cd build
cmake -DCRRSS_ENABLE_PHASE3=ON ..
make
sudo make install
```

### Basic Usage

```bash
# Analyze with heatmap
crrss analyze --heatmap -d moduler_kernel/

# Predict bugs
crrss predict -f kernel/memory.c

# Analyze dependencies
crrss deps -d moduler_kernel/ --circular

# Generate fix suggestions
crrss fix -f kernel/memory.c --apply
```

## Error Heatmap Visualization

### Generate Heatmap

```bash
# ASCII format
crrss heatmap -d moduler_kernel/ --format ascii

# HTML format
crrss heatmap -d moduler_kernel/ --format html -o heatmap.html

# JSON export
crrss heatmap -d moduler_kernel/ --format json -o heatmap.json
```

### Programmatic API

```c
#include "ehv/ehv.h"

// Initialize
ehv_config_t config = {
    .enable_clustering = true,
    .enable_temporal_analysis = true,
    .max_hotspots = 100,
    .heat_threshold = 0.3
};
ehv_context_t* ctx = ehv_initialize(&config);

// Record errors
ehv_record_error(ctx, "memory.c", "allocate", 45, 
                 BUG_CATEGORY_MEMORY, BUG_PRIORITY_P0);

// Generate visualization
ehv_export_visualization(ctx, EHV_FORMAT_HTML, "heatmap.html");

// Cleanup
ehv_shutdown(ctx);
```

## Predictive Bug Modeling

### Predict Bugs

```bash
# Predict for single file
crrss predict -f kernel/memory.c --confidence 0.8

# Predict for directory
crrss predict -d moduler_kernel/ --model latest

# Train new model
crrss train --data training_data.json --epochs 100
```

### Programmatic API

```c
#include "pbm/pbm.h"

// Initialize
pbm_config_t config = {
    .model_type = PBM_MODEL_RANDOM_FOREST,
    .enable_online_learning = true,
    .confidence_threshold = 0.7
};
pbm_context_t* ctx = pbm_initialize(&config);

// Extract features
pbm_feature_vector_t features;
pbm_extract_features(ctx, "memory.c", &features);

// Predict
pbm_prediction_t predictions[10];
uint32_t count;
pbm_predict_file(ctx, "memory.c", predictions, 10, &count);

printf("Risk Score: %.2f\n", predictions[0].risk_score);

pbm_shutdown(ctx);
```

## Dependency Analysis

### Analyze Dependencies

```bash
# Basic analysis
crrss deps -d moduler_kernel/

# Detect circular dependencies
crrss deps -d moduler_kernel/ --circular

# Export graph
crrss deps -d moduler_kernel/ --export deps.dot

# Find coupling points
crrss deps -d moduler_kernel/ --coupling
```

### Programmatic API

```c
#include "deps/deps.h"

deps_config_t config = {
    .analyze_includes = true,
    .detect_circular_deps = true,
    .coupling_threshold = 0.7
};
deps_context_t* ctx = deps_initialize(&config);

deps_analyze_directory(ctx, "moduler_kernel/");

// Detect circular dependencies
deps_circular_dependency_t circulars[10];
uint32_t count;
deps_detect_circular(ctx, circulars, 10, &count);

// Export visualization
deps_export_visualization(ctx, DEPS_FORMAT_DOT, "deps.dot");

deps_shutdown(ctx);
```

## Automated Fix Suggestions

### Generate Fixes

```bash
# Suggest fixes
crrss fix -f kernel/memory.c

# Apply fix with backup
crrss fix -f kernel/memory.c --apply --backup

# Generate diff
crrss fix -f kernel/memory.c --diff
```

### Programmatic API

```c
#include "fix_suggestions/fix_suggestions.h"

fix_config_t config = {
    .enable_buffer_overflow_fixes = true,
    .min_confidence = 0.8
};
fix_context_t* ctx = fix_initialize(&config);

fix_suggestion_t suggestions[100];
uint32_t count;
fix_suggest_for_file(ctx, "memory.c", suggestions, 100, &count);

// Apply first suggestion
fix_apply_suggestion(ctx, &suggestions[0], true);

fix_shutdown(ctx);
```

## Profile Rotation

### Select Profile

```bash
# Use conservative profile
crrss analyze -d moduler_kernel/ --profile conservative

# Use aggressive profile
crrss analyze -d moduler_kernel/ --profile aggressive

# Auto-select profile
crrss analyze -d moduler_kernel/ --profile auto
```

### Programmatic API

```c
#include "profiles/profile_rotation.h"

profile_context_t* ctx = profile_initialize();

// Select profile for task
profile_type_t profile = profile_select_for_task(
    ctx, TASK_BUG_FIX, 25, true
);

// Get profile configuration
profile_config_t config;
profile_get_config(ctx, profile, &config);

profile_shutdown(ctx);
```

## Configuration

### Configuration File (.crrssrc)

```json
{
  "phase3": {
    "ehv": {
      "enable_clustering": true,
      "max_hotspots": 100
    },
    "pbm": {
      "model_path": "models/pbm_latest.pkl",
      "confidence_threshold": 0.8
    },
    "deps": {
      "detect_circular": true,
      "coupling_threshold": 0.7
    },
    "fix": {
      "auto_apply": false,
      "create_backup": true
    },
    "profile": {
      "default": "balanced",
      "auto_select": true
    }
  }
}
```

## Best Practices

1. **Start with Heatmap:** Visualize error patterns first
2. **Use Predictions Wisely:** Focus on high-confidence predictions
3. **Review Dependencies Regularly:** Catch circular dependencies early
4. **Test Fixes:** Always review and test automated fixes
5. **Choose Right Profile:** Match profile to task type
6. **Monitor Performance:** Track analysis time and accuracy

## Troubleshooting

### Common Issues

**Issue:** Model not found
```bash
# Download latest model
crrss model download latest
```

**Issue:** Analysis too slow
```bash
# Use faster profile
crrss analyze --profile aggressive --threads 8
```

**Issue:** Too many false positives
```bash
# Increase confidence threshold
crrss predict --confidence 0.9
```
