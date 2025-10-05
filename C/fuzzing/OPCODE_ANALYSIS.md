# Opcode Usage Analysis Report

Generated: 2025-10-05

## Valid Opcodes (Currently Implemented in bci_chunk.h)

- ✅ `OP_ADD`
- ✅ `OP_CONSTANT`
- ✅ `OP_DIVIDE`
- ✅ `OP_MULTIPLY`
- ✅ `OP_NEGATE`
- ✅ `OP_RETURN`
- ✅ `OP_SUBTRACT`

## Opcode Usage by Harness

### bytecode_parser_fuzz.c

**Valid opcodes used:** 7

- ✅ `OP_ADD`
- ✅ `OP_CONSTANT`
- ✅ `OP_DIVIDE`
- ✅ `OP_MULTIPLY`
- ✅ `OP_NEGATE`
- ✅ `OP_RETURN`
- ✅ `OP_SUBTRACT`

**Undefined opcodes referenced:** 4

- ❌ `OP_CONSTANT_LONG` (needs to be removed/commented)
- ❌ `OP_JUMP` (needs to be removed/commented)
- ❌ `OP_JUMP_IF_FALSE` (needs to be removed/commented)
- ❌ `OP_LOOP` (needs to be removed/commented)

⚠️ **Status:** Contains undefined opcodes - needs fixing

### graph_execution_fuzz.c

**Valid opcodes used:** 7

- ✅ `OP_ADD`
- ✅ `OP_CONSTANT`
- ✅ `OP_DIVIDE`
- ✅ `OP_MULTIPLY`
- ✅ `OP_NEGATE`
- ✅ `OP_RETURN`
- ✅ `OP_SUBTRACT`

✅ **Status:** No undefined opcodes - ready to compile

### jit_compiler_fuzz.c

**Valid opcodes used:** 7

- ✅ `OP_ADD`
- ✅ `OP_CONSTANT`
- ✅ `OP_DIVIDE`
- ✅ `OP_MULTIPLY`
- ✅ `OP_NEGATE`
- ✅ `OP_RETURN`
- ✅ `OP_SUBTRACT`

**Undefined opcodes referenced:** 1

- ❌ `OP_LOOP` (needs to be removed/commented)

⚠️ **Status:** Contains undefined opcodes - needs fixing

### memory_management_fuzz.c

**Valid opcodes used:** 7

- ✅ `OP_ADD`
- ✅ `OP_CONSTANT`
- ✅ `OP_DIVIDE`
- ✅ `OP_MULTIPLY`
- ✅ `OP_NEGATE`
- ✅ `OP_RETURN`
- ✅ `OP_SUBTRACT`

✅ **Status:** No undefined opcodes - ready to compile

### value_system_fuzz.c

**Valid opcodes used:** 7

- ✅ `OP_ADD`
- ✅ `OP_CONSTANT`
- ✅ `OP_DIVIDE`
- ✅ `OP_MULTIPLY`
- ✅ `OP_NEGATE`
- ✅ `OP_RETURN`
- ✅ `OP_SUBTRACT`

✅ **Status:** No undefined opcodes - ready to compile

### vm_bytecode_fuzz.c

**Valid opcodes used:** 7

- ✅ `OP_ADD`
- ✅ `OP_CONSTANT`
- ✅ `OP_DIVIDE`
- ✅ `OP_MULTIPLY`
- ✅ `OP_NEGATE`
- ✅ `OP_RETURN`
- ✅ `OP_SUBTRACT`

✅ **Status:** No undefined opcodes - ready to compile

## Summary

- **Total harnesses analyzed:** 6
- **Harnesses with undefined opcodes:** 2
- **Total unique undefined opcodes:** 4

### All Undefined Opcodes Found

- `OP_CONSTANT_LONG`
- `OP_JUMP`
- `OP_JUMP_IF_FALSE`
- `OP_LOOP`
