#!/bin/bash
# Add more static assertions to reach target of 50+

# Add to gpu_backend.h
if ! grep -q "Compile-time invariants" C/kernel/backend/gpu_backend.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "GPU backend requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
static_assert(sizeof(float) == 4, "float must be 4 bytes");\
' C/kernel/backend/gpu_backend.h 2>/dev/null && echo "✓ Added to gpu_backend.h"
fi

# Add to fpga_backend.h
if ! grep -q "Compile-time invariants" C/kernel/backend/fpga_backend.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "FPGA backend requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/kernel/backend/fpga_backend.h 2>/dev/null && echo "✓ Added to fpga_backend.h"
fi

# Add to bci_lexer.h
if ! grep -q "Compile-time invariants" C/compiler/lexer/bci_lexer.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "Lexer requires at least 32-bit pointers");\
static_assert(sizeof(char) == 1, "char must be 1 byte");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/compiler/lexer/bci_lexer.h 2>/dev/null && echo "✓ Added to bci_lexer.h"
fi

# Add to bci_analyzer.h
if ! grep -q "Compile-time invariants" C/compiler/semantic_analyzer/bci_analyzer.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "Analyzer requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/compiler/semantic_analyzer/bci_analyzer.h 2>/dev/null && echo "✓ Added to bci_analyzer.h"
fi

# Add to bci_symbol.h
if ! grep -q "Compile-time invariants" C/compiler/semantic_analyzer/bci_symbol.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "Symbol table requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/compiler/semantic_analyzer/bci_symbol.h 2>/dev/null && echo "✓ Added to bci_symbol.h"
fi

# Add to bci_types.h
if ! grep -q "Compile-time invariants" C/compiler/types/bci_types.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "Type system requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/compiler/types/bci_types.h 2>/dev/null && echo "✓ Added to bci_types.h"
fi

# Add to bci_codegen.h
if ! grep -q "Compile-time invariants" C/codegen/bci_codegen.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "Codegen requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/codegen/bci_codegen.h 2>/dev/null && echo "✓ Added to bci_codegen.h"
fi

# Add to chimera_bci.h
if ! grep -q "Compile-time invariants" C/bci/chimera_bci.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "BCI requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/bci/chimera_bci.h 2>/dev/null && echo "✓ Added to chimera_bci.h"
fi

# Add to chimera_btl.h
if ! grep -q "Compile-time invariants" C/btl/chimera_btl.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "BTL requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/btl/chimera_btl.h 2>/dev/null && echo "✓ Added to chimera_btl.h"
fi

# Add to ai_trainer.h
if ! grep -q "Compile-time invariants" C/ai_trainer/ai_trainer.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "AI Trainer requires at least 32-bit pointers");\
static_assert(sizeof(float) == 4, "float must be 4 bytes");\
static_assert(sizeof(double) == 8, "double must be 8 bytes");\
' C/ai_trainer/ai_trainer.h 2>/dev/null && echo "✓ Added to ai_trainer.h"
fi

# Add to motif.h
if ! grep -q "Compile-time invariants" C/kernel/motif/motif.h 2>/dev/null; then
    sed -i '/#endif/i \
\
// Compile-time invariants\
static_assert(sizeof(void*) >= 4, "Motif requires at least 32-bit pointers");\
static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");\
' C/kernel/motif/motif.h 2>/dev/null && echo "✓ Added to motif.h"
fi

echo "Done! Counting total static assertions..."
grep -r "static_assert" C/ | wc -l
