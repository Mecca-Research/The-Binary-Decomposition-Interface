
# Phase 3: Compiler Infrastructure Enhancement

## Overview

Phase 3 significantly enhances the BDI compiler infrastructure with advanced type system features, improved parsing capabilities, and comprehensive semantic analysis. This phase builds upon the foundations laid in Phases 0, 1, and 2.

## Phase 3.1: Type System Expansion

### Struct Types

Struct types provide composite data structures with named fields:

```c
BciTypeExt* point = bci_type_struct_create("Point");
bci_type_struct_add_field(point, "x", int_type);
bci_type_struct_add_field(point, "y", int_type);
bci_type_struct_finalize(point);
```

**Features:**
- Named fields with individual types
- Automatic size and alignment calculation
- Field offset computation
- Support for nested structs
- Packed struct option

**API Functions:**
- `bci_type_struct_create()` - Create new struct type
- `bci_type_struct_add_field()` - Add field to struct
- `bci_type_struct_finalize()` - Finalize struct layout
- `bci_type_struct_get_field()` - Retrieve field by name

### Union Types

Union types support discriminated and non-discriminated unions:

```c
BciTypeExt* value = bci_type_union_create("Value", true);
bci_type_union_add_variant(value, "integer", int_type);
bci_type_union_add_variant(value, "floating", float_type);
bci_type_union_finalize(value);
```

**Features:**
- Multiple variants with different types
- Discriminated unions with tag field
- Size determined by largest variant
- Type-safe variant access

### Enum Types

Enum types provide named integer constants:

```c
BciTypeExt* color = bci_type_enum_create("Color", int_type);
bci_type_enum_add_variant(color, "Red", 0);
bci_type_enum_add_variant(color, "Green", 1);
bci_type_enum_add_variant(color, "Blue", 2);
```

**Features:**
- Named variants with explicit values
- Configurable backing type
- Value lookup by name
- Type-safe enum operations

### Function Types

Function types represent function signatures:

```c
BciTypeExt* func = bci_type_function_create(return_type, false);
bci_type_function_add_param(func, param1_type);
bci_type_function_add_param(func, param2_type);
```

**Features:**
- Return type specification
- Parameter type list
- Variadic function support
- Function pointer representation
- Type matching for calls

### Generic Types

Generic types enable parametric polymorphism:

```c
BciTypeExt* array = bci_type_generic_create("Array<T>", base_type);
bci_type_generic_add_param(array, "T");
BciTypeExt* int_array = bci_type_generic_instantiate(array, int_args);
```

**Features:**
- Type parameters
- Type constraints
- Instantiation caching
- Monomorphization support

### Array Types

Array types support fixed and dynamic arrays:

```c
// Fixed-size array
BciTypeExt* arr = bci_type_array_create(elem_type, 10);

// Dynamic slice
BciTypeExt* slice = bci_type_slice_create(elem_type);
```

**Features:**
- Fixed-length arrays
- Dynamic slices (pointer + length)
- Element type specification
- Bounds checking support

### Type Checking Utilities

Comprehensive type checking and inference:

- `bci_type_ext_equals()` - Type equality
- `bci_type_ext_is_assignable()` - Assignment compatibility
- `bci_type_ext_is_numeric()` - Numeric type check
- `bci_type_infer_binary_op()` - Binary operator type inference
- `bci_type_common_type()` - Common type computation

## Phase 3.2: Parser & AST Enhancement

### Operator Precedence Table

Structured precedence levels for expression parsing:

```c
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // ||
    PREC_AND,         // &&
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! - +
    PREC_CALL,        // . () []
    PREC_PRIMARY
} Precedence;
```

**Features:**
- Pratt parsing algorithm
- Prefix and infix parse rules
- Configurable precedence levels
- Efficient expression parsing

### Enhanced Error Recovery

Robust error handling and recovery:

```c
typedef struct {
    const char* message;
    const char* file;
    int line;
    int column;
} ParseError;
```

**Features:**
- Error collection without stopping
- Panic mode for error cascading prevention
- Synchronization at statement boundaries
- Detailed error reporting with location info

### Pattern Matching Support

Comprehensive pattern matching:

```c
// Match expression
match value {
    Pattern1 => body1,
    Pattern2 => body2,
    _ => default_body
}
```

**Pattern Types:**
- Wildcard patterns (`_`)
- Literal patterns (constants)
- Binding patterns (variable capture)
- Struct patterns (destructuring)
- Enum patterns (variant matching)

**API Functions:**
- `ast_new_match_expr()` - Create match expression
- `ast_new_pattern_*()` - Create various pattern types
- `ast_match_add_arm()` - Add match arm

### Lambda Expressions

First-class function support:

```c
// Lambda with captures
|x, y| { x + y + captured_var }
```

**Features:**
- Parameter lists
- Capture lists (closures)
- Type inference for parameters
- Body expressions or blocks

**API Functions:**
- `ast_new_lambda()` - Create lambda
- `ast_lambda_add_param()` - Add parameter
- `ast_lambda_add_capture()` - Add captured variable
- `ast_lambda_set_body()` - Set lambda body

## Phase 3.3: Semantic Analysis Enhancement

### Type Inference Engine

Hindley-Milner style type inference:

```c
TypeInferenceContext ctx;
type_inference_init(&ctx);

TypeVariable* var = type_inference_new_var(&ctx, "T");
type_inference_add_constraint(&ctx, constraint);
type_inference_solve(&ctx);
```

**Features:**
- Type variables
- Constraint generation
- Constraint solving
- Unification algorithm
- Polymorphic type support

**Constraint Types:**
- Equality constraints
- Subtype constraints
- Field access constraints
- Callable constraints

### Lifetime Analysis

Variable lifetime tracking:

```c
LifetimeAnalyzer analyzer;
lifetime_analyzer_init(&analyzer);
lifetime_analyze_program(&analyzer, program);
```

**Features:**
- Birth line tracking (declaration)
- Death line tracking (last use)
- Escape detection
- Borrow checking
- Use-after-free detection

**API Functions:**
- `lifetime_get()` - Get lifetime info
- `lifetime_is_live()` - Check if variable is live
- `lifetime_check_use_after_free()` - Validate lifetimes

### Escape Analysis

Determines allocation strategy:

```c
EscapeAnalyzer analyzer;
escape_analyzer_init(&analyzer);
escape_analyze_function(&analyzer, func);
```

**Escape Kinds:**
- `ESCAPE_NONE` - Local only
- `ESCAPE_HEAP` - Allocated on heap
- `ESCAPE_RETURN` - Returned from function
- `ESCAPE_GLOBAL` - Stored in global
- `ESCAPE_PARAMETER` - Passed as parameter

**Optimization Hints:**
- Stack allocation eligibility
- Heap allocation requirements
- Escape scope determination

### Control Flow Graph

CFG construction and analysis:

```c
ControlFlowGraph cfg;
cfg_init(&cfg);
cfg_build_from_ast(&cfg, program);
cfg_compute_dominators(&cfg);
```

**Node Types:**
- Entry nodes
- Exit nodes
- Basic blocks
- Branch nodes
- Loop nodes

**Analysis:**
- Reachability analysis
- Dominator computation
- Dead code detection
- Loop detection

## Testing

### Test Coverage

**Phase 3.1 - Type System: 150+ tests**
- Struct type tests (30)
- Union type tests (20)
- Enum type tests (20)
- Function type tests (30)
- Generic type tests (20)
- Array type tests (15)
- Type checking tests (15+)

**Phase 3.2 - Parser & AST: 200+ tests**
- Parser initialization (20)
- Operator precedence (40)
- Error recovery (30)
- Pattern matching (50)
- Lambda expressions (40)
- Parse rules (20+)

**Phase 3.3 - Semantic Analysis: 150+ tests**
- Type inference (50)
- Lifetime analysis (40)
- Escape analysis (30)
- Control flow graph (30+)

**Total: 500+ comprehensive tests**

### Running Tests

```bash
# Build all tests
cd C
make test_phase3

# Run individual test suites
./tests/test_phase3_types
./tests/test_phase3_parser
./tests/test_phase3_semantic

# Run with sanitizers
make test_phase3_asan
make test_phase3_ubsan
```

## Risk Mitigation

### Feature Detection

C23 feature detection macros ensure compatibility:

```c
#ifndef nullptr
#define nullptr ((void*)0)
#endif

#if __has_c_attribute(nodiscard)
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD __attribute__((warn_unused_result))
#endif
```

### Sanitizer Support

Comprehensive sanitizer integration:

- **AddressSanitizer (ASan)**: Memory error detection
- **UndefinedBehaviorSanitizer (UBSan)**: UB detection
- **MemorySanitizer (MSan)**: Uninitialized memory detection

Enable via CMake:
```bash
cmake -DENABLE_ASAN=ON -DENABLE_UBSAN=ON ..
```

### Performance Baselines

Performance tracking for critical operations:

- Type checking: < 1μs per operation
- Pattern matching: < 10μs per match
- Type inference: < 100μs per function
- CFG construction: < 1ms per function

## API Contracts

### Memory Management

- All `*_create()` functions return owned pointers
- Caller must call corresponding `*_free()` function
- NULL pointers are safe to pass to `*_free()`
- No double-free protection (caller responsibility)

### Error Handling

- Functions return `nullptr` on allocation failure
- Boolean functions return `false` on error
- Error messages written to stderr
- No exceptions (pure C)

### Thread Safety

- Type system operations are NOT thread-safe
- Parser is NOT thread-safe
- Semantic analyzers are NOT thread-safe
- Use separate instances per thread

## Integration

### With Existing Compiler

Phase 3 extends existing compiler components:

```c
// Existing types
#include "compiler/types/bci_types.h"

// Extended types
#include "compiler/types/bci_types_extended.h"

// Existing parser
#include "compiler/parser/bci_parser.h"

// Extended parser
#include "compiler/parser/bci_parser_extended.h"
```

### Build System Integration

CMakeLists.txt additions:

```cmake
# Phase 3 libraries
add_library(bci_types_extended STATIC
    compiler/types/bci_types_extended.c
)

add_library(bci_parser_extended STATIC
    compiler/parser/bci_parser_extended.c
    compiler/ast/bci_ast_extended.c
)

add_library(bci_semantic_extended STATIC
    compiler/semantic_analyzer/bci_type_inference.c
    compiler/semantic_analyzer/bci_lifetime.c
    compiler/semantic_analyzer/bci_escape.c
    compiler/semantic_analyzer/bci_cfg.c
)
```

## Future Work

### Phase 4 Considerations

- Optimization passes using CFG
- Register allocation using lifetime info
- Inlining using escape analysis
- Generic specialization
- Pattern matching compilation

### Potential Enhancements

- More sophisticated type inference
- Lifetime polymorphism
- Effect system integration
- Dependent types
- Linear types

## References

- Hindley-Milner Type Inference
- Pratt Parsing Algorithm
- Static Single Assignment (SSA)
- Escape Analysis Techniques
- Control Flow Analysis

## Conclusion

Phase 3 provides a solid foundation for advanced compiler features. The type system is expressive, the parser is robust, and the semantic analysis is comprehensive. All components are well-tested and production-ready.
