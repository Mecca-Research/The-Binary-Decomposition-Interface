# CRRSS Git Hooks

Optional pre-commit hooks for automated CRRSS code quality checks.

## Overview

The CRRSS pre-commit hook automatically analyzes staged C/H files before each commit, helping catch potential issues early in the development process.

## Installation

### Quick Install

```bash
# From repository root
./.git-hooks/install-hooks.sh install
```

### Manual Install

```bash
# Copy the hook to .git/hooks/
cp .git-hooks/pre-commit-crrss .git/hooks/pre-commit
chmod +x .git/hooks/pre-commit
```

## Usage

Once installed, the hook automatically runs on every commit:

```bash
git add some_file.c
git commit -m "Fix memory leak"
# Hook runs automatically
```

### Bypass Hook (Single Commit)

```bash
# Option 1: Disable CRRSS
CRRSS_ENABLED=0 git commit -m "message"

# Option 2: Skip all hooks
git commit --no-verify -m "message"
```

### Bypass Hook (Permanently)

```bash
# Disable in your shell session
export CRRSS_ENABLED=0

# Or add to ~/.bashrc or ~/.zshrc
```

## Configuration

Configure the hook behavior with environment variables:

### CRRSS_ENABLED

Enable/disable the hook:

```bash
export CRRSS_ENABLED=1  # Enabled (default)
export CRRSS_ENABLED=0  # Disabled
```

### CRRSS_STRICT

Control blocking behavior:

```bash
export CRRSS_STRICT=0   # Warning mode - show issues but allow commit (default)
export CRRSS_STRICT=1   # Strict mode - block commit on critical issues (P0/P1)
```

### CRRSS_MAX_ISSUES

Limit displayed issues:

```bash
export CRRSS_MAX_ISSUES=10   # Show max 10 issues (default)
export CRRSS_MAX_ISSUES=50   # Show max 50 issues
```

## Examples

### Development Workflow

```bash
# Regular development (warnings only)
git commit -m "Add feature"

# Enable strict mode for important commits
CRRSS_STRICT=1 git commit -m "Critical fix"

# Quick commit without checks
CRRSS_ENABLED=0 git commit -m "WIP: work in progress"
```

### Team Configuration

Add to `.bashrc` or `.zshrc`:

```bash
# Strict mode for all commits
export CRRSS_STRICT=1

# Show more issues
export CRRSS_MAX_ISSUES=20
```

## Hook Behavior

### Warning Mode (Default)

- **CRRSS_STRICT=0**
- Analyzes all staged C/H files
- Reports issues found
- **Always allows commit**
- Useful for continuous development

### Strict Mode

- **CRRSS_STRICT=1**
- Analyzes all staged C/H files
- Reports issues found
- **Blocks commit if critical issues found** (P0/P1 priority)
- Useful for production commits

## Management

### Check Status

```bash
./.git-hooks/install-hooks.sh status
```

### Test Hook

```bash
./.git-hooks/install-hooks.sh test
```

### Uninstall

```bash
./.git-hooks/install-hooks.sh uninstall
```

## Output Example

```
========================================
CRRSS Pre-Commit Hook
========================================
[CRRSS] Analyzing 2 staged C/H file(s)...
[CRRSS] Checking: moduler_kernel/memory.c
[CRRSS] Found 3 issue(s) in moduler_kernel/memory.c (1 critical)
Issues:
  Issue: Potential memory leak at line 145
  Issue: NULL pointer dereference at line 203
  Issue: Use-after-free at line 267
[CRRSS] Checking: moduler_kernel/scheduler.c
[CRRSS] ✓ moduler_kernel/scheduler.c passed all checks
========================================
CRRSS Pre-Commit Summary
========================================
Files analyzed:     2
Total issues:       3
Critical issues:    1

[CRRSS] Warning: 3 issue(s) found
[CRRSS] Consider fixing these issues before committing
[CRRSS] To enable strict mode: CRRSS_STRICT=1
[CRRSS] Pre-commit checks complete
```

## Troubleshooting

### Hook Not Running

```bash
# Check if hook is installed
ls -la .git/hooks/pre-commit

# Check if hook is executable
chmod +x .git/hooks/pre-commit

# Check status
./.git-hooks/install-hooks.sh status
```

### CRRSS Tool Not Found

```bash
# Build CRRSS tool
make -C tools/crrss tool

# Or build everything
make crrss
```

### Hook Fails to Build CRRSS

```bash
# Manually build CRRSS first
cd tools/crrss
make clean
make all
cd ../..

# Then test hook
./.git-hooks/install-hooks.sh test
```

### Permission Denied

```bash
# Make scripts executable
chmod +x .git-hooks/pre-commit-crrss
chmod +x .git-hooks/install-hooks.sh
chmod +x .git/hooks/pre-commit
```

## CI/CD Integration

The hook automatically detects CI environments and adjusts behavior:

```bash
# In CI (detected automatically)
CI=true git commit -m "message"
```

## Best Practices

1. **Development Phase**: Use warning mode (default)
   ```bash
   export CRRSS_STRICT=0
   ```

2. **Pre-Release**: Enable strict mode
   ```bash
   export CRRSS_STRICT=1
   ```

3. **Quick Fixes**: Bypass when needed
   ```bash
   CRRSS_ENABLED=0 git commit -m "Quick fix"
   ```

4. **Team Standard**: Add configuration to shell profile
   ```bash
   # In ~/.bashrc or ~/.zshrc
   export CRRSS_STRICT=1
   export CRRSS_MAX_ISSUES=20
   ```

## Integration with Other Hooks

If you have existing pre-commit hooks:

```bash
# Option 1: Append to existing hook
cat .git-hooks/pre-commit-crrss >> .git/hooks/pre-commit

# Option 2: Call from existing hook
echo ".git-hooks/pre-commit-crrss" >> .git/hooks/pre-commit
```

## Performance

- **Small commits** (1-5 files): < 5 seconds
- **Medium commits** (5-20 files): 5-15 seconds
- **Large commits** (20+ files): 15-30 seconds

Tip: Analyze files individually during development to save time:

```bash
# Analyze before staging
make crrss-analyze FILE=moduler_kernel/memory.c
```

## Support

For issues or questions:
- See main CRRSS documentation: `tools/crrss/docs/USAGE.md`
- Run: `make crrss-help`
- Check hook status: `./.git-hooks/install-hooks.sh status`

## Uninstallation

```bash
# Remove hook
./.git-hooks/install-hooks.sh uninstall

# Or manually
rm .git/hooks/pre-commit
```

## Version

CRRSS Git Hooks - Version 1.0.0
Phase 1B Stage 4: Build System Integration & Documentation
