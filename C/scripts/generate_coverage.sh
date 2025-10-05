
#!/bin/bash

# BDI Kernel Coverage Generation Script
# ====================================

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
COVERAGE_DIR="$PROJECT_ROOT/coverage_reports"
BUILD_DIR="$PROJECT_ROOT/build"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Create coverage directories
create_directories() {
    log_info "Creating coverage directories..."
    mkdir -p "$COVERAGE_DIR"/{lcov,html,xml,json,text}
    mkdir -p "$BUILD_DIR"
}

# Clean previous coverage data
clean_coverage_data() {
    log_info "Cleaning previous coverage data..."
    find "$PROJECT_ROOT" -name "*.gcda" -delete 2>/dev/null || true
    find "$PROJECT_ROOT" -name "*.gcno" -delete 2>/dev/null || true
    find "$PROJECT_ROOT" -name "*.gcov" -delete 2>/dev/null || true
    rm -rf "$COVERAGE_DIR"/*
}

# Build with coverage flags
build_with_coverage() {
    log_info "Building project with coverage instrumentation..."
    cd "$PROJECT_ROOT"
    
    # Add coverage flags to make
    export CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage -fPIC -O0 -g"
    export LDFLAGS="$LDFLAGS -lgcov --coverage"
    
    # Build the project
    make clean
    make all vm_lib test_phase7 test_phase8_graph
    
    log_success "Build completed with coverage instrumentation"
}

# Run tests to generate coverage data
run_tests() {
    log_info "Running tests to generate coverage data..."
    cd "$PROJECT_ROOT"
    
    # Run unit tests
    if [ -f "tests/framework/test_runner" ]; then
        log_info "Running comprehensive test suite..."
        ./tests/framework/test_runner || log_warning "Some tests failed, but continuing with coverage analysis"
    fi
    
    # Run existing tests
    if make test >/dev/null 2>&1; then
        log_info "Running existing test suite..."
        make test || log_warning "Some existing tests failed"
    fi
    
    # Run VM tests
    if make test_vm_complete >/dev/null 2>&1; then
        log_info "Running VM test suite..."
        make test_vm_complete || log_warning "Some VM tests failed"
    fi
    
    log_success "Test execution completed"
}

# Generate LCOV coverage data
generate_lcov_data() {
    log_info "Generating LCOV coverage data..."
    cd "$PROJECT_ROOT"
    
    # Capture coverage data
    lcov --capture \
         --directory . \
         --output-file "$COVERAGE_DIR/lcov/coverage.info" \
         --rc lcov_branch_coverage=1
    
    # Remove external files and test files from coverage
    lcov --remove "$COVERAGE_DIR/lcov/coverage.info" \
         '/usr/*' \
         '*/tests/*' \
         '*/test_*' \
         '*_test.c' \
         '*_benchmark.c' \
         --output-file "$COVERAGE_DIR/lcov/coverage_filtered.info" \
         --rc lcov_branch_coverage=1
    
    log_success "LCOV data generated"
}

# Generate HTML coverage report
generate_html_report() {
    log_info "Generating HTML coverage report..."
    
    genhtml "$COVERAGE_DIR/lcov/coverage_filtered.info" \
            --output-directory "$COVERAGE_DIR/html" \
            --title "BDI Kernel Coverage Report" \
            --show-details \
            --highlight \
            --legend \
            --branch-coverage \
            --function-coverage \
            --rc genhtml_branch_coverage=1
    
    log_success "HTML report generated at $COVERAGE_DIR/html/index.html"
}

# Generate XML coverage report (Cobertura format)
generate_xml_report() {
    log_info "Generating XML coverage report..."
    
    if command -v gcovr >/dev/null 2>&1; then
        gcovr --root "$PROJECT_ROOT" \
              --exclude '.*test.*' \
              --exclude '.*benchmark.*' \
              --xml-pretty \
              --output "$COVERAGE_DIR/xml/coverage.xml"
        
        log_success "XML report generated at $COVERAGE_DIR/xml/coverage.xml"
    else
        log_warning "gcovr not found, skipping XML report generation"
    fi
}

# Generate JSON coverage report
generate_json_report() {
    log_info "Generating JSON coverage report..."
    
    if command -v gcovr >/dev/null 2>&1; then
        gcovr --root "$PROJECT_ROOT" \
              --exclude '.*test.*' \
              --exclude '.*benchmark.*' \
              --json-pretty \
              --output "$COVERAGE_DIR/json/coverage.json"
        
        log_success "JSON report generated at $COVERAGE_DIR/json/coverage.json"
    else
        log_warning "gcovr not found, skipping JSON report generation"
    fi
}

# Generate text coverage summary
generate_text_summary() {
    log_info "Generating text coverage summary..."
    
    # LCOV summary
    lcov --summary "$COVERAGE_DIR/lcov/coverage_filtered.info" \
         --rc lcov_branch_coverage=1 > "$COVERAGE_DIR/text/lcov_summary.txt"
    
    # gcovr summary if available
    if command -v gcovr >/dev/null 2>&1; then
        gcovr --root "$PROJECT_ROOT" \
              --exclude '.*test.*' \
              --exclude '.*benchmark.*' > "$COVERAGE_DIR/text/gcovr_summary.txt"
    fi
    
    log_success "Text summaries generated"
}

# Check coverage thresholds
check_coverage_thresholds() {
    log_info "Checking coverage thresholds..."
    
    if [ -f "$COVERAGE_DIR/text/lcov_summary.txt" ]; then
        # Extract coverage percentages
        line_coverage=$(grep "lines" "$COVERAGE_DIR/text/lcov_summary.txt" | grep -o '[0-9.]*%' | head -1 | sed 's/%//')
        function_coverage=$(grep "functions" "$COVERAGE_DIR/text/lcov_summary.txt" | grep -o '[0-9.]*%' | head -1 | sed 's/%//')
        branch_coverage=$(grep "branches" "$COVERAGE_DIR/text/lcov_summary.txt" | grep -o '[0-9.]*%' | head -1 | sed 's/%//')
        
        # Set thresholds
        line_threshold=90
        function_threshold=95
        branch_threshold=85
        
        # Check thresholds
        coverage_passed=true
        
        if (( $(echo "$line_coverage < $line_threshold" | bc -l) )); then
            log_error "Line coverage ($line_coverage%) below threshold ($line_threshold%)"
            coverage_passed=false
        else
            log_success "Line coverage: $line_coverage% (threshold: $line_threshold%)"
        fi
        
        if (( $(echo "$function_coverage < $function_threshold" | bc -l) )); then
            log_error "Function coverage ($function_coverage%) below threshold ($function_threshold%)"
            coverage_passed=false
        else
            log_success "Function coverage: $function_coverage% (threshold: $function_threshold%)"
        fi
        
        if (( $(echo "$branch_coverage < $branch_threshold" | bc -l) )); then
            log_error "Branch coverage ($branch_coverage%) below threshold ($branch_threshold%)"
            coverage_passed=false
        else
            log_success "Branch coverage: $branch_coverage% (threshold: $branch_threshold%)"
        fi
        
        if [ "$coverage_passed" = true ]; then
            log_success "All coverage thresholds met!"
            return 0
        else
            log_error "Coverage thresholds not met"
            return 1
        fi
    else
        log_warning "Could not find coverage summary for threshold checking"
        return 1
    fi
}

# Generate coverage badge
generate_coverage_badge() {
    log_info "Generating coverage badge..."
    
    if [ -f "$COVERAGE_DIR/text/lcov_summary.txt" ]; then
        line_coverage=$(grep "lines" "$COVERAGE_DIR/text/lcov_summary.txt" | grep -o '[0-9.]*%' | head -1)
        
        # Create a simple badge file
        cat > "$COVERAGE_DIR/coverage_badge.svg" << EOF
<svg xmlns="http://www.w3.org/2000/svg" width="104" height="20">
  <linearGradient id="b" x2="0" y2="100%">
    <stop offset="0" stop-color="#bbb" stop-opacity=".1"/>
    <stop offset="1" stop-opacity=".1"/>
  </linearGradient>
  <mask id="a">
    <rect width="104" height="20" rx="3" fill="#fff"/>
  </mask>
  <g mask="url(#a)">
    <path fill="#555" d="M0 0h63v20H0z"/>
    <path fill="#4c1" d="M63 0h41v20H63z"/>
    <path fill="url(#b)" d="M0 0h104v20H0z"/>
  </g>
  <g fill="#fff" text-anchor="middle" font-family="DejaVu Sans,Verdana,Geneva,sans-serif" font-size="11">
    <text x="31.5" y="15" fill="#010101" fill-opacity=".3">coverage</text>
    <text x="31.5" y="14">coverage</text>
    <text x="82.5" y="15" fill="#010101" fill-opacity=".3">$line_coverage</text>
    <text x="82.5" y="14">$line_coverage</text>
  </g>
</svg>
EOF
        
        log_success "Coverage badge generated at $COVERAGE_DIR/coverage_badge.svg"
    fi
}

# Main execution
main() {
    log_info "Starting BDI Kernel coverage analysis..."
    
    create_directories
    clean_coverage_data
    build_with_coverage
    run_tests
    generate_lcov_data
    generate_html_report
    generate_xml_report
    generate_json_report
    generate_text_summary
    generate_coverage_badge
    
    log_info "Coverage analysis completed!"
    log_info "Reports available in: $COVERAGE_DIR"
    log_info "HTML Report: $COVERAGE_DIR/html/index.html"
    
    # Check thresholds and exit with appropriate code
    if check_coverage_thresholds; then
        log_success "Coverage analysis passed all quality gates!"
        exit 0
    else
        log_error "Coverage analysis failed quality gates"
        exit 1
    fi
}

# Run main function
main "$@"
