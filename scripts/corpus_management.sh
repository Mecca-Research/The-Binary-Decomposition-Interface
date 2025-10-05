
#!/bin/bash

# Corpus Management Script for BDI Kernel Fuzzing
# 
# This script manages fuzzing corpus, including minimization,
# seed generation, and corpus optimization.

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
FUZZING_DIR="$PROJECT_ROOT/C/fuzzing"
BUILD_DIR="$PROJECT_ROOT/build/fuzzing"
CORPUS_DIR="$FUZZING_DIR/corpus"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

log() {
    echo -e "${BLUE}[$(date '+%Y-%m-%d %H:%M:%S')]${NC} $1"
}

error() {
    echo -e "${RED}[ERROR]${NC} $1" >&2
}

success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Generate seed files for VM bytecode fuzzing
generate_vm_seeds() {
    log "Generating VM bytecode seeds..."
    
    local seed_dir="$CORPUS_DIR/vm_seeds"
    mkdir -p "$seed_dir"
    
    # Basic opcode sequences
    {
        # Simple constant and return
        printf '\x00\x00\x01'  # OP_CONSTANT 0, OP_RETURN
        
    } > "$seed_dir/constant_return.bin"
    
    {
        # Arithmetic operations
        printf '\x00\x00\x00\x01\x02\x01'  # CONSTANT 0, CONSTANT 1, ADD, RETURN
        
    } > "$seed_dir/arithmetic.bin"
    
    {
        # Control flow
        printf '\x00\x00\x06\x00\x05\x01'  # CONSTANT 0, JUMP_IF_FALSE 5, RETURN
        
    } > "$seed_dir/control_flow.bin"
    
    {
        # Loop structure
        printf '\x00\x00\x00\x01\x07\x00\x03\x01'  # CONSTANT 0, CONSTANT 1, LOOP 3, RETURN
        
    } > "$seed_dir/loop.bin"
    
    success "Generated VM bytecode seeds"
}

# Generate seed files for JIT compiler fuzzing
generate_jit_seeds() {
    log "Generating JIT compiler seeds..."
    
    local seed_dir="$CORPUS_DIR/jit_seeds"
    mkdir -p "$seed_dir"
    
    # Hotspot-triggering loop
    {
        printf '\x00\x00'      # OP_CONSTANT 0
        printf '\x00\x01'      # OP_CONSTANT 1
        printf '\x02'          # OP_ADD
        printf '\x07\x00\x06'  # OP_LOOP (jump back 6 bytes)
        printf '\x01'          # OP_RETURN
        
    } > "$seed_dir/hotspot_loop.bin"
    
    # Complex arithmetic for JIT optimization
    {
        printf '\x00\x00'      # OP_CONSTANT 0
        printf '\x00\x01'      # OP_CONSTANT 1
        printf '\x02'          # OP_ADD
        printf '\x00\x02'      # OP_CONSTANT 2
        printf '\x03'          # OP_SUBTRACT
        printf '\x00\x03'      # OP_CONSTANT 3
        printf '\x04'          # OP_MULTIPLY
        printf '\x01'          # OP_RETURN
        
    } > "$seed_dir/complex_arithmetic.bin"
    
    success "Generated JIT compiler seeds"
}

# Generate seed files for graph execution fuzzing
generate_graph_seeds() {
    log "Generating graph execution seeds..."
    
    local seed_dir="$CORPUS_DIR/graph_seeds"
    mkdir -p "$seed_dir"
    
    # Simple graph: 2 nodes, 1 edge
    {
        printf '\x02'          # 2 nodes
        printf '\x00'          # Node 0 type
        printf '\x00\x00\x00\x00\x00\x00\xf0\x3f'  # Value 1.0 (double)
        printf '\x01'          # Node 1 type
        printf '\x00\x00\x00\x00\x00\x00\x00\x40'  # Value 2.0 (double)
        printf '\x00\x01'      # Edge from node 0 to node 1
        
    } > "$seed_dir/simple_graph.bin"
    
    # Complex graph: 4 nodes, multiple edges
    {
        printf '\x04'          # 4 nodes
        printf '\x00'          # Node 0
        printf '\x00\x00\x00\x00\x00\x00\xf0\x3f'  # 1.0
        printf '\x01'          # Node 1
        printf '\x00\x00\x00\x00\x00\x00\x00\x40'  # 2.0
        printf '\x02'          # Node 2
        printf '\x00\x00\x00\x00\x00\x00\x08\x40'  # 3.0
        printf '\x00'          # Node 3
        printf '\x00\x00\x00\x00\x00\x00\x10\x40'  # 4.0
        printf '\x00\x01'      # Edge 0->1
        printf '\x01\x02'      # Edge 1->2
        printf '\x02\x03'      # Edge 2->3
        printf '\x00\x03'      # Edge 0->3
        
    } > "$seed_dir/complex_graph.bin"
    
    success "Generated graph execution seeds"
}

# Generate seed files for memory management fuzzing
generate_memory_seeds() {
    log "Generating memory management seeds..."
    
    local seed_dir="$CORPUS_DIR/memory_seeds"
    mkdir -p "$seed_dir"
    
    # Allocation pattern
    {
        printf '\x08'          # 8 operations
        printf '\x00\x01\x00'  # Allocate 256 bytes
        printf '\x00\x02\x00'  # Allocate 512 bytes
        printf '\x02'          # Free previous
        printf '\x01\x04\x00'  # Reallocate 1024 bytes
        printf '\x03'          # Trigger GC
        printf '\x02'          # Free
        printf '\x03'          # Trigger GC
        printf '\x00\x00\x10'  # Allocate 4096 bytes
        
    } > "$seed_dir/alloc_pattern.bin"
    
    # Stress pattern
    {
        printf '\x10'          # 16 operations
        for i in {1..16}; do
            printf '\x00\x01\x00'  # Allocate
        done
        
    } > "$seed_dir/stress_pattern.bin"
    
    success "Generated memory management seeds"
}

# Generate seed files for bytecode parser fuzzing
generate_parser_seeds() {
    log "Generating bytecode parser seeds..."
    
    local seed_dir="$CORPUS_DIR/parser_seeds"
    mkdir -p "$seed_dir"
    
    # Valid bytecode header
    {
        printf 'BDIC'          # Magic
        printf '\x01\x00\x00\x00'  # Version 1
        printf '\x04\x00\x00\x00'  # Code size 4
        printf '\x01\x00\x00\x00'  # 1 constant
        printf '\x00\x00\x01\x00'  # Code: CONSTANT 0, RETURN
        printf '\x00\x00\x00\x00\x00\x00\xf0\x3f'  # Constant: 1.0
        
    } > "$seed_dir/valid_bytecode.bin"
    
    # Malformed header (for testing parser robustness)
    {
        printf 'XXXX'          # Invalid magic
        printf '\xff\xff\xff\xff'  # Invalid version
        printf '\x00\x00\x00\x00'  # Zero code size
        printf '\x00\x00\x00\x00'  # Zero constants
        
    } > "$seed_dir/malformed_header.bin"
    
    success "Generated bytecode parser seeds"
}

# Generate seed files for value system fuzzing
generate_value_seeds() {
    log "Generating value system seeds..."
    
    local seed_dir="$CORPUS_DIR/values_seeds"
    mkdir -p "$seed_dir"
    
    # Mixed value types
    {
        printf '\x04'          # 4 values
        printf '\x00'          # Number
        printf '\x00\x00\x00\x00\x00\x00\xf0\x3f'  # 1.0
        printf '\x01\x01'      # Boolean true
        printf '\x02'          # Nil
        printf '\x03\x05hello' # String "hello"
        
    } > "$seed_dir/mixed_values.bin"
    
    # Edge case numbers
    {
        printf '\x03'          # 3 values
        printf '\x00'          # Number
        printf '\xff\xff\xff\xff\xff\xff\xef\x7f'  # Max double
        printf '\x00'          # Number
        printf '\x01\x00\x00\x00\x00\x00\x00\x00'  # Min positive double
        printf '\x00'          # Number
        printf '\x00\x00\x00\x00\x00\x00\xf8\x7f'  # NaN
        
    } > "$seed_dir/edge_numbers.bin"
    
    success "Generated value system seeds"
}

# Minimize corpus using AFL tools
minimize_corpus() {
    local harness=$1
    
    log "Minimizing corpus for harness: $harness"
    
    local corpus_dir="$CORPUS_DIR/${harness}_seeds"
    local minimized_dir="$CORPUS_DIR/${harness}_minimized"
    local binary="$BUILD_DIR/afl_${harness}"
    
    if [ ! -f "$binary" ]; then
        warning "AFL binary not found: $binary"
        return 1
    fi
    
    if [ ! -d "$corpus_dir" ]; then
        warning "Corpus directory not found: $corpus_dir"
        return 1
    fi
    
    mkdir -p "$minimized_dir"
    
    # Use afl-cmin to minimize corpus
    if command -v afl-cmin &> /dev/null; then
        log "Running afl-cmin for $harness..."
        
        timeout 300 afl-cmin \
            -i "$corpus_dir" \
            -o "$minimized_dir" \
            -t 1000 \
            "$binary" @@ 2>/dev/null || true
        
        local original_count=$(find "$corpus_dir" -type f | wc -l)
        local minimized_count=$(find "$minimized_dir" -type f | wc -l)
        
        log "Corpus minimized: $original_count -> $minimized_count files"
    else
        warning "afl-cmin not found, skipping corpus minimization"
    fi
    
    success "Corpus minimization completed for $harness"
}

# Merge corpus from fuzzing campaigns
merge_corpus() {
    local harness=$1
    
    log "Merging corpus for harness: $harness"
    
    local base_corpus="$CORPUS_DIR/${harness}_seeds"
    local merged_corpus="$CORPUS_DIR/${harness}_merged"
    local crashes_dir="$FUZZING_DIR/crashes/$harness"
    
    mkdir -p "$merged_corpus"
    
    # Copy original seeds
    if [ -d "$base_corpus" ]; then
        cp "$base_corpus"/* "$merged_corpus/" 2>/dev/null || true
    fi
    
    # Find and copy interesting inputs from AFL
    local afl_queue_dirs=$(find "$crashes_dir" -name "queue" -type d 2>/dev/null || true)
    for queue_dir in $afl_queue_dirs; do
        if [ -d "$queue_dir" ]; then
            log "Merging AFL queue: $queue_dir"
            cp "$queue_dir"/id:* "$merged_corpus/" 2>/dev/null || true
        fi
    done
    
    # Find and copy LibFuzzer corpus
    local libfuzzer_corpus=$(find "$crashes_dir" -name "libfuzzer_corpus" -type d 2>/dev/null || true)
    for corpus_dir in $libfuzzer_corpus; do
        if [ -d "$corpus_dir" ]; then
            log "Merging LibFuzzer corpus: $corpus_dir"
            cp "$corpus_dir"/* "$merged_corpus/" 2>/dev/null || true
        fi
    done
    
    local merged_count=$(find "$merged_corpus" -type f | wc -l)
    log "Merged corpus contains $merged_count files"
    
    success "Corpus merge completed for $harness"
}

# Generate corpus statistics
generate_corpus_stats() {
    log "Generating corpus statistics..."
    
    local stats_file="$CORPUS_DIR/corpus_stats.txt"
    
    {
        echo "BDI Kernel Fuzzing Corpus Statistics"
        echo "Generated: $(date)"
        echo "===================================="
        echo
        
        local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
        
        for harness in "${harnesses[@]}"; do
            echo "[$harness]"
            
            local seed_dir="$CORPUS_DIR/${harness}_seeds"
            local minimized_dir="$CORPUS_DIR/${harness}_minimized"
            local merged_dir="$CORPUS_DIR/${harness}_merged"
            
            if [ -d "$seed_dir" ]; then
                local seed_count=$(find "$seed_dir" -type f | wc -l)
                local seed_size=$(du -sh "$seed_dir" 2>/dev/null | cut -f1 || echo "0")
                echo "  Original seeds: $seed_count files ($seed_size)"
            fi
            
            if [ -d "$minimized_dir" ]; then
                local min_count=$(find "$minimized_dir" -type f | wc -l)
                local min_size=$(du -sh "$minimized_dir" 2>/dev/null | cut -f1 || echo "0")
                echo "  Minimized corpus: $min_count files ($min_size)"
            fi
            
            if [ -d "$merged_dir" ]; then
                local merged_count=$(find "$merged_dir" -type f | wc -l)
                local merged_size=$(du -sh "$merged_dir" 2>/dev/null | cut -f1 || echo "0")
                echo "  Merged corpus: $merged_count files ($merged_size)"
            fi
            
            echo
        done
        
        echo "Total corpus size: $(du -sh "$CORPUS_DIR" 2>/dev/null | cut -f1 || echo "0")"
        echo "Statistics generated: $(date)"
        
    } > "$stats_file"
    
    success "Corpus statistics generated: $stats_file"
    cat "$stats_file"
}

# Main execution
main() {
    local action="generate"
    local harness=""
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -a|--action)
                action="$2"
                shift 2
                ;;
            -h|--harness)
                harness="$2"
                shift 2
                ;;
            --help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  -a, --action ACTION     Action: generate, minimize, merge, stats (default: generate)"
                echo "  -h, --harness HARNESS   Target specific harness"
                echo "  --help                  Show this help"
                echo
                echo "Actions:"
                echo "  generate    Generate seed files for all harnesses"
                echo "  minimize    Minimize corpus using AFL tools"
                echo "  merge       Merge corpus from fuzzing campaigns"
                echo "  stats       Generate corpus statistics"
                exit 0
                ;;
            *)
                error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    log "Starting corpus management..."
    log "Action: $action"
    
    case $action in
        "generate")
            log "Generating seed files..."
            generate_vm_seeds
            generate_jit_seeds
            generate_graph_seeds
            generate_memory_seeds
            generate_parser_seeds
            generate_value_seeds
            success "All seed files generated"
            ;;
            
        "minimize")
            if [ -n "$harness" ]; then
                minimize_corpus "$harness"
            else
                local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
                for h in "${harnesses[@]}"; do
                    minimize_corpus "$h"
                done
            fi
            ;;
            
        "merge")
            if [ -n "$harness" ]; then
                merge_corpus "$harness"
            else
                local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
                for h in "${harnesses[@]}"; do
                    merge_corpus "$h"
                done
            fi
            ;;
            
        "stats")
            generate_corpus_stats
            ;;
            
        *)
            error "Unknown action: $action"
            exit 1
            ;;
    esac
    
    success "Corpus management completed!"
}

# Run main function
main "$@"
