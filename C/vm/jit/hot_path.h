
#ifndef HOT_PATH_H
#define HOT_PATH_H

#include <stdint.h>
#include <stdbool.h>

// Hot path detection thresholds
#define HOT_PATH_THRESHOLD_BASELINE 100
#define HOT_PATH_THRESHOLD_OPTIMIZED 1000
#define HOT_PATH_MAX_ENTRIES 1024

// Hot path entry
typedef struct {
    uint32_t function_id;
    uint32_t basic_block_id;
    uint64_t execution_count;
    uint64_t total_time_ns;
    bool is_hot;
    bool is_compiled;
} HotPathEntry;

// Hot path detector
typedef struct {
    HotPathEntry* entries;
    size_t entry_count;
    size_t capacity;
    
    uint64_t baseline_threshold;
    uint64_t optimized_threshold;
    
    // Statistics
    uint64_t total_executions;
    uint64_t hot_path_hits;
    uint64_t cold_path_hits;
} HotPathDetector;

// Hot path detector API
HotPathDetector* hot_path_detector_create(void);
void hot_path_detector_destroy(HotPathDetector* detector);

void hot_path_detector_record_execution(
    HotPathDetector* detector,
    uint32_t function_id,
    uint32_t basic_block_id,
    uint64_t execution_time_ns
);

bool hot_path_detector_is_hot(
    const HotPathDetector* detector,
    uint32_t function_id,
    uint32_t basic_block_id
);

bool hot_path_detector_should_optimize(
    const HotPathDetector* detector,
    uint32_t function_id,
    uint32_t basic_block_id
);

void hot_path_detector_mark_compiled(
    HotPathDetector* detector,
    uint32_t function_id,
    uint32_t basic_block_id
);

// Configuration
void hot_path_detector_set_thresholds(
    HotPathDetector* detector,
    uint64_t baseline_threshold,
    uint64_t optimized_threshold
);

// Statistics
void hot_path_detector_get_stats(
    const HotPathDetector* detector,
    uint64_t* total_executions,
    uint64_t* hot_path_hits,
    uint64_t* cold_path_hits
);

void hot_path_detector_reset_stats(HotPathDetector* detector);

// Hot path analysis
typedef struct {
    uint32_t function_id;
    uint32_t basic_block_id;
    uint64_t execution_count;
    double hotness_score;
} HotPathInfo;

size_t hot_path_detector_get_hot_paths(
    const HotPathDetector* detector,
    HotPathInfo* paths,
    size_t max_paths
);

#endif // HOT_PATH_H
