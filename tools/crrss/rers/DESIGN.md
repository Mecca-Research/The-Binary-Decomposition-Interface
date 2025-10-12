# RERS - Runtime Error Replay System
## Design Document

**Version:** 1.0.0  
**Part of:** CRRSS (Cognitive Runtime Reflection and Self-Supervision) Framework  
**Date:** October 12, 2025  
**Author:** BDI Development Team

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Architecture Overview](#architecture-overview)
3. [Core Components](#core-components)
4. [Design Decisions](#design-decisions)
5. [Data Structures](#data-structures)
6. [Algorithms](#algorithms)
7. [Integration Strategy](#integration-strategy)
8. [Performance Considerations](#performance-considerations)
9. [Security and Safety](#security-and-safety)
10. [Future Extensions](#future-extensions)

---

## Executive Summary

The Runtime Error Replay System (RERS) is a sophisticated error handling and learning framework designed to enhance the reliability and debuggability of the BDI (Binary Decomposition Interface) system. RERS provides four key capabilities:

1. **Error Replay Engine**: Captures and replays runtime errors for debugging
2. **Active Learning System**: Learns from bugs using hierarchical and priority-based classification
3. **Bug Pattern Database**: Stores and matches error patterns with confidence scoring
4. **Integration Layer**: Coordinates four CRRSS personality profiles for intelligent error handling

### Key Design Goals

- **Modularity**: Each component operates independently
- **Extensibility**: Easy to add new error types, patterns, and profiles
- **Performance**: Minimal runtime overhead in production
- **Reliability**: Robust error handling without crashing the system
- **Maintainability**: Clear interfaces and comprehensive documentation

---

## Architecture Overview

### System Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     RERS System Core                         │
│  • Configuration Management                                  │
│  • Statistics Tracking                                       │
│  • Component Lifecycle                                       │
│  • Error Code Translation                                    │
└──────────────────────┬──────────────────────────────────────┘
                       │
       ┌───────────────┼───────────────┬───────────────┐
       │               │               │               │
       ▼               ▼               ▼               ▼
┌──────────────┐ ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│   Replay     │ │   Learning   │ │   Pattern    │ │ Integration  │
│   Engine     │ │   System     │ │   Database   │ │    Layer     │
└──────────────┘ └──────────────┘ └──────────────┘ └──────────────┘
       │               │               │               │
       │               │               │               │
       └───────────────┴───────────────┴───────────────┘
                               │
                               ▼
                    ┌─────────────────────┐
                    │ CRRSS Profiles      │
                    │ ┌─────┬─────────┐   │
                    │ │ MSM │   STP   │   │
                    │ └─────┴─────────┘   │
                    │ ┌─────┬─────────┐   │
                    │ │BPME │   TDT   │   │
                    │ └─────┴─────────┘   │
                    └─────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Key Operations |
|-----------|---------------|----------------|
| **RERS Core** | System management | Init, shutdown, stats, version |
| **Replay Engine** | Error recording & replay | Record, execute, count |
| **Learning System** | Bug classification & learning | Learn, query by priority/level |
| **Pattern Database** | Pattern storage & matching | Add, match, get, clear |
| **Integration Layer** | Profile coordination | Submit, coordinate, manage profiles |

---

## Core Components

### 1. RERS Core System

#### Purpose
Central management hub for all RERS components. Handles initialization, configuration, statistics, and component lifecycle.

#### Key Data Structures

```c
struct rers_system {
    rers_config_t config;              // System configuration
    rers_stats_t stats;                // Runtime statistics
    rers_replay_engine_t *replay_engine;
    rers_learning_system_t *learning_system;
    rers_pattern_db_t *pattern_db;
    rers_integration_layer_t *integration_layer;
    bool initialized;
};
```

#### Design Decisions

1. **Opaque Handles**: All components use opaque handles to hide implementation details
   - Rationale: Enables API stability and implementation changes without breaking users
   
2. **Optional Components**: Each component can be enabled/disabled via configuration
   - Rationale: Allows users to enable only needed features, reducing overhead
   
3. **Default Configuration**: Sensible defaults for all parameters
   - Rationale: Easy to use out-of-the-box while allowing customization

### 2. Error Replay Engine

#### Purpose
Records runtime errors with full context and enables replay for debugging and analysis.

#### Supported Error Types (9 total)

1. **SEGFAULT**: Segmentation faults (NULL deref, access violations)
2. **ASSERTION**: Failed assertions and invariant violations
3. **MEMORY_LEAK**: Memory leaks detected by allocator
4. **LOGIC_ERROR**: Logic errors (incorrect state, invalid operations)
5. **BUFFER_OVERFLOW**: Buffer overflow/underflow
6. **NULL_DEREF**: Explicit NULL pointer dereference
7. **USE_AFTER_FREE**: Access to freed memory
8. **DOUBLE_FREE**: Double free operations
9. **CUSTOM**: User-defined error types

#### Error Context Structure

```c
typedef struct {
    rers_error_type_t type;           // Error type classification
    const char *file;                 // Source file (__FILE__)
    int line;                         // Line number (__LINE__)
    const char *function;             // Function name (__func__)
    const char *message;              // Human-readable message
    void *context_data;               // Additional context (stack, registers)
    size_t context_size;              // Size of context data
    uint64_t timestamp;               // Error occurrence time
} rers_error_context_t;
```

#### Replay Mechanism

1. **Recording Phase**
   - Capture error context (file, line, function)
   - Store additional context (stack trace, memory state)
   - Assign unique error ID
   - Update statistics

2. **Replay Phase**
   - Retrieve error by ID
   - Recreate error context
   - Execute replay handler
   - Log replay results

#### Design Decisions

1. **In-Memory Storage**: Errors stored in fixed-size array
   - Rationale: Fast access, simple implementation
   - Trade-off: Limited capacity (256 errors), no persistence

2. **Context Data**: Optional additional context via void pointer
   - Rationale: Flexible storage for stack traces, register dumps
   - Trade-off: User must manage memory

3. **Timestamp**: Uses monotonic clock for ordering
   - Rationale: Accurate ordering even with system clock changes

### 3. Active Learning System

#### Purpose
Learns from bugs using hierarchical classification and priority-based ranking to guide debugging efforts.

#### Hierarchical Learning (5 Levels)

```
Level 0: ERROR_TYPE     - Error type classification (segfault, leak, etc.)
Level 1: COMPONENT      - Component where error occurred (MM, scheduler, etc.)
Level 2: SUBSYSTEM      - Subsystem classification (memory, I/O, network)
Level 3: SYSTEM         - System-level categorization
Level 4: GLOBAL         - Cross-system patterns
```

#### Priority-Based Ranking (4 Tiers)

```
Critical (P0): Security vulnerabilities, crashes, data corruption
High (P1):     Data loss, severe functional issues
Medium (P2):   Moderate functional issues, performance degradation
Low (P3):      Minor issues, cosmetic problems
```

#### Bug Information Structure

```c
typedef struct {
    uint64_t bug_id;                  // Unique identifier
    rers_error_type_t error_type;     // Error type
    rers_priority_t priority;         // Priority level
    rers_hierarchy_level_t level;     // Hierarchy level
    const char *component;            // Component name
    const char *description;          // Bug description
    uint32_t occurrence_count;        // Occurrence count
    uint64_t first_seen;              // First occurrence
    uint64_t last_seen;               // Last occurrence
} rers_bug_info_t;
```

#### Learning Algorithm

1. **Bug Submission**
   - Check for duplicate (same error type + component)
   - If duplicate: increment occurrence count
   - If new: assign bug ID, store information

2. **Priority Assignment**
   - Based on error type and impact
   - Critical: crashes, security issues
   - High: data corruption, functional failures
   - Medium: degraded functionality
   - Low: minor issues

3. **Hierarchical Classification**
   - Level determined by scope
   - Component → Subsystem → System → Global
   - Enables queries at different granularities

#### Design Decisions

1. **Deduplication**: Bugs tracked by (error_type, component) tuple
   - Rationale: Prevents duplicate entries for same bug
   - Trade-off: May miss subtle variations

2. **Occurrence Tracking**: Count incremented on each occurrence
   - Rationale: Helps identify frequent issues
   - Use case: Trigger learning threshold

3. **Learning Threshold**: Configurable minimum occurrences
   - Rationale: Filters noise, focuses on recurring issues
   - Default: 5 occurrences

### 4. Bug Pattern Database

#### Purpose
Stores known bug patterns and matches new errors against them using fuzzy matching and confidence scoring.

#### Pattern Structure

```c
typedef struct {
    uint64_t pattern_id;              // Unique pattern ID
    rers_error_type_t error_type;     // Error type
    const char *signature;            // Pattern signature (regex, substring)
    const char *description;          // Pattern description
    const char *fix_suggestion;       // Recommended fix
    uint32_t match_count;             // Number of matches
    uint64_t created_at;              // Creation timestamp
} rers_pattern_t;
```

#### Confidence Levels

```
EXACT   (95-100%): Exact match (signature equals error message)
HIGH    (75-94%):  High confidence (substring match)
MEDIUM  (50-74%):  Medium confidence (partial match)
LOW     (25-49%):  Low confidence (weak match)
NONE    (0-24%):   No match
```

#### Matching Algorithm

1. **Error Type Filter**
   - Only match patterns with same error type
   - Rationale: Reduces false positives

2. **Signature Matching**
   - Exact: `strcmp(signature, error_message) == 0`
   - High: `strstr(error_message, signature) != NULL`
   - Medium: Fuzzy substring match
   - Low: Weak similarity

3. **Confidence Scoring**
   - Calculate similarity score (0.0 - 1.0)
   - Map to confidence level
   - Return best match with highest confidence

#### Design Decisions

1. **In-Memory Storage**: Fixed-size array of patterns
   - Rationale: Fast lookup, simple implementation
   - Trade-off: Limited capacity (1024 patterns default)

2. **Simple Matching**: String-based matching with substring search
   - Rationale: Fast and reliable for common cases
   - Future: Add regex, ML-based matching

3. **Confidence Thresholds**: Fixed thresholds for levels
   - Rationale: Predictable behavior
   - Trade-off: May need tuning for specific domains

### 5. Integration Layer

#### Purpose
Coordinates four CRRSS personality profiles to provide intelligent, multi-perspective error analysis.

#### CRRSS Personality Profiles

| Profile | Role | Expertise |
|---------|------|-----------|
| **MSM** | Multi-State Machine | State tracking, transitions, invariants |
| **STP** | Self-Testing Protocol | Test generation, validation, regression |
| **BPME** | Bug Pattern Matching Engine | Pattern analysis, similarity detection |
| **TDT** | Test-Driven Thinking | Test-first design, coverage analysis |

#### Task-to-Profile Mapping

```
ERROR_ANALYSIS:      Primary: BPME  | Secondary: MSM, STP, TDT
PATTERN_MATCHING:    Primary: BPME  | Secondary: TDT, STP, MSM
TEST_GENERATION:     Primary: TDT   | Secondary: STP, BPME, MSM
STATE_TRACKING:      Primary: MSM   | Secondary: STP, BPME, TDT
BUG_CLASSIFICATION:  Primary: BPME  | Secondary: MSM, TDT, STP
```

#### Coordination Algorithm

1. **Task Submission**
   - Identify task type
   - Select primary profile
   - Determine secondary profiles

2. **Profile Execution**
   - Primary profile executes first
   - Secondary profiles execute in parallel
   - Each produces output with confidence

3. **Result Aggregation**
   - Weighted average of confidences
   - Primary profile: 50% weight
   - Secondary profiles: 50% weight (distributed)
   - Generate recommendation

4. **Result Delivery**
   - Return coordination result
   - Include best recommendation
   - Provide confidence score

#### Design Decisions

1. **Profile Independence**: Profiles don't directly communicate
   - Rationale: Maintains modularity, easier to test
   - Integration layer handles coordination

2. **Confidence-Based Weighting**: Results weighted by confidence
   - Rationale: More reliable profiles have more influence
   - Prevents low-confidence results from skewing outcome

3. **Dynamic Profile Management**: Profiles can be enabled/disabled
   - Rationale: Allows customization for specific use cases
   - Example: Disable expensive profiles in production

---

## Design Decisions

### Memory Management

**Decision**: Use malloc/free for dynamic allocation

**Rationale**:
- Standard C library, portable
- Predictable behavior
- Easy to debug

**Trade-offs**:
- No automatic garbage collection
- User must call shutdown functions

**Mitigation**:
- Clear ownership semantics
- Comprehensive documentation
- Shutdown functions free all resources

### Error Handling

**Decision**: Return error codes, no exceptions

**Rationale**:
- C11 standard doesn't support exceptions
- Explicit error handling is more predictable
- Compatible with kernel and embedded systems

**Implementation**:
```c
typedef enum {
    RERS_SUCCESS = 0,
    RERS_ERROR_INVALID_PARAM,
    RERS_ERROR_NO_MEMORY,
    // ...
} rers_error_t;
```

### Thread Safety

**Decision**: RERS is NOT thread-safe by default

**Rationale**:
- Locking adds overhead
- Many use cases are single-threaded
- Users can add external locks if needed

**Documentation**:
- Clearly document non-thread-safe behavior
- Provide guidance on external synchronization
- Future: Add optional thread-safe mode

### Configuration

**Decision**: Struct-based configuration with defaults

**Rationale**:
- Explicit, type-safe
- Easy to extend with new fields
- NULL config uses defaults

**Example**:
```c
rers_config_t config = {
    .enable_replay = true,
    .enable_learning = true,
    .max_patterns = 2048,  // Custom value
    // Other fields use defaults
};
```

### API Design

**Decision**: Opaque handles with init/shutdown pattern

**Rationale**:
- Hides implementation details
- Enables ABI stability
- Standard C pattern

**Example**:
```c
rers_system_t *system = NULL;
rers_init(&config, &system);
// Use system...
rers_shutdown(system);
```

---

## Data Structures

### Storage Strategies

#### Fixed-Size Arrays

Used for:
- Error replay buffer (256 errors)
- Bug learning system (512 bugs)
- Pattern database (1024 patterns)

**Advantages**:
- O(1) access time
- Predictable memory usage
- Simple implementation

**Disadvantages**:
- Fixed capacity
- Memory waste if not fully utilized
- No automatic growth

#### Linear Search

Used for:
- Pattern matching
- Bug queries
- Error lookup

**Rationale**:
- Simple to implement
- Fast for small datasets (<1000 items)
- No external dependencies

**Future Optimization**:
- Hash tables for O(1) lookup
- B-trees for range queries
- Bloom filters for existence checks

### Memory Layout

```
rers_system_t (56 bytes)
├── config (28 bytes)
├── stats (40 bytes)
├── replay_engine* (8 bytes) → rers_replay_engine (4KB)
├── learning_system* (8 bytes) → rers_learning_system (16KB)
├── pattern_db* (8 bytes) → rers_pattern_db (32KB)
└── integration_layer* (8 bytes) → rers_integration_layer (8KB)

Total: ~60KB per RERS instance
```

---

## Algorithms

### Pattern Matching Algorithm

```
Input: error_context, pattern_db
Output: match_result

1. best_match = NULL
2. best_score = 0.0

3. FOR each pattern in pattern_db:
   a. IF pattern.error_type != error_context.type:
      CONTINUE
   
   b. score = calculate_similarity(pattern.signature, error_context.message)
   
   c. IF score > best_score:
      best_match = pattern
      best_score = score

4. IF best_match == NULL:
   RETURN RERS_ERROR_PATTERN_NOT_FOUND

5. confidence = map_score_to_confidence(best_score)

6. RETURN match_result {
      pattern_id: best_match.id,
      confidence: confidence,
      similarity_score: best_score,
      ...
   }
```

**Complexity**: O(n) where n = number of patterns  
**Optimization**: Can be reduced to O(log n) with indexing

### Learning Deduplication Algorithm

```
Input: new_bug_info
Output: bug_id (new or existing)

1. key = (bug_info.error_type, bug_info.component)

2. FOR each existing_bug in learning_system:
   a. existing_key = (existing_bug.error_type, existing_bug.component)
   
   b. IF key == existing_key:
      existing_bug.occurrence_count++
      existing_bug.last_seen = current_timestamp()
      RETURN existing_bug.bug_id

3. IF bug_count >= max_bugs:
   RETURN RERS_ERROR_NO_MEMORY

4. new_bug_id = next_bug_id++
5. new_bug.occurrence_count = 1
6. new_bug.first_seen = current_timestamp()
7. new_bug.last_seen = current_timestamp()
8. ADD new_bug to learning_system

9. RETURN new_bug_id
```

**Complexity**: O(n) where n = number of bugs  
**Optimization**: Use hash table for O(1) lookup

### Profile Coordination Algorithm

```
Input: task_type, integration_layer
Output: coordination_result

1. primary_profile = get_primary_profile_for_task(task_type)
2. secondary_profiles = get_secondary_profiles_for_task(task_type)

3. IF primary_profile is disabled:
   RETURN RERS_ERROR_COMPONENT_FAILED

4. primary_output = execute_profile(primary_profile, task_type)
5. primary_weight = 0.5

6. secondary_outputs = []
7. FOR each secondary_profile in secondary_profiles:
   a. IF secondary_profile is enabled:
      output = execute_profile(secondary_profile, task_type)
      ADD output to secondary_outputs

8. secondary_weight = 0.5 / count(secondary_outputs)

9. overall_confidence = 
   primary_output.confidence * primary_weight +
   SUM(output.confidence * secondary_weight for output in secondary_outputs)

10. recommendation = generate_recommendation(
       primary_output, secondary_outputs, overall_confidence
    )

11. RETURN coordination_result {
       task: task_type,
       primary_profile: primary_profile,
       profiles_used: bitmask of enabled profiles,
       overall_confidence: overall_confidence,
       recommendation: recommendation
    }
```

**Complexity**: O(p) where p = number of profiles (max 4)  
**Parallelization**: Secondary profiles can execute in parallel

---

## Integration Strategy

### Integration with BDI Subsystems

#### Memory Manager Integration

```c
// In memory allocator
void* bdi_alloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        // Record memory allocation failure
        rers_error_context_t ctx = {
            .type = RERS_ERROR_TYPE_MEMORY_LEAK,
            .file = __FILE__,
            .line = __LINE__,
            .function = __func__,
            .message = "Memory allocation failed"
        };
        rers_replay_record(rers_system->replay_engine, &ctx);
    }
    return ptr;
}
```

#### Scheduler Integration

```c
// In task scheduler
void schedule_task(task_t *task) {
    if (!task || !task->valid) {
        rers_error_context_t ctx = {
            .type = RERS_ERROR_TYPE_ASSERTION,
            .file = __FILE__,
            .line = __LINE__,
            .function = __func__,
            .message = "Invalid task in scheduler"
        };
        rers_replay_record(rers_system->replay_engine, &ctx);
        return;
    }
    // Schedule task...
}
```

### Integration with Master Memory Manager

The Master Memory Manager (Phase 2, PR#10) can use RERS for:

1. **Memory Leak Detection**
   - Record allocation without corresponding free
   - Learn patterns of leak-prone code paths

2. **Double-Free Detection**
   - Record double-free attempts
   - Match against known patterns

3. **Profile Integration**
   - Use BPME for pattern matching
   - Use MSM for state tracking
   - Use TDT for test generation

---

## Performance Considerations

### Time Complexity

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Error recording | O(1) | Array append |
| Error replay | O(1) | Array index lookup |
| Pattern matching | O(n) | Linear search through patterns |
| Bug learning | O(n) | Linear search for deduplication |
| Profile coordination | O(p) | p = number of profiles (max 4) |

### Space Complexity

| Component | Memory Usage | Configurable |
|-----------|--------------|--------------|
| Replay Engine | ~4KB (256 errors × 16 bytes) | No |
| Learning System | ~16KB (512 bugs × 32 bytes) | No |
| Pattern Database | ~32KB (1024 patterns × 32 bytes) | Yes (max_patterns) |
| Integration Layer | ~8KB | No |
| **Total per instance** | **~60KB** | Partially |

### Performance Optimization Strategies

1. **Lazy Initialization**
   - Components initialized only when needed
   - Disabled components consume no memory

2. **Cache-Friendly Layout**
   - Structures aligned to cache line boundaries
   - Hot data grouped together

3. **Minimize Allocations**
   - Fixed-size arrays reduce malloc/free overhead
   - Pre-allocated buffers for common operations

4. **Fast Path Optimization**
   - Error recording optimized for speed
   - Pattern matching can be skipped if disabled

---

## Security and Safety

### Memory Safety

1. **Bounds Checking**: All array accesses bounds-checked
2. **NULL Checks**: All pointers validated before dereference
3. **Buffer Overflows**: Fixed-size buffers with size tracking
4. **Use-After-Free**: Clear pointers after free

### Input Validation

1. **Parameter Validation**: All public APIs validate parameters
2. **Error Handling**: Invalid inputs return error codes
3. **Defensive Programming**: Assert preconditions in debug builds

### Fault Isolation

1. **Component Independence**: Failure in one component doesn't crash others
2. **Graceful Degradation**: System continues with reduced functionality
3. **Error Recovery**: Components can be reinitialized after failure

### Security Considerations

1. **No Persistence**: In-memory storage prevents information leakage
2. **No Network**: No network communication reduces attack surface
3. **Minimal Dependencies**: Only standard C library

---

## Future Extensions

### Phase 1: Enhanced Storage

- Persistent storage backend (SQLite, files)
- Configurable storage strategies
- LRU eviction for limited memory

### Phase 2: Advanced Matching

- Regular expression patterns
- Machine learning-based similarity
- Fuzzy string matching algorithms

### Phase 3: Distributed RERS

- Multi-node error collection
- Centralized pattern database
- Distributed learning

### Phase 4: Real-Time Analytics

- Web dashboard for error visualization
- Real-time error streams
- Statistical analysis and trending

### Phase 5: Automated Fix Generation

- AI-powered fix suggestions
- Patch generation from patterns
- Automated testing of fixes

### Phase 6: Integration Enhancements

- More CRRSS profiles
- Custom profile development
- Profile marketplace

---

## Conclusion

RERS provides a comprehensive, modular, and extensible framework for runtime error handling, learning, and debugging. The design emphasizes:

- **Simplicity**: Easy to understand and use
- **Modularity**: Components are independent and reusable
- **Performance**: Minimal overhead in production
- **Extensibility**: Easy to add new features
- **Reliability**: Robust error handling

The system is production-ready for PR#179 and provides a solid foundation for future enhancements in the BDI project.

---

**Document Version**: 1.0.0  
**Last Updated**: October 12, 2025  
**Status**: Complete
