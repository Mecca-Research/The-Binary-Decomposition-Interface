# ============================================================================
# CRRSS - Code Review, Reliability, and Static Safety System
# Makefile - Build System Integration
# Phase 1B Stage 4: Build System Integration & Documentation
# ============================================================================

# ============================================================================
# Build Configuration
# ============================================================================

# Compiler Configuration
CC := gcc
AR := ar
RANLIB := ranlib

# C23 Standard with full feature support
CFLAGS := -std=c2x -Wall -Wextra -Wpedantic -Werror
CFLAGS += -Wno-unknown-pragmas

# Include paths
CFLAGS += -I. -Icommon -Ibpme -Isciv -Imemory_layer -Imsm -Istp -Icli

# Default build mode (can be: debug, release)
BUILD_MODE ?= release

# ============================================================================
# Build Mode Flags
# ============================================================================

# CRRSS is a user-space tool - clear any inherited kernel-specific linker flags
# (e.g., -T linker.ld from parent Makefile)
override LDFLAGS :=

ifeq ($(BUILD_MODE),debug)
    # Debug build: minimal optimization, full debug info
    CFLAGS += -O0 -g3 -DDEBUG -fno-omit-frame-pointer
    CFLAGS += -fsanitize=address -fsanitize=undefined
    LDFLAGS += -fsanitize=address -fsanitize=undefined
else
    # Release build: maximum optimization
    CFLAGS += -O3 -march=native -mtune=native -DNDEBUG
    CFLAGS += -flto=auto -fuse-linker-plugin
    LDFLAGS += -flto=auto -fuse-linker-plugin
endif

# Additional flags
CFLAGS += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections

# Libraries
LIBS := -lm -lpthread

# ============================================================================
# Directories
# ============================================================================

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
BIN_DIR := $(BUILD_DIR)/bin
LIB_DIR := $(BUILD_DIR)/lib
TEST_DIR := $(BUILD_DIR)/test
INSTALL_PREFIX ?= /usr/local

# ============================================================================
# Source Files
# ============================================================================

# Common sources
COMMON_SRCS := common/crrss_types.c

# BPME sources
BPME_SRCS := bpme/bpme.c

# SCIV sources
SCIV_SRCS := sciv/sciv.c

# Memory Integration sources
MEMORY_SRCS := memory_layer/memory_integration.c

# MSM sources
MSM_SRCS := msm/msm.c

# STP sources
STP_SRCS := stp/stp.c

# CLI sources
CLI_SRCS := cli/crrss_cli.c

# Main CLI executable
MAIN_SRC := cli/crrss_main.c

# All library sources
LIB_SRCS := $(COMMON_SRCS) $(BPME_SRCS) $(SCIV_SRCS) $(MEMORY_SRCS) $(MSM_SRCS) $(STP_SRCS) $(CLI_SRCS)

# Test sources
TEST_BPME_SRC := tests/test_bpme.c
TEST_SCIV_SRC := tests/test_sciv.c
TEST_MEMORY_SRC := tests/test_memory.c
TEST_MSM_SRC := tests/test_msm.c
TEST_STP_SRC := tests/test_stp.c

ALL_TEST_SRCS := $(TEST_BPME_SRC) $(TEST_SCIV_SRC) $(TEST_MEMORY_SRC) $(TEST_MSM_SRC) $(TEST_STP_SRC)

# ============================================================================
# Object Files
# ============================================================================

LIB_OBJS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(LIB_SRCS))
MAIN_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(MAIN_SRC))

TEST_BPME_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_BPME_SRC))
TEST_SCIV_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_SCIV_SRC))
TEST_MEMORY_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_MEMORY_SRC))
TEST_MSM_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_MSM_SRC))
TEST_STP_OBJ := $(patsubst %.c,$(OBJ_DIR)/%.o,$(TEST_STP_SRC))

# ============================================================================
# Output Files
# ============================================================================

STATIC_LIB := $(LIB_DIR)/libcrrss.a
CRRSS_TOOL := $(BIN_DIR)/crrss

TEST_BPME := $(TEST_DIR)/test_bpme
TEST_SCIV := $(TEST_DIR)/test_sciv
TEST_MEMORY := $(TEST_DIR)/test_memory
TEST_MSM := $(TEST_DIR)/test_msm
TEST_STP := $(TEST_DIR)/test_stp

ALL_TESTS := $(TEST_BPME) $(TEST_SCIV) $(TEST_MEMORY) $(TEST_MSM) $(TEST_STP)

# ============================================================================
# Phony Targets
# ============================================================================

.PHONY: all clean test install uninstall help info
.PHONY: lib tool tests run-tests
.PHONY: test-bpme test-sciv test-memory test-msm
.PHONY: check validate
.PHONY: check-codebase analyze-bdi

# ============================================================================
# Main Targets
# ============================================================================

all: lib tool tests
	@echo "==> CRRSS build complete [$(BUILD_MODE)]"

lib: $(STATIC_LIB)
	@echo "==> CRRSS library built: $(STATIC_LIB)"

tool: $(CRRSS_TOOL)
	@echo "==> CRRSS tool built: $(CRRSS_TOOL)"

tests: $(ALL_TESTS)
	@echo "==> CRRSS tests built"

# ============================================================================
# Build Rules
# ============================================================================

# Create directories
$(BUILD_DIR) $(OBJ_DIR) $(BIN_DIR) $(LIB_DIR) $(TEST_DIR):
	@mkdir -p $@

$(OBJ_DIR)/common $(OBJ_DIR)/bpme $(OBJ_DIR)/sciv $(OBJ_DIR)/memory_layer $(OBJ_DIR)/msm $(OBJ_DIR)/stp $(OBJ_DIR)/cli $(OBJ_DIR)/tests:
	@mkdir -p $@

# Build object files
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)/common $(OBJ_DIR)/bpme $(OBJ_DIR)/sciv $(OBJ_DIR)/memory_layer $(OBJ_DIR)/msm $(OBJ_DIR)/stp $(OBJ_DIR)/cli $(OBJ_DIR)/tests
	@echo "  CC  $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Build static library
$(STATIC_LIB): $(LIB_OBJS) | $(LIB_DIR)
	@echo "==> Creating static library: $@"
	@$(AR) rcs $@ $^
	@$(RANLIB) $@

# Build CRRSS tool
$(CRRSS_TOOL): $(MAIN_OBJ) $(STATIC_LIB) | $(BIN_DIR)
	@echo "==> Linking CRRSS tool: $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lcrrss $(LIBS)

# Build test executables
$(TEST_BPME): $(TEST_BPME_OBJ) $(STATIC_LIB) | $(TEST_DIR)
	@echo "==> Linking test: $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lcrrss $(LIBS)

$(TEST_SCIV): $(TEST_SCIV_OBJ) $(STATIC_LIB) | $(TEST_DIR)
	@echo "==> Linking test: $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lcrrss $(LIBS)

$(TEST_MEMORY): $(TEST_MEMORY_OBJ) $(STATIC_LIB) | $(TEST_DIR)
	@echo "==> Linking test: $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lcrrss $(LIBS)

$(TEST_MSM): $(TEST_MSM_OBJ) $(STATIC_LIB) | $(TEST_DIR)
	@echo "==> Linking test: $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lcrrss $(LIBS)

$(TEST_STP): $(TEST_STP_OBJ) $(STATIC_LIB) | $(TEST_DIR)
	@echo "==> Linking test: $@"
	@$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(LIB_DIR) -lcrrss $(LIBS)

# ============================================================================
# Testing Targets
# ============================================================================

test: run-tests

run-tests: tests
	@echo "========================================"
	@echo "Running CRRSS Test Suite"
	@echo "========================================"
	@echo ""
	@echo "==> Running BPME tests..."
	@$(TEST_BPME) && echo "✓ BPME tests passed" || echo "✗ BPME tests failed"
	@echo ""
	@echo "==> Running SCIV tests..."
	@$(TEST_SCIV) && echo "✓ SCIV tests passed" || echo "✗ SCIV tests failed"
	@echo ""
	@echo "==> Running Memory Integration tests..."
	@$(TEST_MEMORY) && echo "✓ Memory tests passed" || echo "✗ Memory tests failed"
	@echo ""
	@echo "==> Running MSM tests..."
	@$(TEST_MSM) && echo "✓ MSM tests passed" || echo "✗ MSM tests failed"
	@echo ""
	@echo "==> Running STP tests..."
	@$(TEST_STP) && echo "✓ STP tests passed" || echo "✗ STP tests failed"
	@echo ""
	@echo "========================================"
	@echo "CRRSS Test Suite Complete"
	@echo "========================================"

test-bpme: $(TEST_BPME)
	@echo "==> Running BPME tests..."
	@$(TEST_BPME)

test-sciv: $(TEST_SCIV)
	@echo "==> Running SCIV tests..."
	@$(TEST_SCIV)

test-memory: $(TEST_MEMORY)
	@echo "==> Running Memory Integration tests..."
	@$(TEST_MEMORY)

test-msm: $(TEST_MSM)
	@echo "==> Running MSM tests..."
	@$(TEST_MSM)

test-stp: $(TEST_STP)
	@echo "==> Running STP tests..."
	@$(TEST_STP)

check: run-tests

# ============================================================================
# Validation Targets
# ============================================================================

validate: tool
	@echo "==> Validating CRRSS build..."
	@if [ -f $(CRRSS_TOOL) ]; then \
                echo "✓ CRRSS tool exists"; \
                ls -lh $(CRRSS_TOOL); \
                echo "✓ Validation passed"; \
        else \
                echo "✗ Validation failed"; \
                exit 1; \
        fi

# ============================================================================
# BDI Codebase Analysis Targets
# ============================================================================

check-codebase: tool
	@echo "==> Running CRRSS checks on BDI codebase..."
	@echo ""
	@echo "This will analyze the BDI kernel code with CRRSS tools."
	@echo "Note: Requires CRRSS tool to be built."
	@echo ""
	@if [ ! -f $(CRRSS_TOOL) ]; then \
                echo "Error: CRRSS tool not found. Run 'make tool' first."; \
                exit 1; \
        fi
	@echo "==> Analyzing moduler_kernel/ with MSM..."
	@$(CRRSS_TOOL) msm -d ../../moduler_kernel/ --report crrss_bdi_analysis.txt --format text || echo "Note: Analysis may have found issues"
	@echo ""
	@echo "==> Analysis complete. See crrss_bdi_analysis.txt for results."

analyze-bdi: check-codebase

# Quick analysis of specific file
analyze-file: tool
	@if [ -z "$(FILE)" ]; then \
                echo "Error: Please specify FILE=<path>"; \
                echo "Example: make analyze-file FILE=../../moduler_kernel/memory.c"; \
                exit 1; \
        fi
	@echo "==> Analyzing $(FILE) with CRRSS..."
	@$(CRRSS_TOOL) msm -f $(FILE) --report crrss_file_analysis.txt --format text || echo "Analysis complete"
	@echo ""
	@echo "==> Analysis complete. See crrss_file_analysis.txt for results."

# ============================================================================
# Installation Targets
# ============================================================================

install: all
	@echo "==> Installing CRRSS to $(INSTALL_PREFIX)..."
	@mkdir -p $(INSTALL_PREFIX)/bin
	@mkdir -p $(INSTALL_PREFIX)/lib
	@mkdir -p $(INSTALL_PREFIX)/include/crrss
	@mkdir -p $(INSTALL_PREFIX)/share/doc/crrss
	@cp $(CRRSS_TOOL) $(INSTALL_PREFIX)/bin/
	@cp $(STATIC_LIB) $(INSTALL_PREFIX)/lib/
	@cp -r common/*.h $(INSTALL_PREFIX)/include/crrss/
	@cp -r bpme/*.h $(INSTALL_PREFIX)/include/crrss/
	@cp -r sciv/*.h $(INSTALL_PREFIX)/include/crrss/
	@cp -r memory_layer/*.h $(INSTALL_PREFIX)/include/crrss/
	@cp -r msm/*.h $(INSTALL_PREFIX)/include/crrss/
	@cp -r cli/*.h $(INSTALL_PREFIX)/include/crrss/
	@if [ -d docs ]; then cp -r docs/* $(INSTALL_PREFIX)/share/doc/crrss/; fi
	@if [ -f README.md ]; then cp README.md $(INSTALL_PREFIX)/share/doc/crrss/; fi
	@echo "==> CRRSS installed successfully"

uninstall:
	@echo "==> Uninstalling CRRSS from $(INSTALL_PREFIX)..."
	@rm -f $(INSTALL_PREFIX)/bin/crrss
	@rm -f $(INSTALL_PREFIX)/lib/libcrrss.a
	@rm -rf $(INSTALL_PREFIX)/include/crrss
	@rm -rf $(INSTALL_PREFIX)/share/doc/crrss
	@echo "==> CRRSS uninstalled"

# ============================================================================
# Cleaning Targets
# ============================================================================

clean:
	@echo "==> Cleaning CRRSS build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -f crrss_*.txt crrss_*.json crrss_*.html
	@find . -name "*.o" -type f -delete
	@find . -name "*.d" -type f -delete
	@echo "==> Clean complete"

# ============================================================================
# Information Targets
# ============================================================================

info:
	@echo "========================================"
	@echo "CRRSS Build System - Stage 4"
	@echo "========================================"
	@echo "Version:        1.0.0"
	@echo "Build Mode:     $(BUILD_MODE)"
	@echo "Compiler:       $(CC)"
	@echo "CFLAGS:         $(CFLAGS)"
	@echo "LDFLAGS:        $(LDFLAGS)"
	@echo ""
	@echo "Components:"
	@echo "  - BPME:       Bug Prior Mapping Engine"
	@echo "  - SCIV:       Self-Check Internal Validator"
	@echo "  - Memory:     Memory Integration Layer"
	@echo "  - MSM:        Memory-Safety Maniac Profile"
	@echo "  - CLI:        Command-Line Interface"
	@echo ""
	@echo "Outputs:"
	@echo "  Library:      $(STATIC_LIB)"
	@echo "  Tool:         $(CRRSS_TOOL)"
	@echo "  Tests:        $(words $(ALL_TESTS)) test executables"
	@echo ""
	@echo "Build Targets:"
	@echo "  make all              - Build everything"
	@echo "  make lib              - Build static library"
	@echo "  make tool             - Build CRRSS tool"
	@echo "  make tests            - Build test suite"
	@echo ""
	@echo "Test Targets:"
	@echo "  make test             - Run all tests"
	@echo "  make test-bpme        - Run BPME tests"
	@echo "  make test-sciv        - Run SCIV tests"
	@echo "  make test-memory      - Run Memory tests"
	@echo "  make test-msm         - Run MSM tests"
	@echo ""
	@echo "Analysis Targets:"
	@echo "  make check-codebase   - Analyze BDI codebase"
	@echo "  make analyze-file     - Analyze specific file"
	@echo "                          (use FILE=<path>)"
	@echo ""
	@echo "Installation:"
	@echo "  make install          - Install CRRSS"
	@echo "  make uninstall        - Uninstall CRRSS"
	@echo "  INSTALL_PREFIX=<path> - Custom install path"
	@echo ""
	@echo "Other Targets:"
	@echo "  make clean            - Clean build artifacts"
	@echo "  make validate         - Validate build"
	@echo "  make help             - Show this help"
	@echo "========================================"

help: info

# ============================================================================
# Dependencies
# ============================================================================

# Auto-generate dependencies
-include $(LIB_OBJS:.o=.d)
-include $(MAIN_OBJ:.o=.d)
-include $(TEST_BPME_OBJ:.o=.d)
-include $(TEST_SCIV_OBJ:.o=.d)
-include $(TEST_MEMORY_OBJ:.o=.d)
-include $(TEST_MSM_OBJ:.o=.d)
-include $(TEST_STP_OBJ:.o=.d)

%.d: %.c
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< -MF $@
