
#include "hot_path.h"
#include <stdlib.h>
#include <string.h>

HotPathDetector* hot_path_detector_create(void) {
    HotPathDetector* detector = (HotPathDetector*)calloc(1, sizeof(HotPathDetector));
    if (!detector) return NULL;
    
    detector->capacity = HOT_PATH_MAX_ENTRIES;
    detector->entries = (HotPathEntry*)calloc(detector->capacity, sizeof(HotPathEntry));
    if (!detector->entries) {
        free(detector);
        return NULL;
    }
    
    detector->baseline_threshold = HOT_PATH_THRESHOLD_BASELINE;
    detector->optimized_threshold = HOT_PATH_THRESHOLD_OPTIMIZED;
    
    return detector;
}

void hot_path_detector_destroy(HotPathDetector* detector) {
    if (!detector) return;
    
    free(detector->entries);
    free(detector);
}

static HotPathEntry* find_or_create_entry(
    HotPathDetector* detector,
    uint32_t function_id,
    uint32_t basic_block_id
) {
    // Find existing entry
    for (size_t i = 0; i < detector->entry_count; i++) {
        HotPathEntry* entry = &detector->entries[i];
        if (entry->function_id == function_id && entry->basic_block_id == basic_block_id) {
            return entry;
        }
    }
    
    // Create new entry if space available
    if (detector->entry_count < detector->capacity) {
        HotPathEntry* entry = &detector->entries[detector->entry_count++];
        entry->function_id = function_id;
        entry->basic_block_id = basic_block_id;
        entry->execution_count = 0;
        entry->total_time_ns = 0;
        entry->is_hot = false;
        entry->is_compiled = false;
        return entry;
    }
    
    return NULL;
}

void hot_path_detector_record_execution(
    HotPathDetector* detector,
    uint32_t function_id,
    uint32_t basic_block_id,
    uint64_t execution_time_ns
) {
    if (!detector) return;
    
    HotPathEntry* entry = find_or_create_entry(detector, function_id, basic_block_id);
    if (!entry) return;
    
    entry->execution_count++;
    entry->total_time_ns += execution_time_ns;
    
    detector->total_executions++;
    
    // Update hot path status
    if (entry->execution_count >= detector->baseline_threshold) {
        entry->is_hot = true;
        detector->hot_path_hits++;
    } else {
        detector->cold_path_hits++;
    }
}

bool hot_path_detector_is_hot(
    const HotPathDetector* detector,
    uint32_t function_id,
    uint32_t basic_block_id
) {
    if (!detector) return false;
    
    for (size_t i = 0; i < detector->entry_count; i++) {
        const HotPathEntry* entry = &detector->entries[i];
        if (entry->function_id == function_id && entry->basic_block_id == basic_block_id) {
            return entry->is_hot;
        }
    }
    
    return false;
}

bool hot_path_detector_should_optimize(
    const HotPathDetector* detector,
    uint32_t function_id,
    uint32_t basic_block_id
) {
    if (!detector) return false;
    
    for (size_t i = 0; i < detector->entry_count; i++) {
        const HotPathEntry* entry = &detector->entries[i];
        if (entry->function_id == function_id && entry->basic_block_id == basic_block_id) {
            return entry->execution_count >= detector->optimized_threshold && !entry->is_compiled;
        }
    }
    
    return false;
}

void hot_path_detector_mark_compiled(
    HotPathDetector* detector,
    uint32_t function_id,
    uint32_t basic_block_id
) {
    if (!detector) return;
    
    for (size_t i = 0; i < detector->entry_count; i++) {
        HotPathEntry* entry = &detector->entries[i];
        if (entry->function_id == function_id && entry->basic_block_id == basic_block_id) {
            entry->is_compiled = true;
            return;
        }
    }
}

void hot_path_detector_set_thresholds(
    HotPathDetector* detector,
    uint64_t baseline_threshold,
    uint64_t optimized_threshold
) {
    if (!detector) return;
    
    detector->baseline_threshold = baseline_threshold;
    detector->optimized_threshold = optimized_threshold;
}

void hot_path_detector_get_stats(
    const HotPathDetector* detector,
    uint64_t* total_executions,
    uint64_t* hot_path_hits,
    uint64_t* cold_path_hits
) {
    if (!detector) return;
    
    if (total_executions) *total_executions = detector->total_executions;
    if (hot_path_hits) *hot_path_hits = detector->hot_path_hits;
    if (cold_path_hits) *cold_path_hits = detector->cold_path_hits;
}

void hot_path_detector_reset_stats(HotPathDetector* detector) {
    if (!detector) return;
    
    detector->total_executions = 0;
    detector->hot_path_hits = 0;
    detector->cold_path_hits = 0;
    
    for (size_t i = 0; i < detector->entry_count; i++) {
        detector->entries[i].execution_count = 0;
        detector->entries[i].total_time_ns = 0;
        detector->entries[i].is_hot = false;
    }
}

static int compare_hotness(const void* a, const void* b) {
    const HotPathInfo* info_a = (const HotPathInfo*)a;
    const HotPathInfo* info_b = (const HotPathInfo*)b;
    
    if (info_a->hotness_score > info_b->hotness_score) return -1;
    if (info_a->hotness_score < info_b->hotness_score) return 1;
    return 0;
}

size_t hot_path_detector_get_hot_paths(
    const HotPathDetector* detector,
    HotPathInfo* paths,
    size_t max_paths
) {
    if (!detector || !paths || max_paths == 0) return 0;
    
    size_t count = 0;
    for (size_t i = 0; i < detector->entry_count && count < max_paths; i++) {
        const HotPathEntry* entry = &detector->entries[i];
        if (entry->is_hot) {
            paths[count].function_id = entry->function_id;
            paths[count].basic_block_id = entry->basic_block_id;
            paths[count].execution_count = entry->execution_count;
            paths[count].hotness_score = (double)entry->execution_count / 
                                        (entry->total_time_ns + 1);
            count++;
        }
    }
    
    // Sort by hotness score
    qsort(paths, count, sizeof(HotPathInfo), compare_hotness);
    
    return count;
}
