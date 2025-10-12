
# RERS - Runtime Error Replay System

**Version:** 1.0.0  
**Part of:** CRRSS (Cognitive Runtime Reflection and Self-Supervision) Framework  
**Date:** October 12, 2025

## Overview

RERS (Runtime Error Replay System) is an advanced error handling and learning framework designed for the BDI (Binary Decomposition Interface) system. It provides comprehensive error replay, active learning from bugs, pattern matching, and intelligent coordination of personality profiles for enhanced debugging and system reliability.

## Features

### 1. **Error Replay Engine** 🔄
- Handles multiple error types:
  - Segmentation faults
  - Assertion failures
  - Memory leaks
  - Logic errors
  - Buffer overflows
  - NULL pointer dereferences
  - Use-after-free errors
  - Double-free errors
- Records error context (file, line, function, message)
- Supports error replay for debugging
- Configurable replay depth

### 2. **Active Learning System** 🧠
- **Hierarchical Learning:** 5-level hierarchy
  - Error Type Level
  - Component Level
  - Subsystem Level
  - System Level
  - Global Level
- **Priority-Based Bug Ranking:**
  - Critical (security, crashes)
  - High (data loss, corruption)
  - Medium (functional issues)
  - Low (minor issues)
- Automatic bug deduplication and occurrence tracking
- Learning threshold configuration

### 3. **Bug Pattern Database** 🗄️
- In-memory storage for fast access
- Pattern matching with confidence levels:
  - Exact match (≥95% similarity)
  - High confidence (≥75% similarity)
  - Medium confidence (≥50% similarity)
  - Low confidence (≥25% similarity)
- Fuzzy pattern matching support
- Fix suggestions storage
- Pattern statistics tracking

### 4. **Integration Layer** 🔗
- Coordinates four CRRSS personality profiles:
  - **MSM** (Multi-State Machine) - State tracking
  - **STP** (Self-Testing Protocol) - Testing and validation
  - **BPME** (Bug Pattern Matching Engine) - Pattern analysis
  - **TDT** (Test-Driven Thinking) - Test generation
- Task-based profile selection
- Multi-profile coordination with confidence aggregation
- Dynamic profile enabling/disabling

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    RERS Main System                      │
│  (Initialization, Configuration, Statistics)             │
└────────────────┬────────────────────────────────────────┘
                 │
    ┌────────────┼────────────┬─────────────┬──────────────┐
    │            │            │             │              │
    ▼            ▼            ▼             ▼              ▼
┌────────┐  ┌─────────┐  ┌─────────┐  ┌──────────┐  ┌─────────┐
│ Replay │  │Learning │  │ Pattern │  │Integration│  │  Stats  │
│ Engine │  │ System  │  │Database │  │  Layer    │  │ Tracking│
└────────┘  └─────────┘  └─────────┘  └──────────┘  └─────────┘
    │            │            │             │
    │            │            │             │
    └────────────┴────────────┴─────────────┘
                      │
                      ▼
          ┌──────────────────────┐
          │  Personality Profiles │
          │  MSM │ STP │ BPME │ TDT │
          └──────────────────────┘
```

## Building RERS

### Prerequisites
- GCC 9.0+ with C11 support
- GNU Make
- Standard C library

### Build Commands

```bash
# Build library and tests
make all

# Build library only
make librers.a

# Build tests only
make tests

# Run all tests
make test

# Clean build artifacts
make clean

# Show help
make help
```

### Build Output
- `librers.a` - Static library
- `tests/test_rers_*` - Test executables

## Usage Examples

### 1. Initialize RERS System

```c
#include "rers.h"

// Default configuration
rers_system_t *system = NULL;
rers_error_t err = rers_init(NULL, &system);
if (err != RERS_SUCCESS) {
    fprintf(stderr, "RERS init failed: %s\n", 
            rers_get_error_string(err));
    return -1;
}

// Custom configuration
rers_config_t config = {
    .enable_replay = true,
    .enable_learning = true,
    .enable_patterns = true,
    .enable_integration = true,
    .max_patterns = 2048,
    .max_replay_depth = 20,
    .learning_threshold = 10
};

err = rers_init(&config, &system);
```

### 2. Record and Replay Errors

```c
#include "rers_replay.h"

// Get replay engine from system
rers_replay_engine_t *engine = system->replay_engine;

// Record error
rers_error_context_t context = {
    .type = RERS_ERROR_TYPE_SEGFAULT,
    .file = __FILE__,
    .line = __LINE__,
    .function = __func__,
    .message = "NULL pointer dereference",
    .context_data = NULL,
    .context_size = 0
};

rers_replay_record(engine, &context);

// Replay error for debugging
rers_replay_execute(engine, 1); // Replay error ID 1
```

### 3. Active Learning from Bugs

```c
#include "rers_learning.h"

// Get learning system
rers_learning_system_t *learning = system->learning_system;

// Learn from critical bug
rers_bug_info_t bug = {
    .bug_id = 0,
    .error_type = RERS_ERROR_TYPE_MEMORY_LEAK,
    .priority = RERS_PRIORITY_CRITICAL,
    .level = RERS_HIERARCHY_COMPONENT,
    .component = "memory_manager",
    .description = "Memory leak in allocation path",
    .occurrence_count = 0
};

rers_learning_learn(learning, &bug);

// Get critical bugs for review
uint64_t critical_bugs[100];
size_t count;
rers_learning_get_by_priority(learning, RERS_PRIORITY_CRITICAL,
                              critical_bugs, 100, &count);

printf("Found %zu critical bugs\n", count);
```

### 4. Pattern Matching

```c
#include "rers_patterns.h"

// Get pattern database
rers_pattern_db_t *db = system->pattern_db;

// Add pattern
rers_pattern_t pattern = {
    .pattern_id = 0,
    .error_type = RERS_ERROR_TYPE_NULL_DEREF,
    .signature = "null_pointer_check",
    .description = "Missing NULL pointer check",
    .fix_suggestion = "Add NULL check before dereferencing",
    .match_count = 0
};

uint64_t pattern_id;
rers_pattern_add(db, &pattern, &pattern_id);

// Match error against patterns
rers_error_context_t error = {
    .type = RERS_ERROR_TYPE_NULL_DEREF,
    .message = "null_pointer_check failed"
};

rers_match_result_t match;
if (rers_pattern_match(db, &error, &match) == RERS_SUCCESS) {
    printf("Match found: %s (confidence: %s)\n",
           match.pattern_description,
           rers_pattern_get_confidence_name(match.confidence));
    printf("Suggestion: %s\n", match.fix_suggestion);
}
```

### 5. Profile Integration

```c
#include "rers_integration.h"

// Get integration layer
rers_integration_layer_t *integration = system->integration_layer;

// Submit output from BPME profile
rers_profile_output_t output = {
    .profile = RERS_PROFILE_BPME,
    .task = RERS_TASK_ERROR_ANALYSIS,
    .data = NULL,
    .data_size = 0,
    .confidence = 0.85f
};

rers_integration_submit_output(integration, &output);

// Coordinate profiles for error analysis
rers_coordination_result_t result;
rers_integration_coordinate(integration, 
                            RERS_TASK_ERROR_ANALYSIS,
                            &result);

printf("Task coordinated with %.1f%% confidence\n",
       result.overall_confidence * 100.0f);
printf("Recommendation: %s\n", result.recommendation);
```

## Integration with Personality Profiles

### Task-to-Profile Mapping

| Task | Primary Profile | Secondary Profiles |
|------|----------------|-------------------|
| Error Analysis | BPME | MSM, STP, TDT |
| Pattern Matching | BPME | TDT, STP, MSM |
| Test Generation | TDT | STP, BPME, MSM |
| State Tracking | MSM | STP, BPME, TDT |
| Bug Classification | BPME | MSM, TDT, STP |

### Profile Responsibilities

- **MSM (Multi-State Machine):** Tracks system state transitions and error states
- **STP (Self-Testing Protocol):** Validates fixes and runs regression tests
- **BPME (Bug Pattern Matching Engine):** Analyzes error patterns and similarities
- **TDT (Test-Driven Thinking):** Generates targeted tests for error reproduction

## API Reference

### Main System API

| Function | Description |
|----------|-------------|
| `rers_init()` | Initialize RERS system |
| `rers_shutdown()` | Shutdown RERS system |
| `rers_get_stats()` | Get system statistics |
| `rers_reset_stats()` | Reset statistics |
| `rers_get_version()` | Get version string |
| `rers_get_error_string()` | Get error description |

### Replay Engine API

| Function | Description |
|----------|-------------|
| `rers_replay_init()` | Initialize replay engine |
| `rers_replay_shutdown()` | Shutdown replay engine |
| `rers_replay_record()` | Record error for replay |
| `rers_replay_execute()` | Replay recorded error |
| `rers_replay_get_count()` | Get recorded error count |

### Learning System API

| Function | Description |
|----------|-------------|
| `rers_learning_init()` | Initialize learning system |
| `rers_learning_shutdown()` | Shutdown learning system |
| `rers_learning_learn()` | Learn from new bug |
| `rers_learning_get_by_priority()` | Get bugs by priority |
| `rers_learning_get_by_level()` | Get bugs by hierarchy level |
| `rers_learning_get_count()` | Get learned bug count |

### Pattern Database API

| Function | Description |
|----------|-------------|
| `rers_pattern_init()` | Initialize pattern database |
| `rers_pattern_shutdown()` | Shutdown pattern database |
| `rers_pattern_add()` | Add pattern to database |
| `rers_pattern_match()` | Match error against patterns |
| `rers_pattern_get()` | Get pattern by ID |
| `rers_pattern_get_count()` | Get pattern count |
| `rers_pattern_clear()` | Clear all patterns |

### Integration Layer API

| Function | Description |
|----------|-------------|
| `rers_integration_init()` | Initialize integration layer |
| `rers_integration_shutdown()` | Shutdown integration layer |
| `rers_integration_submit_output()` | Submit profile output |
| `rers_integration_coordinate()` | Coordinate profiles for task |
| `rers_integration_get_active_profiles()` | Get active profiles |
| `rers_integration_set_profile_enabled()` | Enable/disable profile |

## Testing

### Test Suite
- **test_rers_main** - Main system tests (7 tests)
- **test_rers_replay** - Replay engine tests (5 tests)
- **test_rers_learning** - Learning system tests (6 tests)
- **test_rers_patterns** - Pattern database tests (7 tests)
- **test_rers_integration** - Integration layer tests (8 tests)

### Running Tests

```bash
# Run all tests
make test

# Run specific test
./tests/test_rers_replay

# Run with script (colored output)
cd tests && ./run_all_tests.sh
```

### Test Coverage
- ✅ Component initialization/shutdown
- ✅ Error recording and replay
- ✅ Hierarchical learning
- ✅ Priority-based bug ranking
- ✅ Pattern matching and confidence levels
- ✅ Profile coordination
- ✅ Invalid parameter handling
- ✅ Statistics tracking

## Configuration

### Default Configuration

```c
{
    .enable_replay = true,
    .enable_learning = true,
    .enable_patterns = true,
    .enable_integration = true,
    .max_patterns = 1024,
    .max_replay_depth = 10,
    .learning_threshold = 5
}
```

### Tuning Parameters

- **max_patterns:** Increase for complex systems with many error types
- **max_replay_depth:** Increase for deep call stack analysis
- **learning_threshold:** Lower for aggressive learning, higher for stability

## Performance Characteristics

- **Memory:** O(n) where n = number of errors/patterns stored
- **Pattern Matching:** O(m) where m = number of patterns
- **Learning:** O(1) for insertion, O(n) for queries
- **Replay:** O(1) for execution

## Limitations

- Maximum 256 recorded errors per engine instance
- Maximum 512 learned bugs per learning system
- Maximum 1024 patterns in database (configurable)
- In-memory storage only (no persistence)

## Future Enhancements

- [ ] Persistent storage backend
- [ ] Advanced pattern matching algorithms
- [ ] Machine learning-based bug prediction
- [ ] Distributed error collection
- [ ] Real-time error analytics dashboard
- [ ] Automated fix generation

## Contributing

This module is part of the BDI project. For contributions:
1. Follow C11 coding standards
2. Add unit tests for new features
3. Update documentation
4. Ensure all tests pass

## License

See project LICENSE file.

## Support

For issues and questions:
- Create an issue in the BDI repository
- Tag with `component:rers` label

---

**RERS v1.0.0** - Built for reliability and intelligence in error handling.
