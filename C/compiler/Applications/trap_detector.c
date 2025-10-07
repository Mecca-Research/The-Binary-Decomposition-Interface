
#include "trap_detector.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static bool trap_detector_initialized = false;

bool trap_detector_init(void) {
    if (trap_detector_initialized) {
        return true;
    }
    trap_detector_initialized = true;
    return true;
}

void trap_detector_cleanup(void) {
    trap_detector_initialized = false;
}

TrapDetection* trap_detector_analyze(const char *code, size_t *detection_count) {
    if (!code || !detection_count) {
        return NULL;
    }

    TrapDetection *detections = calloc(20, sizeof(TrapDetection));
    if (!detections) {
        return NULL;
    }

    size_t det_idx = 0;

    // Detect buffer overflow
    if (strstr(code, "strcpy(") || strstr(code, "sprintf(") || strstr(code, "gets(")) {
        TrapDetection *det = &detections[det_idx++];
        det->type = TRAP_BUFFER_OVERFLOW;
        det->line = 0;
        strcpy(det->description, "Unsafe string operation - potential buffer overflow");
        det->confidence = 0.90;
        det->is_critical = true;
    }

    // Detect memory leak
    if (strstr(code, "malloc(") && !strstr(code, "free(")) {
        TrapDetection *det = &detections[det_idx++];
        det->type = TRAP_MEMORY_LEAK;
        det->line = 0;
        strcpy(det->description, "malloc() without corresponding free()");
        det->confidence = 0.75;
        det->is_critical = true;
    }

    // Detect off-by-one
    if (strstr(code, "for") && strstr(code, "<=")) {
        TrapDetection *det = &detections[det_idx++];
        det->type = TRAP_OFF_BY_ONE;
        det->line = 0;
        strcpy(det->description, "Potential off-by-one error in loop condition");
        det->confidence = 0.60;
        det->is_critical = false;
    }

    // Detect null dereference
    if (strstr(code, "->") && !strstr(code, "if") && !strstr(code, "NULL")) {
        TrapDetection *det = &detections[det_idx++];
        det->type = TRAP_NULL_DEREFERENCE;
        det->line = 0;
        strcpy(det->description, "Potential null pointer dereference");
        det->confidence = 0.65;
        det->is_critical = true;
    }

    *detection_count = det_idx;
    return detections;
}

void trap_detector_free_detections(TrapDetection *detections) {
    free(detections);
}

bool trap_detector_train(const char *buggy_code, TrapType trap_type) {
    if (!buggy_code) {
        return false;
    }
    // TODO: Implement ML-based training
    return true;
}
