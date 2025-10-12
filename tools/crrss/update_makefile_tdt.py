#!/usr/bin/env python3
"""
Update Makefile with TDT support while preserving tabs.
This script carefully handles Makefile syntax requirements:
- Recipe lines MUST start with TAB (ASCII 9), not spaces
- Variable assignments and comments use regular indentation
"""

import sys
import re

def main():
    makefile_path = "Makefile"
    
    # Read the current Makefile
    with open(makefile_path, 'r') as f:
        lines = f.readlines()
    
    # Track if we've made changes
    modified = False
    new_lines = []
    
    for i, line in enumerate(lines):
        # 1. Add TDT to include paths (after line 21)
        if i == 20 and '-Itdt' not in line:
            # Check if next line needs TDT added
            if '-Itdt' not in lines[i]:
                new_lines.append(line.rstrip() + ' -Itdt\n')
                modified = True
                continue
        
        # 2. Add TDT sources (after STP sources at line 84)
        if 'STP_SRCS := stp/stp.c' in line and i < len(lines) - 1:
            new_lines.append(line)
            # Check if TDT line doesn't exist yet
            if i + 1 < len(lines) and 'TDT_SRCS' not in lines[i + 1]:
                new_lines.append('\n# TDT sources\n')
                new_lines.append('TDT_SRCS := tdt/tdt.c tdt/tdt_generator.c tdt/tdt_coverage.c tdt/tdt_templates.c tdt/tdt_integration.c\n')
                modified = True
            continue
        
        # 3. Update LIB_SRCS to include TDT (line 93)
        if 'LIB_SRCS := $(COMMON_SRCS) $(BPME_SRCS) $(SCIV_SRCS) $(MEMORY_SRCS) $(MSM_SRCS) $(STP_SRCS)' in line:
            if '$(TDT_SRCS)' not in line:
                new_lines.append(line.rstrip() + ' $(TDT_SRCS)\n')
                modified = True
                continue
        
        # 4. Add TDT test source (after TEST_STP_SRC at line 100)
        if 'TEST_STP_SRC := tests/test_stp.c' in line and i < len(lines) - 1:
            new_lines.append(line)
            # Check if TDT test doesn't exist yet
            if i + 1 < len(lines) and 'TEST_TDT_SRC' not in lines[i + 1]:
                new_lines.append('TEST_TDT_SRC := tests/test_tdt.c\n')
                modified = True
            continue
        
        # 5. Update ALL_TEST_SRCS (line 102)
        if 'ALL_TEST_SRCS := $(TEST_BPME_SRC) $(TEST_SCIV_SRC) $(TEST_MEMORY_SRC) $(TEST_MSM_SRC) $(TEST_STP_SRC)' in line:
            if '$(TEST_TDT_SRC)' not in line:
                new_lines.append(line.rstrip() + ' $(TEST_TDT_SRC)\n')
                modified = True
                continue
        
        # 6. Add TDT test object (after TEST_STP_OBJ at line 115)
        if 'TEST_STP_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_STP_SRC))' in line and i < len(lines) - 1:
            new_lines.append(line)
            if i + 1 < len(lines) and 'TEST_TDT_OBJ' not in lines[i + 1]:
                new_lines.append('TEST_TDT_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_TDT_SRC))\n')
                modified = True
            continue
        
        # 7. Add TDT test executable (after TEST_STP at line 128)
        if 'TEST_STP := $(TEST_DIR)/test_stp' in line and i < len(lines) - 1:
            new_lines.append(line)
            if i + 1 < len(lines) and 'TEST_TDT' not in lines[i + 1]:
                new_lines.append('TEST_TDT := $(TEST_DIR)/test_tdt\n')
                modified = True
            continue
        
        # 8. Update ALL_TESTS (line 130)
        if 'ALL_TESTS := $(TEST_BPME) $(TEST_SCIV) $(TEST_MEMORY) $(TEST_MSM) $(TEST_STP)' in line:
            if '$(TEST_TDT)' not in line:
                new_lines.append(line.rstrip() + ' $(TEST_TDT)\n')
                modified = True
                continue
        
        # 9. Update .PHONY target (line 138)
        if '.PHONY: test-bpme test-sciv test-memory test-msm' in line:
            if 'test-tdt' not in line:
                new_lines.append(line.rstrip() + ' test-tdt\n')
                modified = True
                continue
        
        # 10. Update OBJ_DIR dependencies (line 166)
        if '$(OBJ_DIR)/common $(OBJ_DIR)/bpme $(OBJ_DIR)/sciv $(OBJ_DIR)/memory_layer $(OBJ_DIR)/msm $(OBJ_DIR)/stp $(OBJ_DIR)/cli $(OBJ_DIR)/tests:' in line:
            if '$(OBJ_DIR)/tdt' not in line:
                new_line = line.replace('$(OBJ_DIR)/stp $(OBJ_DIR)/cli', '$(OBJ_DIR)/stp $(OBJ_DIR)/tdt $(OBJ_DIR)/cli')
                new_lines.append(new_line)
                modified = True
                continue
        
        # 11. Update pattern rule dependencies (line 170)
        if '$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)/common $(OBJ_DIR)/bpme $(OBJ_DIR)/sciv $(OBJ_DIR)/memory_layer $(OBJ_DIR)/msm $(OBJ_DIR)/stp $(OBJ_DIR)/cli $(OBJ_DIR)/tests' in line:
            if '$(OBJ_DIR)/tdt' not in line:
                new_line = line.replace('$(OBJ_DIR)/stp $(OBJ_DIR)/cli', '$(OBJ_DIR)/stp $(OBJ_DIR)/tdt $(OBJ_DIR)/cli')
                new_lines.append(new_line)
                modified = True
                continue
        
        # 12. Add TDT test build rule (after TEST_STP rule around line 204)
        if '$(TEST_STP): $(TEST_STP_OBJ) $(STATIC_LIB) | $(TEST_DIR)' in line:
            new_lines.append(line)
            # Add next 2 lines (echo and gcc)
            if i + 1 < len(lines):
                new_lines.append(lines[i + 1])
            if i + 2 < len(lines):
                new_lines.append(lines[i + 2])
            
            # Check if TDT rule doesn't exist
            if i + 3 < len(lines) and 'TEST_TDT' not in lines[i + 3]:
                new_lines.append('\n')
                new_lines.append('$(TEST_TDT): $(TEST_TDT_OBJ) $(STATIC_LIB) | $(TEST_DIR)\n')
                # CRITICAL: Use actual TAB character for recipe lines
                new_lines.append('\t@echo "==> Linking test: $@"\n')
                new_lines.append('\t@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lcrrss $(LIBS)\n')
                modified = True
                # Skip the next lines since we already added them
                continue
        
        # 13. Add TDT test run command (after test-stp around line 254)
        if 'test-stp: $(TEST_STP)' in line:
            new_lines.append(line)
            # Add next 2 lines
            if i + 1 < len(lines):
                new_lines.append(lines[i + 1])
            if i + 2 < len(lines):
                new_lines.append(lines[i + 2])
            
            # Check if TDT test target doesn't exist
            if i + 3 < len(lines) and 'test-tdt' not in lines[i + 3]:
                new_lines.append('\n')
                new_lines.append('test-tdt: $(TEST_TDT)\n')
                new_lines.append('\t@echo "==> Running TDT tests..."\n')
                new_lines.append('\t@$(TEST_TDT)\n')
                modified = True
                continue
        
        # 14. Add TDT to run-tests (around line 230)
        if '\t@$(TEST_STP) && echo "✓ STP tests passed" || echo "✗ STP tests failed"' in line:
            new_lines.append(line)
            # Check if TDT run doesn't exist
            if i + 1 < len(lines) and 'TEST_TDT' not in lines[i + 1]:
                new_lines.append('\t@echo ""\n')
                new_lines.append('\t@echo "==> Running TDT tests..."\n')
                new_lines.append('\t@$(TEST_TDT) && echo "✓ TDT tests passed" || echo "✗ TDT tests failed"\n')
                modified = True
            continue
        
        # 15. Update info target to mention TDT (around line 367)
        if '  - MSM:        Memory-Safety Maniac Profile' in line:
            new_lines.append(line)
            if i + 1 < len(lines) and 'TDT' not in lines[i + 1]:
                new_lines.append('  - TDT:        Test-Driven Timmy Profile\n')
                modified = True
            continue
        
        # 16. Add TDT test to help (around line 385)
        if '  make test-msm         - Run MSM tests' in line:
            new_lines.append(line)
            if i + 1 < len(lines) and 'test-tdt' not in lines[i + 1]:
                new_lines.append('  make test-tdt         - Run TDT tests\n')
                modified = True
            continue
        
        # 17. Add TDT test dependency (around line 416)
        if '-include $(TEST_STP_OBJ:.o=.d)' in line and i < len(lines) - 1:
            new_lines.append(line)
            if i + 1 < len(lines) and 'TEST_TDT_OBJ' not in lines[i + 1]:
                new_lines.append('-include $(TEST_TDT_OBJ:.o=.d)\n')
                modified = True
            continue
        
        # Default: keep line as-is
        new_lines.append(line)
    
    if not modified:
        print("No changes needed - TDT already integrated")
        return 0
    
    # Write the updated Makefile
    with open(makefile_path, 'w') as f:
        f.writelines(new_lines)
    
    print("✓ Makefile updated with TDT support")
    print("✓ All tabs preserved correctly")
    return 0

if __name__ == '__main__':
    sys.exit(main())
