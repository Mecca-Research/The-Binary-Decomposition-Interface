#!/bin/bash
# Phase 1: C23 Core Modernization Script
# Applies nullptr, attributes, static_assert, and typeof transformations

set -e

echo "=========================================="
echo "Phase 1: C23 Core Modernization"
echo "=========================================="
echo ""

# Phase 1.1: nullptr Migration
echo "[Phase 1.1] Replacing NULL with nullptr..."
find C -type f \( -name "*.c" -o -name "*.h" \) -exec sed -i 's/\bNULL\b/nullptr/g' {} +
NULL_COUNT=$(grep -r "nullptr" C | wc -l)
echo "  ✓ Replaced NULL with nullptr (${NULL_COUNT} occurrences)"
echo ""

echo "=========================================="
echo "Phase 1.1 Complete: nullptr Migration"
echo "=========================================="
