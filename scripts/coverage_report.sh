
#!/bin/bash

# Coverage Analysis and Reporting Script for BDI Kernel Fuzzing
# 
# This script analyzes fuzzing coverage, generates reports,
# and tracks coverage improvements over time.

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
FUZZING_DIR="$PROJECT_ROOT/C/fuzzing"
BUILD_DIR="$PROJECT_ROOT/build/fuzzing"
COVERAGE_DIR="$FUZZING_DIR/coverage"

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

# Check if gcov/lcov tools are available
check_coverage_tools() {
    local missing_tools=()
    
    if ! command -v gcov &> /dev/null; then
        missing_tools+=("gcov")
    fi
    
    if ! command -v lcov &> /dev/null; then
        missing_tools+=("lcov")
    fi
    
    if ! command -v genhtml &> /dev/null; then
        missing_tools+=("genhtml")
    fi
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        warning "Missing coverage tools: ${missing_tools[*]}"
        warning "Install lcov package for full coverage analysis"
        return 1
    fi
    
    return 0
}

# Build coverage-enabled binaries
build_coverage_binaries() {
    log "Building coverage-enabled binaries..."
    
    cd "$PROJECT_ROOT"
    
    # Build with coverage flags
    if make fuzz-coverage; then
        success "Coverage-enabled binaries built"
    else
        error "Failed to build coverage-enabled binaries"
        return 1
    fi
}

# Run harness with coverage collection
run_coverage_harness() {
    local harness=$1
    local duration=${2:-60}  # Default 1 minute
    
    log "Running coverage analysis for harness: $harness"
    
    local binary="$BUILD_DIR/coverage_${harness}"
    local corpus_dir="$FUZZING_DIR/corpus/${harness}_seeds"
    local coverage_output="$COVERAGE_DIR/$harness"
    
    if [ ! -f "$binary" ]; then
        error "Coverage binary not found: $binary"
        return 1
    fi
    
    mkdir -p "$coverage_output"
    
    # Reset coverage counters
    if command -v lcov &> /dev/null; then
        lcov --directory "$PROJECT_ROOT" --zerocounters 2>/dev/null || true
    fi
    
    # Run harness with corpus
    if [ -d "$corpus_dir" ]; then
        log "Running $harness with corpus for coverage analysis..."
        
        for input_file in "$corpus_dir"/*; do
            if [ -f "$input_file" ]; then
                timeout 5 "$binary" "$input_file" 2>/dev/null || true
            fi
        done
    fi
    
    # Run with LibFuzzer for additional coverage
    log "Running LibFuzzer for additional coverage..."
    timeout "$duration" "$binary" "$corpus_dir" -runs=10000 2>/dev/null || true
    
    # Collect coverage data
    if command -v lcov &> /dev/null; then
        log "Collecting coverage data for $harness..."
        
        lcov --directory "$PROJECT_ROOT" \
             --capture \
             --output-file "$coverage_output/coverage.info" \
             2>/dev/null || true
        
        # Filter out system headers and test files
        lcov --remove "$coverage_output/coverage.info" \
             '/usr/*' \
             '*/test/*' \
             '*/tests/*' \
             --output-file "$coverage_output/coverage_filtered.info" \
             2>/dev/null || true
        
        # Generate HTML report
        genhtml "$coverage_output/coverage_filtered.info" \
                --output-directory "$coverage_output/html" \
                --title "BDI Kernel $harness Coverage" \
                --show-details \
                --legend \
                2>/dev/null || true
        
        success "Coverage report generated for $harness"
    fi
}

# Analyze coverage data
analyze_coverage() {
    local harness=$1
    
    log "Analyzing coverage for harness: $harness"
    
    local coverage_output="$COVERAGE_DIR/$harness"
    local coverage_file="$coverage_output/coverage_filtered.info"
    
    if [ ! -f "$coverage_file" ]; then
        warning "Coverage file not found: $coverage_file"
        return 1
    fi
    
    local analysis_file="$coverage_output/analysis.txt"
    
    {
        echo "Coverage Analysis for $harness"
        echo "Generated: $(date)"
        echo "=============================="
        echo
        
        # Extract coverage statistics
        if command -v lcov &> /dev/null; then
            echo "Coverage Summary:"
            echo "----------------"
            lcov --summary "$coverage_file" 2>/dev/null || echo "Failed to generate summary"
            echo
        fi
        
        # Function coverage
        echo "Function Coverage:"
        echo "-----------------"
        if command -v lcov &> /dev/null; then
            lcov --list "$coverage_file" 2>/dev/null | grep -E "^\s*[0-9]" | head -20 || echo "No function data available"
        fi
        echo
        
        # Uncovered lines (potential targets for improvement)
        echo "Uncovered Areas (sample):"
        echo "-------------------------"
        if [ -f "$coverage_output/html/index.html" ]; then
            echo "Detailed HTML report available at: $coverage_output/html/index.html"
        fi
        echo
        
    } > "$analysis_file"
    
    success "Coverage analysis saved to: $analysis_file"
}

# Compare coverage between runs
compare_coverage() {
    local harness=$1
    local baseline_file=$2
    local current_file=$3
    
    log "Comparing coverage for harness: $harness"
    
    if [ ! -f "$baseline_file" ] || [ ! -f "$current_file" ]; then
        warning "Coverage files not found for comparison"
        return 1
    fi
    
    local comparison_file="$COVERAGE_DIR/$harness/comparison.txt"
    
    {
        echo "Coverage Comparison for $harness"
        echo "Generated: $(date)"
        echo "==============================="
        echo
        
        echo "Baseline: $baseline_file"
        echo "Current:  $current_file"
        echo
        
        # Use lcov to compare if available
        if command -v lcov &> /dev/null; then
            echo "Coverage Difference:"
            echo "-------------------"
            
            # This is a simplified comparison - lcov doesn't have direct diff
            echo "Baseline summary:"
            lcov --summary "$baseline_file" 2>/dev/null || echo "Failed to read baseline"
            echo
            echo "Current summary:"
            lcov --summary "$current_file" 2>/dev/null || echo "Failed to read current"
            echo
        fi
        
    } > "$comparison_file"
    
    success "Coverage comparison saved to: $comparison_file"
}

# Generate comprehensive coverage report
generate_comprehensive_report() {
    log "Generating comprehensive coverage report..."
    
    local report_file="$COVERAGE_DIR/comprehensive_report.html"
    local summary_file="$COVERAGE_DIR/coverage_summary.txt"
    
    # Generate text summary
    {
        echo "BDI Kernel Fuzzing Coverage Report"
        echo "Generated: $(date)"
        echo "=================================="
        echo
        
        local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
        local total_lines=0
        local total_covered=0
        
        for harness in "${harnesses[@]}"; do
            echo "[$harness]"
            
            local coverage_file="$COVERAGE_DIR/$harness/coverage_filtered.info"
            if [ -f "$coverage_file" ]; then
                if command -v lcov &> /dev/null; then
                    local summary=$(lcov --summary "$coverage_file" 2>/dev/null | grep "lines" | head -1)
                    echo "  $summary"
                    
                    # Extract numbers for totals (simplified parsing)
                    local covered=$(echo "$summary" | grep -o "[0-9]*" | head -1 || echo "0")
                    local total=$(echo "$summary" | grep -o "[0-9]*" | tail -1 || echo "0")
                    
                    total_covered=$((total_covered + covered))
                    total_lines=$((total_lines + total))
                else
                    echo "  Coverage data available"
                fi
                
                # Check for HTML report
                if [ -f "$COVERAGE_DIR/$harness/html/index.html" ]; then
                    echo "  HTML report: $COVERAGE_DIR/$harness/html/index.html"
                fi
            else
                echo "  No coverage data available"
            fi
            
            echo
        done
        
        echo "Overall Summary:"
        echo "---------------"
        if [ $total_lines -gt 0 ]; then
            local percentage=$((total_covered * 100 / total_lines))
            echo "Total lines covered: $total_covered / $total_lines ($percentage%)"
        else
            echo "No coverage data available"
        fi
        
        echo "Report generated: $(date)"
        
    } > "$summary_file"
    
    # Generate HTML index
    {
        cat << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>BDI Kernel Fuzzing Coverage Report</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .header { background-color: #f0f0f0; padding: 20px; border-radius: 5px; }
        .harness { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }
        .harness h3 { margin-top: 0; color: #333; }
        .coverage-link { display: inline-block; margin: 5px 10px 5px 0; padding: 8px 15px; background-color: #007cba; color: white; text-decoration: none; border-radius: 3px; }
        .coverage-link:hover { background-color: #005a87; }
        .summary { background-color: #e8f4f8; padding: 15px; border-radius: 5px; margin-top: 20px; }
    </style>
</head>
<body>
    <div class="header">
        <h1>BDI Kernel Fuzzing Coverage Report</h1>
        <p>Generated: $(date)</p>
    </div>
EOF
        
        local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
        
        for harness in "${harnesses[@]}"; do
            echo "    <div class=\"harness\">"
            echo "        <h3>$harness</h3>"
            
            if [ -f "$COVERAGE_DIR/$harness/html/index.html" ]; then
                echo "        <a href=\"$harness/html/index.html\" class=\"coverage-link\">View Coverage Report</a>"
            fi
            
            if [ -f "$COVERAGE_DIR/$harness/analysis.txt" ]; then
                echo "        <a href=\"$harness/analysis.txt\" class=\"coverage-link\">View Analysis</a>"
            fi
            
            echo "    </div>"
        done
        
        cat << 'EOF'
    <div class="summary">
        <h3>Summary</h3>
        <p>This report provides coverage analysis for all BDI Kernel fuzzing harnesses.</p>
        <p>Click on the links above to view detailed coverage reports for each harness.</p>
    </div>
</body>
</html>
EOF
        
    } > "$report_file"
    
    success "Comprehensive coverage report generated:"
    success "  Text summary: $summary_file"
    success "  HTML index: $report_file"
    
    # Display summary
    cat "$summary_file"
}

# Track coverage over time
track_coverage_history() {
    log "Tracking coverage history..."
    
    local history_file="$COVERAGE_DIR/coverage_history.csv"
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    
    # Create header if file doesn't exist
    if [ ! -f "$history_file" ]; then
        echo "timestamp,harness,lines_covered,total_lines,percentage" > "$history_file"
    fi
    
    local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
    
    for harness in "${harnesses[@]}"; do
        local coverage_file="$COVERAGE_DIR/$harness/coverage_filtered.info"
        
        if [ -f "$coverage_file" ] && command -v lcov &> /dev/null; then
            local summary=$(lcov --summary "$coverage_file" 2>/dev/null | grep "lines" | head -1)
            
            if [ -n "$summary" ]; then
                # Parse coverage numbers (simplified)
                local covered=$(echo "$summary" | grep -o "[0-9]*" | head -1 || echo "0")
                local total=$(echo "$summary" | grep -o "[0-9]*" | tail -1 || echo "0")
                local percentage=0
                
                if [ $total -gt 0 ]; then
                    percentage=$((covered * 100 / total))
                fi
                
                echo "$timestamp,$harness,$covered,$total,$percentage" >> "$history_file"
            fi
        fi
    done
    
    success "Coverage history updated: $history_file"
}

# Main execution
main() {
    local action="analyze"
    local harness=""
    local duration=300
    
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
            -d|--duration)
                duration="$2"
                shift 2
                ;;
            --help)
                echo "Usage: $0 [OPTIONS]"
                echo "Options:"
                echo "  -a, --action ACTION     Action: analyze, build, run, report, history (default: analyze)"
                echo "  -h, --harness HARNESS   Target specific harness"
                echo "  -d, --duration SECONDS  Duration for coverage run (default: 300)"
                echo "  --help                  Show this help"
                echo
                echo "Actions:"
                echo "  build       Build coverage-enabled binaries"
                echo "  run         Run coverage analysis"
                echo "  analyze     Analyze existing coverage data"
                echo "  report      Generate comprehensive report"
                echo "  history     Track coverage history"
                exit 0
                ;;
            *)
                error "Unknown option: $1"
                exit 1
                ;;
        esac
    done
    
    log "Starting coverage analysis..."
    log "Action: $action"
    
    # Check tools
    check_coverage_tools
    
    case $action in
        "build")
            build_coverage_binaries
            ;;
            
        "run")
            build_coverage_binaries
            
            if [ -n "$harness" ]; then
                run_coverage_harness "$harness" "$duration"
                analyze_coverage "$harness"
            else
                local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
                for h in "${harnesses[@]}"; do
                    run_coverage_harness "$h" "$duration"
                    analyze_coverage "$h"
                done
            fi
            ;;
            
        "analyze")
            if [ -n "$harness" ]; then
                analyze_coverage "$harness"
            else
                local harnesses=("vm_bytecode" "jit_compiler" "graph_execution" "memory_management" "bytecode_parser" "value_system")
                for h in "${harnesses[@]}"; do
                    analyze_coverage "$h"
                done
            fi
            ;;
            
        "report")
            generate_comprehensive_report
            ;;
            
        "history")
            track_coverage_history
            ;;
            
        *)
            error "Unknown action: $action"
            exit 1
            ;;
    esac
    
    success "Coverage analysis completed!"
}

# Run main function
main "$@"
