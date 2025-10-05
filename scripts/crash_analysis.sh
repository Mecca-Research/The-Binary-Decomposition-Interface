
#!/bin/bash

# Crash Analysis Script for BDI Kernel Fuzzing
# 
# This script analyzes crashes found during fuzzing campaigns,
# reproduces them, and generates detailed reports.

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
FUZZING_DIR="$PROJECT_ROOT/C/fuzzing"
BUILD_DIR="$PROJECT_ROOT/build/fuzzing"
CRASHES_DIR="$FUZZING_DIR/crashes"

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

# Check if GDB is available
check_gdb() {
    if ! command -v gdb &> /dev/null; then
        warning "GDB not found. Stack traces will be limited."
        return 1
    fi
    return 0
}

# Check if addr2line is available
check_addr2line() {
    if ! command -v addr2line &> /dev/null; then
        warning "addr2line not found. Symbol resolution will be limited."
        return 1
    fi
    return 0
}

# Reproduce a crash
reproduce_crash() {
    local harness=$1
    local crash_file=$2
    local fuzzer_type=$3  # "afl" or "libfuzzer"
    
    log "Reproducing crash: $crash_file"
    
    local binary=""
    case $fuzzer_type in
        "afl")
            binary="$BUILD_DIR/afl_${harness}"
            ;;
        "libfuzzer")
            binary="$BUILD_DIR/libfuzzer_${harness}"
            ;;
        *)
            error "Unknown fuzzer type: $fuzzer_type"
            return 1
            ;;
    esac
    
    if [ ! -f "$binary" ]; then
        error "Binary not found: $binary"
        return 1
    fi
    
    if [ ! -f "$crash_file" ]; then
        error "Crash file not found: $crash_file"
        return 1
    fi
    
    local crash_name=$(basename "$crash_file")
    local output_dir="$(dirname "$crash_file")/analysis"
    mkdir -p "$output_dir"
    
    local analysis_file="$output_dir/${crash_name}.analysis"
    
    {
        echo "Crash Analysis Report"
        echo "===================="
        echo "Harness: $harness"
        echo "Fuzzer: $fuzzer_type"
        echo "Crash file: $crash_file"
        echo "Binary: $binary"
        echo "Analysis time: $(date)"
        echo
        
        # Basic file info
        echo "Crash File Information:"
        echo "----------------------"
        ls -la "$crash_file"
        echo "File size: $(stat -c%s "$crash_file") bytes"
        echo "File type: $(file "$crash_file")"
        echo
        
        # Hexdump of crash input
        echo "Crash Input (hex dump, first 256 bytes):"
        echo "----------------------------------------"
        hexdump -C "$crash_file" | head -16
        echo
        
        # Try to reproduce the crash
        echo "Crash Reproduction:"
        echo "------------------"
        
        if [ "$fuzzer_type" = "afl" ]; then
            # AFL crash reproduction
            echo "Running: $binary $crash_file"
            timeout 10 "$binary" "$crash_file" 2>&1 || echo "Process exited with code $?"
        else
            # LibFuzzer crash reproduction
            echo "Running: $binary $crash_file"
            timeout 10 "$binary" "$crash_file" 2>&1 || echo "Process exited with code $?"
        fi
        
        echo
        
    } > "$analysis_file"
    
    # Try to get stack trace with GDB
    if check_gdb; then
        log "Getting stack trace with GDB..."
        
        {
            echo "Stack Trace (GDB):"
            echo "-----------------"
            
            if [ "$fuzzer_type" = "afl" ]; then
                timeout 30 gdb -batch -ex "run $crash_file" -ex "bt" -ex "quit" "$binary" 2>&1 || true
            else
                # For LibFuzzer, we need to pass the input differently
                timeout 30 gdb -batch -ex "run $crash_file" -ex "bt" -ex "quit" "$binary" 2>&1 || true
            fi
            
            echo
            
        } >> "$analysis_file"
    fi
    
    # Try to get more detailed crash info with sanitizers
    if [ -f "$BUILD_DIR/sanitizer_${harness}" ]; then
        log "Running with sanitizers for detailed analysis..."
        
        {
            echo "Sanitizer Analysis:"
            echo "------------------"
            
            # Set sanitizer options for detailed output
            export ASAN_OPTIONS="abort_on_error=0:print_stacktrace=1:symbolize=1"
            export UBSAN_OPTIONS="print_stacktrace=1:symbolize=1"
            
            if [ "$fuzzer_type" = "afl" ]; then
                timeout 10 "$BUILD_DIR/sanitizer_${harness}" "$crash_file" 2>&1 || true
            else
                timeout 10 "$BUILD_DIR/sanitizer_${harness}" "$crash_file" 2>&1 || true
            fi
            
            echo
            
        } >> "$analysis_file"
    fi
    
    success "Crash analysis saved to: $analysis_file"
    return 0
}

# Classify crash type
classify_crash() {
    local analysis_file=$1
    
    if [ ! -f "$analysis_file" ]; then
        return 1
    fi
    
    local crash_type="Unknown"
    
    # Check for common crash patterns
    if grep -q "heap-buffer-overflow" "$analysis_file"; then
        crash_type="Heap Buffer Overflow"
    elif grep -q "stack-buffer-overflow" "$analysis_file"; then
        crash_type="Stack Buffer Overflow"
    elif grep -q "use-after-free" "$analysis_file"; then
        crash_type="Use After Free"
    elif grep -q "double-free" "$analysis_file"; then
        crash_type="Double Free"
    elif grep -q "null-pointer-dereference" "$analysis_file"; then
        crash_type="Null Pointer Dereference"
    elif grep -q "integer-overflow" "$analysis_file"; then
        crash_type="Integer Overflow"
    elif grep -q "division-by-zero" "$analysis_file"; then
        crash_type="Division by Zero"
    elif grep -q "SIGSEGV" "$analysis_file"; then
        crash_type="Segmentation Fault"
    elif grep -q "SIGABRT" "$analysis_file"; then
        crash_type="Abort Signal"
    elif grep -q "timeout" "$analysis_file"; then
        crash_type="Timeout/Hang"
    fi
    
    echo "$crash_type"
}

# Analyze all crashes for a harness
analyze_harness_crashes() {
    local harness=$1
    
    log "Analyzing crashes for harness: $harness"
    
    local harness_crash_dir="$CRASHES_DIR/$harness"
    local total_crashes=0
    local analyzed_crashes=0
    
    # Find AFL crashes
    local afl_crashes=$(find "$harness_crash_dir" -path "*/crashes/id:*" -type f 2>/dev/null || true)
    for crash_file in $afl_crashes; do
        if [ -f "$crash_file" ]; then
            total_crashes=$((total_crashes + 1))
            if reproduce_crash "$harness" "$crash_file" "afl"; then
                analyzed_crashes=$((analyzed_crashes + 1))
            fi
        fi
    done
    
    # Find LibFuzzer crashes
    local libfuzzer_crashes=$(find "$harness_crash_dir" -name "*crash*" -type f 2>/dev/null || true)
    for crash_file in $libfuzzer_crashes; do
        if [ -f "$crash_file" ]; then
            total_crashes=$((total_crashes + 1))
            if reproduce_crash "$harness" "$crash_file" "libfuzzer"; then
                analyzed_crashes=$((analyzed_crashes + 1))
            fi
        fi
    done
    
    log "Analyzed $analyzed_crashes out of $total_crashes crashes for $harness"
}

# Generate crash summary report
generate_crash_summary() {
    log "Generating crash summary report..."
    
    local summary_file="$CRASHES_DIR/crash_summary.txt"
    
    {
        echo "BDI Kernel Fuzzing Crash Summary"
        echo "Generated: $(date)"
        echo "==============================="
        echo
        
        local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
        local total_crashes=0
        
        for harness in "${harnesses[@]}"; do
            echo "[$harness]"
            
            local harness_crash_dir="$CRASHES_DIR/$harness"
            local harness_crashes=0
            
            # Count crashes
            local afl_crashes=$(find "$harness_crash_dir" -path "*/crashes/id:*" -type f 2>/dev/null | wc -l)
            local libfuzzer_crashes=$(find "$harness_crash_dir" -name "*crash*" -type f 2>/dev/null | wc -l)
            harness_crashes=$((afl_crashes + libfuzzer_crashes))
            total_crashes=$((total_crashes + harness_crashes))
            
            echo "  Total crashes: $harness_crashes"
            echo "  AFL++ crashes: $afl_crashes"
            echo "  LibFuzzer crashes: $libfuzzer_crashes"
            
            # Classify crashes
            local analysis_files=$(find "$harness_crash_dir" -name "*.analysis" 2>/dev/null || true)
            if [ -n "$analysis_files" ]; then
                echo "  Crash types:"
                
                declare -A crash_types
                for analysis_file in $analysis_files; do
                    local crash_type=$(classify_crash "$analysis_file")
                    crash_types["$crash_type"]=$((${crash_types["$crash_type"]} + 1))
                done
                
                for crash_type in "${!crash_types[@]}"; do
                    echo "    $crash_type: ${crash_types[$crash_type]}"
                done
            fi
            
            echo
        done
        
        echo "Overall Summary:"
        echo "---------------"
        echo "Total crashes found: $total_crashes"
        echo "Analysis completed: $(date)"
        
    } > "$summary_file"
    
    success "Crash summary generated: $summary_file"
    cat "$summary_file"
}

# Minimize crash inputs
minimize_crashes() {
    local harness=$1
    
    log "Minimizing crashes for harness: $harness"
    
    local harness_crash_dir="$CRASHES_DIR/$harness"
    
    # Find AFL crashes and minimize them
    local afl_crashes=$(find "$harness_crash_dir" -path "*/crashes/id:*" -type f 2>/dev/null || true)
    for crash_file in $afl_crashes; do
        if [ -f "$crash_file" ]; then
            local minimized_file="${crash_file}.min"
            
            if command -v afl-tmin &> /dev/null; then
                log "Minimizing AFL crash: $(basename "$crash_file")"
                
                timeout 300 afl-tmin \
                    -i "$crash_file" \
                    -o "$minimized_file" \
                    -t 1000 \
                    "$BUILD_DIR/afl_${harness}" @@ 2>/dev/null || true
                
                if [ -f "$minimized_file" ]; then
                    local original_size=$(stat -c%s "$crash_file")
                    local minimized_size=$(stat -c%s "$minimized_file")
                    log "Minimized $(basename "$crash_file"): $original_size -> $minimized_size bytes"
                fi
            fi
        fi
    done
    
    success "Crash minimization completed for $harness"
}

# Main execution
main() {
    local harness=""
    local minimize=false
    
    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--harness)
                harness="$2"
                shift 2
                ;;
            -m|--minimize)
                minimize=true
                shift
                ;;
            --help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  -h, --harness HARNESS   Analyze specific harness only"
                echo "  -m, --minimize          Minimize crash inputs"
                echo "  --help                  Show this help"
                exit 0
                ;;
            *)
                error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    log "Starting crash analysis..."
    
    # Check tools
    check_gdb
    check_addr2line
    
    if [ -n "$harness" ]; then
        # Analyze specific harness
        analyze_harness_crashes "$harness"
        
        if [ "$minimize" = true ]; then
            minimize_crashes "$harness"
        fi
    else
        # Analyze all harnesses
        local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
        
        for harness in "${harnesses[@]}"; do
            analyze_harness_crashes "$harness"
            
            if [ "$minimize" = true ]; then
                minimize_crashes "$harness"
            fi
        done
    fi
    
    # Generate summary
    generate_crash_summary
    
    success "Crash analysis completed!"
}

# Run main function
main "$@"
