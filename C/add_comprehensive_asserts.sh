#!/bin/bash
# Add comprehensive static assertions to all header files

echo "Adding comprehensive static assertions..."

# Add to scheduler.h
if ! grep -q "Compile-time invariants" C/kernel/scheduler/scheduler.h 2>/dev/null; then
    sed -i '/#endif.*SCHEDULER_H/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "Scheduler requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/kernel/scheduler/scheduler.h 2>/dev/null && echo "✓ Added to scheduler.h"
fi

# Add to graph.h
if ! grep -q "Compile-time invariants" C/kernel/graph/graph.h 2>/dev/null; then
    sed -i '/#endif.*GRAPH_H/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "Graph requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/kernel/graph/graph.h 2>/dev/null && echo "✓ Added to graph.h"
fi

# Add to bci_ast.h
if ! grep -q "Compile-time invariants" C/compiler/ast/bci_ast.h 2>/dev/null; then
    sed -i '/#endif.*AST_H/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "AST requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/compiler/ast/bci_ast.h 2>/dev/null && echo "✓ Added to bci_ast.h"
fi

# Add to bci_parser.h
if ! grep -q "Compile-time invariants" C/compiler/parser/bci_parser.h 2>/dev/null; then
    sed -i '/#endif.*PARSER_H/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "Parser requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/compiler/parser/bci_parser.h 2>/dev/null && echo "✓ Added to bci_parser.h"
fi

# Add to bci_chunk.h
if ! grep -q "Compile-time invariants" C/vm/bci_chunk.h 2>/dev/null; then
    sed -i '/#endif.*CHUNK_H/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "Chunk requires at least 32-bit pointers");\
static_assert(sizeof(uint8_t) == 1, "uint8_t must be 1 byte");\
' C/vm/bci_chunk.h 2>/dev/null && echo "✓ Added to bci_chunk.h"
fi

# Add to ham.h
if ! grep -q "Compile-time invariants" C/kernel/ham/ham.h 2>/dev/null; then
    sed -i '/#endif.*HAM_H/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "HAM requires at least 32-bit pointers");\
static_assert(sizeof(size_t) >= 4, "size_t must be at least 4 bytes");\
' C/kernel/ham/ham.h 2>/dev/null && echo "✓ Added to ham.h"
fi

# Add to fs.h
if ! grep -q "Compile-time invariants" C/kernel/file/fs.h 2>/dev/null; then
    sed -i '/#endif.*FS_H/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "FS requires at least 32-bit pointers");\
static_assert(sizeof(size_t) >= 4, "size_t must be at least 4 bytes");\
' C/kernel/file/fs.h 2>/dev/null && echo "✓ Added to fs.h"
fi

echo "Done adding static assertions!"
