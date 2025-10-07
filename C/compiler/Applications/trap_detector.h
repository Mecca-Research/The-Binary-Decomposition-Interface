
#ifndef BDI_TRAP_DETECTOR_H
#define BDI_TRAP_DETECTOR_H

#include <stdbool.h>
#include <stddef.h>

// Trap types
typedef enum {
    TRAP_BUFFER_OVERFLOW,
    TRAP_OFF_BY_ONE,
    TRAP_MEMORY_LEAK,
    TRAP_USE_AFTER_FREE,
    TRAP_NULL_DEREFERENCE,
    TRAP_DOUBLE_FREE
} TrapType;

// Trap detection result
typedef struct {
    TrapType type;
    size_t line;
    size_t column;
    char description[256];
    char code_snippet[512];
    double confidence;
    bool is_critical;
} TrapDetection;

// Initialize trap detector
bool trap_detector_init(void);

// Cleanup trap detector
void trap_detector_cleanup(void);

// Detect traps in code
TrapDetection* trap_detector_analyze(const char *code, size_t *detection_count);

// Free trap detections
void trap_detector_free_detections(TrapDetection *detections);

// Train detector with known bugs
bool trap_detector_train(const char *buggy_code, TrapType trap_type);

#endif // BDI_TRAP_DETECTOR_H
