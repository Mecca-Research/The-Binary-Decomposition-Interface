
# Garbage Collection

This directory contains the garbage collection implementations for the BDI VM.

## Components

### mark_sweep.h/c
Classic mark-and-sweep garbage collector with compaction support.

**Features:**
- Mark-and-sweep algorithm
- Free list management
- Object pinning
- Optional heap compaction
- Configurable GC thresholds

**Key Functions:**
- `mark_sweep_create()` - Create GC instance
- `mark_sweep_allocate()` - Allocate object
- `mark_sweep_collect()` - Run garbage collection
- `mark_sweep_compact()` - Compact heap

### generational_gc.h/c
Generational garbage collector with young and old generations.

**Features:**
- Three generations (young, old, permanent)
- Minor and major collections
- Object promotion
- Remembered set for cross-generation references
- Write barrier
- Parallel and concurrent GC support

**Key Functions:**
- `generational_gc_create()` - Create generational GC
- `generational_gc_allocate()` - Allocate in specific generation
- `generational_gc_minor_collect()` - Collect young generation
- `generational_gc_major_collect()` - Collect old generation
- `generational_gc_promote()` - Promote object to older generation

## Architecture

### Mark-Sweep GC

```
┌─────────────────────────────────────────────────────────────┐
│                         Heap                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ Object 1 │  │ Object 2 │  │   Free   │  │ Object 3 │   │
│  │ (marked) │  │ (marked) │  │  Space   │  │(unmarked)│   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└─────────────────────────────────────────────────────────────┘
         ↓              ↓                            ↓
    ┌────────┐    ┌────────┐                   ┌────────┐
    │  Root  │    │  Root  │                   │Garbage │
    └────────┘    └────────┘                   └────────┘

Mark Phase: Traverse from roots, mark reachable objects
Sweep Phase: Free unmarked objects, add to free list
Compact Phase (optional): Move objects together, update references
```

### Generational GC

```
┌─────────────────────────────────────────────────────────────┐
│                   Young Generation                           │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                  │
│  │  New     │  │  New     │  │  New     │                  │
│  │ Object 1 │  │ Object 2 │  │ Object 3 │                  │
│  └──────────┘  └──────────┘  └──────────┘                  │
│       ↓ (age >= threshold)                                   │
└─────────────────────────────────────────────────────────────┘
                      ↓ Promotion
┌─────────────────────────────────────────────────────────────┐
│                    Old Generation                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                  │
│  │  Old     │  │  Old     │  │Promoted  │                  │
│  │ Object 1 │  │ Object 2 │  │ Object   │                  │
│  └──────────┘  └──────────┘  └──────────┘                  │
└─────────────────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────────────────┐
│                 Permanent Generation                         │
│  ┌──────────┐  ┌──────────┐                                 │
│  │  Class   │  │  Meta    │                                 │
│  │  Data    │  │  Data    │                                 │
│  └──────────┘  └──────────┘                                 │
└─────────────────────────────────────────────────────────────┘
```

## Collection Strategies

### Minor Collection (Young Generation)
- Fast, frequent collections
- Most objects die young (generational hypothesis)
- Evacuate survivors to old generation
- Typical pause time: 1-10ms

### Major Collection (Old Generation)
- Slower, less frequent collections
- Collect long-lived objects
- May use mark-sweep or mark-compact
- Typical pause time: 10-100ms

### Full Collection
- Collect all generations
- Most thorough but slowest
- Used when memory pressure is high
- Typical pause time: 100-1000ms

## Memory Layout

### Object Header
```c
struct GCObjectHeader {
    uint32_t size;        // Object size
    uint32_t type_id;     // Type identifier
    bool marked;          // GC mark bit
    bool pinned;          // Pinned flag
    struct GCObject* next; // Free list pointer
};
```

### Generational Object Header
```c
struct GenObjectHeader {
    GCObjectHeader base;
    Generation generation; // Current generation
    uint32_t age;         // Survival count
    bool remembered;      // In remembered set
};
```

## Write Barrier

The write barrier tracks cross-generation references:

```c
void write_barrier(GenObject* parent, GenObject* child) {
    if (parent->generation > child->generation) {
        add_to_remembered_set(child);
    }
}
```

This ensures that young objects referenced by old objects are not collected prematurely.

## Configuration

### Mark-Sweep GC
```c
// Set GC threshold
mark_sweep_set_threshold(gc, heap_size / 2);

// Set growth factor
mark_sweep_set_growth_factor(gc, 1.5);

// Enable compaction
mark_sweep_enable_compaction(gc, true);
```

### Generational GC
```c
// Set promotion thresholds
generational_gc_set_age_threshold(gc, 3);
generational_gc_set_size_threshold(gc, 1024);

// Enable parallel GC
generational_gc_enable_parallel(gc, true, 4);

// Enable concurrent GC
generational_gc_enable_concurrent(gc, true);
```

## Performance Characteristics

### Mark-Sweep GC
- **Allocation**: O(n) worst case (free list search)
- **Collection**: O(heap_size)
- **Pause time**: Proportional to heap size
- **Throughput**: Good for small heaps

### Generational GC
- **Allocation**: O(1) (bump pointer)
- **Minor collection**: O(young_generation_size)
- **Major collection**: O(old_generation_size)
- **Pause time**: Shorter for minor collections
- **Throughput**: Excellent for typical workloads

## Integration

The GC integrates with:
- **VM**: Manages object lifetime
- **JIT Compiler**: Handles compiled code memory
- **Type System**: Provides object layout information

## Future Enhancements

1. **Concurrent Collection**
   - Reduce pause times
   - Background GC threads
   - Incremental marking

2. **Parallel Collection**
   - Multi-threaded GC
   - Work stealing
   - Parallel marking and sweeping

3. **Advanced Algorithms**
   - G1 GC (region-based)
   - ZGC (scalable low-latency)
   - Shenandoah (concurrent compacting)

4. **Optimization**
   - Escape analysis
   - Stack allocation
   - Object pooling

## Testing

See `C/tests/phase7/` for comprehensive tests covering:
- Mark-sweep collection
- Generational collection
- Object promotion
- Write barriers
- Performance benchmarks
