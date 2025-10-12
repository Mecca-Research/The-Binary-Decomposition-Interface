#!/usr/bin/env python3
"""
Fix Makefile by adding TDT support while preserving tabs.
This script reads the Makefile line by line and makes precise replacements.
"""

def main():
    makefile_path = "Makefile"
    
    with open(makefile_path, 'r') as f:
        content = f.read()
    
    # Make replacements preserving exact whitespace
    replacements = [
        # 1. Include paths
        ('-I. -Icommon -Ibpme -Isciv -Imemory_layer -Imsm -Istp -Icli',
         '-I. -Icommon -Ibpme -Isciv -Imemory_layer -Imsm -Istp -Itdt -Icli'),
        
        # 2. TDT sources (after STP sources)
        ('# STP sources\nSTP_SRCS := stp/stp.c\n\n# CLI sources',
         '# STP sources\nSTP_SRCS := stp/stp.c\n\n# TDT sources\nTDT_SRCS := tdt/tdt.c tdt/tdt_generator.c tdt/tdt_coverage.c tdt/tdt_templates.c tdt/tdt_integration.c\n\n# CLI sources'),
        
        # 3. Library sources
        ('LIB_SRCS := $(COMMON_SRCS) $(BPME_SRCS) $(SCIV_SRCS) $(MEMORY_SRCS) $(MSM_SRCS) $(STP_SRCS) $(CLI_SRCS)',
         'LIB_SRCS := $(COMMON_SRCS) $(BPME_SRCS) $(SCIV_SRCS) $(MEMORY_SRCS) $(MSM_SRCS) $(STP_SRCS) $(TDT_SRCS) $(CLI_SRCS)'),
        
        # 4. Test sources
        ('TEST_STP_SRC := tests/test_stp.c\n\nALL_TEST_SRCS := $(TEST_BPME_SRC) $(TEST_SCIV_SRC) $(TEST_MEMORY_SRC) $(TEST_MSM_SRC) $(TEST_STP_SRC)',
         'TEST_STP_SRC := tests/test_stp.c\nTEST_TDT_SRC := tests/test_tdt.c\n\nALL_TEST_SRCS := $(TEST_BPME_SRC) $(TEST_SCIV_SRC) $(TEST_MEMORY_SRC) $(TEST_MSM_SRC) $(TEST_STP_SRC) $(TEST_TDT_SRC)'),
        
        # 5. Test objects
        ('TEST_STP_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_STP_SRC))\n\n# ============================================================================',
         'TEST_STP_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_STP_SRC))\nTEST_TDT_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_TDT_SRC))\n\n# ============================================================================'),
        
        # 6. Test executables
        ('TEST_STP := $(TEST_DIR)/test_stp\n\nALL_TESTS := $(TEST_BPME) $(TEST_SCIV) $(TEST_MEMORY) $(TEST_MSM) $(TEST_STP)',
         'TEST_STP := $(TEST_DIR)/test_stp\nTEST_TDT := $(TEST_DIR)/test_tdt\n\nALL_TESTS := $(TEST_BPME) $(TEST_SCIV) $(TEST_MEMORY) $(TEST_MSM) $(TEST_STP) $(TEST_TDT)'),
        
        # 7. Phony targets
        ('.PHONY: test-bpme test-sciv test-memory test-msm',
         '.PHONY: test-bpme test-sciv test-memory test-msm test-stp test-tdt'),
        
        # 8. Object directories
        ('$(OBJ_DIR)/common $(OBJ_DIR)/bpme $(OBJ_DIR)/sciv $(OBJ_DIR)/memory_layer $(OBJ_DIR)/msm $(OBJ_DIR)/stp $(OBJ_DIR)/cli $(OBJ_DIR)/tests',
         '$(OBJ_DIR)/common $(OBJ_DIR)/bpme $(OBJ_DIR)/sciv $(OBJ_DIR)/memory_layer $(OBJ_DIR)/msm $(OBJ_DIR)/stp $(OBJ_DIR)/tdt $(OBJ_DIR)/cli $(OBJ_DIR)/tests'),
        
        # 9. Test build rule (needs special handling to preserve tabs)
        ('$(TEST_STP): $(TEST_STP_OBJ) $(STATIC_LIB) | $(TEST_DIR)\n\t@echo "==> Linking test: $@"\n\t@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lcrrss $(LIBS)\n\n# ============================================================================\n# Testing Targets',
         '$(TEST_STP): $(TEST_STP_OBJ) $(STATIC_LIB) | $(TEST_DIR)\n\t@echo "==> Linking test: $@"\n\t@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lcrrss $(LIBS)\n\n$(TEST_TDT): $(TEST_TDT_OBJ) $(STATIC_LIB) | $(TEST_DIR)\n\t@echo "==> Linking test: $@"\n\t@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lcrrss $(LIBS)\n\n# ============================================================================\n# Testing Targets'),
        
        # 10. Run tests (needs special handling to preserve tabs)
        ('\t@$(TEST_STP) && echo "✓ STP tests passed" || echo "✗ STP tests failed"\n\t@echo ""\n\t@echo "========================================"\n\t@echo "CRRSS Test Suite Complete"',
         '\t@$(TEST_STP) && echo "✓ STP tests passed" || echo "✗ STP tests failed"\n\t@echo ""\n\t@echo "==> Running TDT tests..."\n\t@$(TEST_TDT) && echo "✓ TDT tests passed" || echo "✗ TDT tests failed"\n\t@echo ""\n\t@echo "========================================"\n\t@echo "CRRSS Test Suite Complete"'),
        
        # 11. Individual test target
        ('test-stp: $(TEST_STP)\n\t@echo "==> Running STP tests..."\n\t@$(TEST_STP)\n\ncheck: run-tests',
         'test-stp: $(TEST_STP)\n\t@echo "==> Running STP tests..."\n\t@$(TEST_STP)\n\ntest-tdt: $(TEST_TDT)\n\t@echo "==> Running TDT tests..."\n\t@$(TEST_TDT)\n\ncheck: run-tests'),
        
        # 12. Info components
        ('\t@echo "  - MSM:        Memory-Safety Maniac Profile"\n\t@echo "  - CLI:        Command-Line Interface"',
         '\t@echo "  - MSM:        Memory-Safety Maniac Profile"\n\t@echo "  - STP:        Strategic Timmy Profile"\n\t@echo "  - TDT:        Test-Driven Timmy Profile"\n\t@echo "  - CLI:        Command-Line Interface"'),
        
        # 13. Info test targets
        ('\t@echo "  make test-msm         - Run MSM tests"\n\t@echo ""\n\t@echo "Analysis Targets:"',
         '\t@echo "  make test-msm         - Run MSM tests"\n\t@echo "  make test-stp         - Run STP tests"\n\t@echo "  make test-tdt         - Run TDT tests"\n\t@echo ""\n\t@echo "Analysis Targets:"'),
        
        # 14. Dependencies
        ('-include $(TEST_STP_OBJ:.o=.d)\n\n%.d: %.c',
         '-include $(TEST_STP_OBJ:.o=.d)\n-include $(TEST_TDT_OBJ:.o=.d)\n\n%.d: %.c'),
    ]
    
    for old, new in replacements:
        content = content.replace(old, new)
    
    with open(makefile_path, 'w') as f:
        f.write(content)
    
    print("✓ Makefile updated with TDT support (tabs preserved)")

if __name__ == '__main__':
    main()
