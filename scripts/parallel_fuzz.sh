
#!/bin/bash

# Parallel Fuzzing Script for BDI Kernel
# 
# This script runs multiple fuzzing instances in parallel
# to maximize CPU utilization and coverage discovery.

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
FUZZING_DIR="$PROJECT_ROOT/C/fuzzing"
BUILD_DIR="$PROJECT_ROOT/build/fuzzing"

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

# Detect CPU cores
detect_cores() {
    local cores=$(nproc)
    log "Detected $cores CPU cores"
    echo $cores
}

# Run parallel AFL++ instances
run_parallel_afl() {
    local harness=$1
    local duration=$2
    local num_instances=$3
    
    log "Running $num_instances parallel AFL++ instances for $harness"
    
    local binary="$BUILD_DIR/afl_${harness}"
    local corpus_dir="$FUZZING_DIR/corpus/${harness}_seeds"
    local base_output_dir="$FUZZING_DIR/crashes/$harness/afl_parallel"
    
    if [ ! -f "$binary" ]; then
        error "AFL++ binary not found: $binary"
        return 1
    fi
    
    mkdir -p "$base_output_dir"
    
    # Start master instance
    log "Starting AFL++ master instance for $harness"
    timeout "$duration" afl-fuzz \
        -i "$corpus_dir" \
        -o "$base_output_dir" \
        -M master \
        -t 1000 \
        -m none \
        "$binary" @@ &
    
    local master_pid=$!
    
    # Start slave instances
    local pids=($master_pid)
    for ((i=1; i<num_instances; i++)); do
        log "Starting AFL++ slave instance $i for $harness"
        timeout "$duration" afl-fuzz \
            -i "$corpus_dir" \
            -o "$base_output_dir" \
            -S "slave$i" \
            -t 1000 \
            -m none \
            "$binary" @@ &
        
        pids+=($!)
        sleep 2  # Stagger startup
    done
    
    # Wait for all instances
    for pid in "${pids[@]}"; do
        wait $pid 2>/dev/null || true
    done
    
    success "Parallel AFL++ fuzzing completed for $harness"
}

# Run parallel LibFuzzer instances
run_parallel_libfuzzer() {
    local harness=$1
    local duration=$2
    local num_instances=$3
    
    log "Running $num_instances parallel LibFuzzer instances for $harness"
    
    local binary="$BUILD_DIR/libfuzzer_${harness}"
    local corpus_dir="$FUZZING_DIR/corpus/${harness}_seeds"
    local crash_dir="$FUZZING_DIR/crashes/$harness/libfuzzer_parallel"
    
    if [ ! -f "$binary" ]; then
        error "LibFuzzer binary not found: $binary"
        return 1
    fi
    
    mkdir -p "$crash_dir"
    
    # Start parallel instances
    local pids=()
    for ((i=0; i<num_instances; i++)); do
        local instance_crash_dir="$crash_dir/instance_$i"
        mkdir -p "$instance_crash_dir"
        
        log "Starting LibFuzzer instance $i for $harness"
        timeout "$duration" "$binary" \
            "$corpus_dir" \
            -artifact_prefix="$instance_crash_dir/" \
            -max_total_time="$duration" \
            -workers=1 \
            -jobs=1 \
            -reload=30 \
            > "$instance_crash_dir/output.log" 2>&1 &
        
        pids+=($!)
        sleep 1  # Stagger startup
    done
    
    # Wait for all instances
    for pid in "${pids[@]}"; do
        wait $pid 2>/dev/null || true
    done
    
    success "Parallel LibFuzzer fuzzing completed for $harness"
}

# Run distributed fuzzing
run_distributed_fuzzing() {
    local duration=$1
    local cores=$2
    
    log "Starting distributed fuzzing campaign"
    log "Duration: $duration seconds per harness"
    log "CPU cores: $cores"
    
    local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
    local instances_per_harness=$((cores / ${#harnesses[@]}))
    
    if [ $instances_per_harness -lt 1 ]; then
        instances_per_harness=1
    fi
    
    log "Running $instances_per_harness instances per harness"
    
    # Run harnesses in parallel
    local harness_pids=()
    
    for harness in "${harnesses[@]}"; do
        log "Starting parallel fuzzing for $harness"
        
        # Run AFL++ in background
        (run_parallel_afl "$harness" "$duration" "$instances_per_harness") &
        harness_pids+=($!)
        
        # Run LibFuzzer in background
        (run_parallel_libfuzzer "$harness" "$duration" "$instances_per_harness") &
        harness_pids+=($!)
        
        sleep 5  # Stagger harness startup
    done
    
    # Wait for all harnesses to complete
    log "Waiting for all fuzzing campaigns to complete..."
    for pid in "${harness_pids[@]}"; do
        wait $pid 2>/dev/null || true
    done
    
    success "All parallel fuzzing campaigns completed"
}

# Synchronize AFL++ findings
sync_afl_findings() {
    log "Synchronizing AFL++ findings..."
    
    local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
    
    for harness in "${harnesses[@]}"; do
        local sync_dir="$FUZZING_DIR/crashes/$harness/afl_parallel"
        
        if [ -d "$sync_dir" ]; then
            log "Syncing findings for $harness"
            
            # Use afl-whatsup to get status
            if command -v afl-whatsup &> /dev/null; then
                afl-whatsup "$sync_dir" > "$sync_dir/sync_status.txt" 2>/dev/null || true
            fi
            
            # Collect unique crashes
            local unique_crashes_dir="$sync_dir/unique_crashes"
            mkdir -p "$unique_crashes_dir"
            
            find "$sync_dir" -path "*/crashes/id:*" -type f -exec cp {} "$unique_crashes_dir/" \; 2>/dev/null || true
            
            local crash_count=$(ls "$unique_crashes_dir" 2>/dev/null | wc -l)
            log "Found $crash_count unique crashes for $harness"
        fi
    done
    
    success "AFL++ findings synchronized"
}

# Generate parallel fuzzing report
generate_parallel_report() {
    log "Generating parallel fuzzing report..."
    
    local report_file="$FUZZING_DIR/coverage/parallel_fuzzing_report.txt"
    
    {
        echo "BDI Kernel Parallel Fuzzing Campaign Report"
        echo "Generated: $(date)"
        echo "==========================================="
        echo
        
        local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
        
        for harness in "${harnesses[@]}"; do
            echo "[$harness]"
            
            # AFL++ results
            local afl_dir="$FUZZING_DIR/crashes/$harness/afl_parallel"
            if [ -d "$afl_dir" ]; then
                local afl_crashes=$(find "$afl_dir" -path "*/crashes/id:*" 2>/dev/null | wc -l)
                echo "  AFL++ crashes: $afl_crashes"
                
                if [ -f "$afl_dir/sync_status.txt" ]; then
                    local total_execs=$(grep -o "Total execs : [0-9]*" "$afl_dir/sync_status.txt" | cut -d' ' -f4 || echo "0")
                    echo "  AFL++ total executions: $total_execs"
                fi
            fi
            
            # LibFuzzer results
            local libfuzzer_dir="$FUZZING_DIR/crashes/$harness/libfuzzer_parallel"
            if [ -d "$libfuzzer_dir" ]; then
                local libfuzzer_crashes=$(find "$libfuzzer_dir" -name "*crash*" 2>/dev/null | wc -l)
                echo "  LibFuzzer crashes: $libfuzzer_crashes"
            fi
            
            echo
        done
        
        echo "Summary:"
        echo "--------"
        local total_afl_crashes=$(find "$FUZZING_DIR/crashes" -path "*/afl_parallel/*/crashes/id:*" 2>/dev/null | wc -l)
        local total_libfuzzer_crashes=$(find "$FUZZING_DIR/crashes" -path "*/libfuzzer_parallel/*crash*" 2>/dev/null | wc -l)
        
        echo "Total AFL++ crashes: $total_afl_crashes"
        echo "Total LibFuzzer crashes: $total_libfuzzer_crashes"
        echo "Total crashes: $((total_afl_crashes + total_libfuzzer_crashes))"
        echo "Parallel fuzzing completed: $(date)"
        
    } > "$report_file"
    
    success "Parallel fuzzing report generated: $report_file"
    cat "$report_file"
}

# Main execution
main() {
    local duration=1800  # Default 30 minutes
    local cores=$(detect_cores)
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -d|--duration)
                duration="$2"
                shift 2
                ;;
            -c|--cores)
                cores="$2"
                shift 2
                ;;
            -h|--help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  -d, --duration SECONDS  Duration per harness (default: 1800)"
                echo "  -c, --cores CORES       Number of CPU cores to use (default: auto-detect)"
                echo "  -h, --help              Show this help"
                exit 0
                ;;
            *)
                error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    log "Starting parallel fuzzing campaign..."
    log "Duration: $duration seconds"
    log "CPU cores: $cores"
    
    # Ensure harnesses are built
    cd "$PROJECT_ROOT"
    make fuzz-afl fuzz-libfuzzer
    
    # Run distributed fuzzing
    run_distributed_fuzzing "$duration" "$cores"
    
    # Synchronize findings
    sync_afl_findings
    
    # Generate report
    generate_parallel_report
    
    success "Parallel fuzzing campaign completed!"
}

# Run main function
main "$@"
