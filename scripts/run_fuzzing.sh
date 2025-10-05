
#!/bin/bash

# Comprehensive Fuzzing Execution Script for BDI Kernel
# 
# This script automates the execution of all fuzzing harnesses
# with proper configuration and monitoring.

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
FUZZING_DIR="$PROJECT_ROOT/C/fuzzing"
BUILD_DIR="$PROJECT_ROOT/build/fuzzing"
CRASHES_DIR="$FUZZING_DIR/crashes"
COVERAGE_DIR="$FUZZING_DIR/coverage"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging
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

# Check dependencies
check_dependencies() {
    log "Checking fuzzing dependencies..."
    
    local missing_deps=()
    
    # Check for AFL++
    if ! command -v afl-clang-fast &> /dev/null; then
        missing_deps+=("afl++")
    fi
    
    # Check for Clang with LibFuzzer
    if ! clang -fsanitize=fuzzer -x c /dev/null -o /dev/null 2>/dev/null; then
        missing_deps+=("clang with libfuzzer support")
    fi
    
    # Check for sanitizers
    if ! clang -fsanitize=address -x c /dev/null -o /dev/null 2>/dev/null; then
        missing_deps+=("clang with sanitizer support")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        error "Missing dependencies: ${missing_deps[*]}"
        error "Please install AFL++ and Clang with sanitizer support"
        exit 1
    fi
    
    success "All dependencies found"
}

# Setup fuzzing environment
setup_environment() {
    log "Setting up fuzzing environment..."
    
    # Create directories
    mkdir -p "$BUILD_DIR"
    mkdir -p "$CRASHES_DIR"
    mkdir -p "$COVERAGE_DIR"
    
    # Create subdirectories for each harness
    local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
    
    for harness in "${harnesses[@]}"; do
        mkdir -p "$CRASHES_DIR/$harness"
        mkdir -p "$COVERAGE_DIR/$harness"
    done
    
    # Load configurations
    if [ -f "$FUZZING_DIR/config/afl_config.txt" ]; then
        source "$FUZZING_DIR/config/afl_config.txt"
        log "Loaded AFL++ configuration"
    fi
    
    if [ -f "$FUZZING_DIR/config/sanitizer_config.txt" ]; then
        source "$FUZZING_DIR/config/sanitizer_config.txt"
        log "Loaded sanitizer configuration"
    fi
    
    success "Environment setup complete"
}

# Build fuzzing harnesses
build_harnesses() {
    log "Building fuzzing harnesses..."
    
    cd "$PROJECT_ROOT"
    
    # Build with AFL++
    log "Building AFL++ harnesses..."
    if make fuzz-afl; then
        success "AFL++ harnesses built successfully"
    else
        error "Failed to build AFL++ harnesses"
        return 1
    fi
    
    # Build with LibFuzzer
    log "Building LibFuzzer harnesses..."
    if make fuzz-libfuzzer; then
        success "LibFuzzer harnesses built successfully"
    else
        error "Failed to build LibFuzzer harnesses"
        return 1
    fi
    
    # Build with sanitizers
    log "Building sanitizer-enabled harnesses..."
    if make fuzz-sanitizers; then
        success "Sanitizer harnesses built successfully"
    else
        warning "Failed to build sanitizer harnesses (non-critical)"
    fi
}

# Run LibFuzzer harnesses
run_libfuzzer() {
    local harness=$1
    local duration=${2:-300}  # Default 5 minutes
    
    log "Running LibFuzzer harness: $harness"
    
    local binary="$BUILD_DIR/libfuzzer_${harness}"
    local corpus_dir="$FUZZING_DIR/corpus/${harness}_seeds"
    local crash_dir="$CRASHES_DIR/$harness"
    
    if [ ! -f "$binary" ]; then
        error "LibFuzzer binary not found: $binary"
        return 1
    fi
    
    # Create corpus directory if it doesn't exist
    mkdir -p "$corpus_dir"
    
    # Run LibFuzzer with timeout
    timeout "$duration" "$binary" \
        "$corpus_dir" \
        -artifact_prefix="$crash_dir/" \
        -max_total_time="$duration" \
        -print_final_stats=1 \
        -reload=30 \
        -workers=1 \
        -jobs=1 \
        2>&1 | tee "$COVERAGE_DIR/$harness/libfuzzer.log" || true
    
    # Check for crashes
    local crash_count=$(find "$crash_dir" -name "crash-*" 2>/dev/null | wc -l)
    if [ "$crash_count" -gt 0 ]; then
        warning "Found $crash_count crashes in $harness"
    else
        success "No crashes found in $harness"
    fi
}

# Run AFL++ harnesses
run_afl() {
    local harness=$1
    local duration=${2:-300}  # Default 5 minutes
    
    log "Running AFL++ harness: $harness"
    
    local binary="$BUILD_DIR/afl_${harness}"
    local corpus_dir="$FUZZING_DIR/corpus/${harness}_seeds"
    local output_dir="$CRASHES_DIR/$harness/afl_output"
    
    if [ ! -f "$binary" ]; then
        error "AFL++ binary not found: $binary"
        return 1
    fi
    
    # Create directories
    mkdir -p "$corpus_dir"
    mkdir -p "$output_dir"
    
    # Run AFL++ with timeout
    timeout "$duration" afl-fuzz \
        -i "$corpus_dir" \
        -o "$output_dir" \
        -t 1000 \
        -m none \
        -d \
        "$binary" @@ \
        2>&1 | tee "$COVERAGE_DIR/$harness/afl.log" || true
    
    # Check for crashes
    local crash_dir="$output_dir/default/crashes"
    if [ -d "$crash_dir" ]; then
        local crash_count=$(find "$crash_dir" -name "id:*" 2>/dev/null | wc -l)
        if [ "$crash_count" -gt 0 ]; then
            warning "Found $crash_count crashes in $harness"
        else
            success "No crashes found in $harness"
        fi
    fi
}

# Run all harnesses
run_all_harnesses() {
    local duration=${1:-300}  # Default 5 minutes per harness
    
    log "Running all fuzzing harnesses for $duration seconds each..."
    
    local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
    
    for harness in "${harnesses[@]}"; do
        log "Starting fuzzing campaign for $harness..."
        
        # Run LibFuzzer in background
        (run_libfuzzer "$harness" "$duration") &
        local libfuzzer_pid=$!
        
        # Run AFL++ in background  
        (run_afl "$harness" "$duration") &
        local afl_pid=$!
        
        # Wait for both to complete
        wait $libfuzzer_pid
        wait $afl_pid
        
        success "Completed fuzzing campaign for $harness"
    done
}

# Generate coverage report
generate_coverage_report() {
    log "Generating coverage report..."
    
    local report_file="$COVERAGE_DIR/fuzzing_report.txt"
    
    {
        echo "BDI Kernel Fuzzing Campaign Report"
        echo "Generated: $(date)"
        echo "=================================="
        echo
        
        echo "Harness Results:"
        echo "----------------"
        
        local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
        
        for harness in "${harnesses[@]}"; do
            echo "[$harness]"
            
            # Count crashes
            local crash_count=$(find "$CRASHES_DIR/$harness" -name "*crash*" -o -name "id:*" 2>/dev/null | wc -l)
            echo "  Crashes found: $crash_count"
            
            # Check log files
            if [ -f "$COVERAGE_DIR/$harness/libfuzzer.log" ]; then
                local executions=$(grep -o "exec/s: [0-9]*" "$COVERAGE_DIR/$harness/libfuzzer.log" | tail -1 | cut -d' ' -f2 || echo "0")
                echo "  LibFuzzer executions/sec: $executions"
            fi
            
            if [ -f "$COVERAGE_DIR/$harness/afl.log" ]; then
                local afl_execs=$(grep -o "execs_done : [0-9]*" "$COVERAGE_DIR/$harness/afl.log" | tail -1 | cut -d' ' -f3 || echo "0")
                echo "  AFL++ total executions: $afl_execs"
            fi
            
            echo
        done
        
        echo "Summary:"
        echo "--------"
        local total_crashes=$(find "$CRASHES_DIR" -name "*crash*" -o -name "id:*" 2>/dev/null | wc -l)
        echo "Total crashes found: $total_crashes"
        echo "Fuzzing completed: $(date)"
        
    } > "$report_file"
    
    success "Coverage report generated: $report_file"
    cat "$report_file"
}

# Main execution
main() {
    local duration=300  # Default 5 minutes per harness
    local mode="all"    # Default mode
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -d|--duration)
                duration="$2"
                shift 2
                ;;
            -m|--mode)
                mode="$2"
                shift 2
                ;;
            -h|--help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  -d, --duration SECONDS  Duration per harness (default: 300)"
                echo "  -m, --mode MODE         Mode: all, libfuzzer, afl (default: all)"
                echo "  -h, --help              Show this help"
                exit 0
                ;;
            *)
                error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    log "Starting BDI Kernel fuzzing campaign..."
    log "Duration per harness: $duration seconds"
    log "Mode: $mode"
    
    check_dependencies
    setup_environment
    build_harnesses
    
    case $mode in
        "all")
            run_all_harnesses "$duration"
            ;;
        "libfuzzer")
            local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
            for harness in "${harnesses[@]}"; do
                run_libfuzzer "$harness" "$duration"
            done
            ;;
        "afl")
            local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
            for harness in "${harnesses[@]}"; do
                run_afl "$harness" "$duration"
            done
            ;;
        *)
            error "Unknown mode: $mode"
            exit 1
            ;;
    esac
    
    generate_coverage_report
    
    success "Fuzzing campaign completed!"
}

# Run main function
main "$@"
