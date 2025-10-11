#!/bin/bash
# ============================================================================
# CRRSS Git Hooks Installation Script
# Install optional pre-commit hooks for automated CRRSS checks
# Phase 1B Stage 4: Build System Integration & Documentation
# ============================================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
GIT_HOOKS_DIR="$REPO_ROOT/.git/hooks"
CRRSS_HOOK="$SCRIPT_DIR/pre-commit-crrss"
PRE_COMMIT_HOOK="$GIT_HOOKS_DIR/pre-commit"

# Print header
echo -e "${BLUE}========================================"
echo -e "CRRSS Git Hooks Installation"
echo -e "========================================${NC}"
echo ""

# Check if .git directory exists
if [ ! -d "$REPO_ROOT/.git" ]; then
    echo -e "${RED}Error:${NC} Not in a git repository"
    echo "Please run this script from the repository root"
    exit 1
fi

# Check if CRRSS hook exists
if [ ! -f "$CRRSS_HOOK" ]; then
    echo -e "${RED}Error:${NC} CRRSS pre-commit hook not found at $CRRSS_HOOK"
    exit 1
fi

# Create hooks directory if it doesn't exist
mkdir -p "$GIT_HOOKS_DIR"

# Function to install hook
install_hook() {
    echo -e "${BLUE}[Install]${NC} Installing CRRSS pre-commit hook..."
    
    # Make CRRSS hook executable
    chmod +x "$CRRSS_HOOK"
    
    # Check if pre-commit hook already exists
    if [ -f "$PRE_COMMIT_HOOK" ]; then
        echo -e "${YELLOW}[Warning]${NC} Pre-commit hook already exists"
        
        # Check if it's already calling CRRSS
        if grep -q "pre-commit-crrss" "$PRE_COMMIT_HOOK"; then
            echo -e "${GREEN}[OK]${NC} CRRSS hook already integrated"
            return 0
        fi
        
        # Ask user what to do
        echo ""
        echo "Options:"
        echo "  1) Append CRRSS hook to existing pre-commit"
        echo "  2) Backup existing and create new pre-commit"
        echo "  3) Skip installation"
        echo ""
        read -p "Choose option [1-3]: " option
        
        case $option in
            1)
                echo -e "${BLUE}[Install]${NC} Appending CRRSS hook..."
                echo "" >> "$PRE_COMMIT_HOOK"
                echo "# CRRSS Pre-Commit Hook" >> "$PRE_COMMIT_HOOK"
                echo "$(cat $CRRSS_HOOK)" >> "$PRE_COMMIT_HOOK"
                ;;
            2)
                BACKUP="$PRE_COMMIT_HOOK.backup.$(date +%Y%m%d_%H%M%S)"
                echo -e "${BLUE}[Backup]${NC} Backing up to $BACKUP"
                cp "$PRE_COMMIT_HOOK" "$BACKUP"
                cp "$CRRSS_HOOK" "$PRE_COMMIT_HOOK"
                chmod +x "$PRE_COMMIT_HOOK"
                ;;
            3)
                echo -e "${YELLOW}[Skip]${NC} Installation skipped"
                return 1
                ;;
            *)
                echo -e "${RED}[Error]${NC} Invalid option"
                return 1
                ;;
        esac
    else
        # No existing hook, just copy
        echo -e "${BLUE}[Install]${NC} Creating new pre-commit hook"
        cp "$CRRSS_HOOK" "$PRE_COMMIT_HOOK"
        chmod +x "$PRE_COMMIT_HOOK"
    fi
    
    echo -e "${GREEN}[Success]${NC} CRRSS pre-commit hook installed"
    return 0
}

# Function to uninstall hook
uninstall_hook() {
    echo -e "${BLUE}[Uninstall]${NC} Removing CRRSS pre-commit hook..."
    
    if [ ! -f "$PRE_COMMIT_HOOK" ]; then
        echo -e "${YELLOW}[Warning]${NC} No pre-commit hook found"
        return 1
    fi
    
    # Check if it's the CRRSS hook
    if grep -q "CRRSS Pre-Commit Hook" "$PRE_COMMIT_HOOK"; then
        # Look for backup
        LATEST_BACKUP=$(ls -t "$PRE_COMMIT_HOOK.backup."* 2>/dev/null | head -1 || true)
        
        if [ -n "$LATEST_BACKUP" ]; then
            echo -e "${BLUE}[Restore]${NC} Restoring from backup: $LATEST_BACKUP"
            cp "$LATEST_BACKUP" "$PRE_COMMIT_HOOK"
            chmod +x "$PRE_COMMIT_HOOK"
        else
            echo -e "${BLUE}[Remove]${NC} Removing pre-commit hook (no backup found)"
            rm -f "$PRE_COMMIT_HOOK"
        fi
        
        echo -e "${GREEN}[Success]${NC} CRRSS pre-commit hook uninstalled"
    else
        echo -e "${YELLOW}[Warning]${NC} Pre-commit hook exists but is not CRRSS hook"
        echo "Manual removal required"
        return 1
    fi
    
    return 0
}

# Function to show status
show_status() {
    echo -e "${BLUE}[Status]${NC} CRRSS Hook Status"
    echo ""
    
    if [ -f "$PRE_COMMIT_HOOK" ]; then
        if grep -q "CRRSS Pre-Commit Hook" "$PRE_COMMIT_HOOK"; then
            echo -e "  Status:     ${GREEN}Installed${NC}"
            echo -e "  Location:   $PRE_COMMIT_HOOK"
            echo -e "  Executable: $([ -x "$PRE_COMMIT_HOOK" ] && echo "${GREEN}Yes${NC}" || echo "${RED}No${NC}")"
        else
            echo -e "  Status:     ${YELLOW}Other hook installed${NC}"
            echo -e "  Location:   $PRE_COMMIT_HOOK"
        fi
    else
        echo -e "  Status:     ${RED}Not installed${NC}"
    fi
    
    echo ""
    echo "Configuration (environment variables):"
    echo "  CRRSS_ENABLED=${CRRSS_ENABLED:-1}     (0=disabled, 1=enabled)"
    echo "  CRRSS_STRICT=${CRRSS_STRICT:-0}      (0=warnings, 1=strict)"
    echo "  CRRSS_MAX_ISSUES=${CRRSS_MAX_ISSUES:-10}  (max issues to display)"
}

# Function to test hook
test_hook() {
    echo -e "${BLUE}[Test]${NC} Testing CRRSS pre-commit hook..."
    echo ""
    
    if [ ! -f "$PRE_COMMIT_HOOK" ]; then
        echo -e "${RED}[Error]${NC} Pre-commit hook not installed"
        return 1
    fi
    
    # Create temporary test
    echo -e "${BLUE}[Test]${NC} Running hook..."
    if bash "$PRE_COMMIT_HOOK"; then
        echo -e "${GREEN}[Success]${NC} Hook executed successfully"
    else
        echo -e "${RED}[Error]${NC} Hook execution failed"
        return 1
    fi
}

# Show help
show_help() {
    echo "Usage: $0 [command]"
    echo ""
    echo "Commands:"
    echo "  install     Install CRRSS pre-commit hook"
    echo "  uninstall   Uninstall CRRSS pre-commit hook"
    echo "  status      Show hook installation status"
    echo "  test        Test the pre-commit hook"
    echo "  help        Show this help message"
    echo ""
    echo "Configuration:"
    echo "  Set environment variables to configure the hook:"
    echo "    export CRRSS_ENABLED=0      # Disable CRRSS checks"
    echo "    export CRRSS_STRICT=1       # Enable strict mode (block on critical issues)"
    echo "    export CRRSS_MAX_ISSUES=20  # Show more issues in output"
    echo ""
    echo "Bypass hook for a single commit:"
    echo "  CRRSS_ENABLED=0 git commit -m \"message\""
    echo "  git commit --no-verify -m \"message\""
    echo ""
}

# Main logic
case "${1:-install}" in
    install)
        if install_hook; then
            echo ""
            show_status
            echo ""
            echo -e "${GREEN}Installation complete!${NC}"
            echo ""
            echo "The hook will now run on every commit."
            echo "To disable temporarily: CRRSS_ENABLED=0 git commit"
            echo "To enable strict mode: export CRRSS_STRICT=1"
        fi
        ;;
    uninstall)
        if uninstall_hook; then
            echo ""
            echo -e "${GREEN}Uninstallation complete!${NC}"
        fi
        ;;
    status)
        show_status
        ;;
    test)
        test_hook
        ;;
    help|--help|-h)
        show_help
        ;;
    *)
        echo -e "${RED}Error:${NC} Unknown command: $1"
        echo ""
        show_help
        exit 1
        ;;
esac

echo ""
echo -e "${BLUE}========================================${NC}"
