
# ===================================================================
# BDI Modular Kernel Build System
# Supports multiple profiles and architectures
# ===================================================================

# Build configuration
CC ?= gcc
CXX ?= g++
AR ?= ar
PROFILE ?= balanced
TARGET ?= x86_64
FAST ?= 0
DEBUG ?= 0

# Directories
SRCDIR = ..
BUILDDIR = build/$(TARGET)/$(PROFILE)
OBJDIR = $(BUILDDIR)/obj
BINDIR = $(BUILDDIR)/bin
LIBDIR = $(BUILDDIR)/lib

# Source files
UABI_SOURCES = $(SRCDIR)/uabi/uops.c
CAPGRAPH_SOURCES = $(SRCDIR)/capgraph/capability.c
ORCHESTRATOR_SOURCES = $(SRCDIR)/orchestrator/orchestrator.c
ATTENTION_MM_SOURCES = $(SRCDIR)/attention_mm/attention_mm.c
ARCH_SOURCES = $(SRCDIR)/arch/$(TARGET)/uops_$(TARGET).c
TEST_SOURCES = $(SRCDIR)/tests/test_modular.c

ALL_SOURCES = $(UABI_SOURCES) $(CAPGRAPH_SOURCES) $(ORCHESTRATOR_SOURCES) \
	      $(ATTENTION_MM_SOURCES) $(ARCH_SOURCES)

# Object files
UABI_OBJECTS = $(OBJDIR)/uops.o
CAPGRAPH_OBJECTS = $(OBJDIR)/capability.o
ORCHESTRATOR_OBJECTS = $(OBJDIR)/orchestrator.o
ATTENTION_MM_OBJECTS = $(OBJDIR)/attention_mm.o
ARCH_OBJECTS = $(OBJDIR)/uops_$(TARGET).o
TEST_OBJECTS = $(OBJDIR)/test_modular.o

ALL_OBJECTS = $(UABI_OBJECTS) $(CAPGRAPH_OBJECTS) $(ORCHESTRATOR_OBJECTS) \
	      $(ATTENTION_MM_OBJECTS) $(ARCH_OBJECTS)

# Compiler flags
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -I$(SRCDIR)
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -I$(SRCDIR)
LDFLAGS = -lm -lpthread

# Architecture-specific flags
ifeq ($(TARGET),x86_64)
    CFLAGS += -march=native -mtune=native
    ifeq ($(shell grep -q avx2 /proc/cpuinfo && echo yes),yes)
	CFLAGS += -mavx2 -mfma
    endif
    ifeq ($(shell grep -q avx512f /proc/cpuinfo && echo yes),yes)
	CFLAGS += -mavx512f -mavx512dq
    endif
endif

ifeq ($(TARGET),arm64)
    CFLAGS += -march=armv8-a+simd
endif

ifeq ($(TARGET),riscv64)
    CFLAGS += -march=rv64gcv
endif

# Profile-specific optimizations
ifeq ($(PROFILE),latency)
    CFLAGS += -O3 -DBDI_PROFILE_LATENCY -fno-omit-frame-pointer
endif

ifeq ($(PROFILE),throughput)
    CFLAGS += -O3 -DBDI_PROFILE_THROUGHPUT -flto
    LDFLAGS += -flto
endif

ifeq ($(PROFILE),ai-train)
    CFLAGS += -O3 -DBDI_PROFILE_AI_TRAIN -flto -ffast-math
    LDFLAGS += -flto
endif

ifeq ($(PROFILE),energy)
    CFLAGS += -Os -DBDI_PROFILE_ENERGY
endif

ifeq ($(PROFILE),balanced)
    CFLAGS += -O2 -DBDI_PROFILE_BALANCED
endif

# Fast build mode
ifeq ($(FAST),1)
    CFLAGS += -DBDI_FAST_BUILD
endif

# Debug mode
ifeq ($(DEBUG),1)
    CFLAGS += -g -DDEBUG -O0
else
    CFLAGS += -DNDEBUG
endif

# Targets
.PHONY: all clean test install help

all: $(BINDIR)/bdi_modular_test $(LIBDIR)/libbdi_modular.a

# Create directories
$(OBJDIR) $(BINDIR) $(LIBDIR):
	mkdir -p $@

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/uabi/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/capgraph/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/orchestrator/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/attention_mm/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/arch/$(TARGET)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/tests/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Build static library
$(LIBDIR)/libbdi_modular.a: $(ALL_OBJECTS) | $(LIBDIR)
	$(AR) rcs $@ $^

# Build test executable
$(BINDIR)/bdi_modular_test: $(TEST_OBJECTS) $(LIBDIR)/libbdi_modular.a | $(BINDIR)
	$(CC) $^ -o $@ $(LDFLAGS)

# Run tests
test: $(BINDIR)/bdi_modular_test
	@echo "Running BDI Modular Kernel tests..."
	@echo "Profile: $(PROFILE), Target: $(TARGET)"
	$(BINDIR)/bdi_modular_test

# Performance benchmark
benchmark: $(BINDIR)/bdi_modular_test
	@echo "Running performance benchmarks..."
	@echo "Profile: $(PROFILE), Target: $(TARGET)"
	$(BINDIR)/bdi_modular_test --benchmark

# Install (placeholder)
install: all
	@echo "Install target not implemented yet"

# Clean build artifacts
clean:
	rm -rf build/

# Clean specific profile/target
clean-profile:
	rm -rf $(BUILDDIR)

# Help
help:
	@echo "BDI Modular Kernel Build System"
	@echo ""
	@echo "Usage: make [target] [options]"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build library and tests (default)"
	@echo "  test       - Build and run tests"
	@echo "  benchmark  - Run performance benchmarks"
	@echo "  clean      - Clean all build artifacts"
	@echo "  clean-profile - Clean current profile/target"
	@echo "  install    - Install built artifacts"
	@echo "  help       - Show this help"
	@echo ""
	@echo "Options:"
	@echo "  PROFILE={latency|throughput|ai-train|energy|balanced}"
	@echo "          - Optimization profile (default: balanced)"
	@echo "  TARGET={x86_64|arm64|riscv64}"
	@echo "          - Target architecture (default: x86_64)"
	@echo "  FAST=1  - Fast build mode"
	@echo "  DEBUG=1 - Debug build"
	@echo "  CC=     - C compiler (default: gcc)"
	@echo ""
	@echo "Examples:"
	@echo "  make PROFILE=ai-train TARGET=x86_64"
	@echo "  make test PROFILE=latency DEBUG=1"
	@echo "  make benchmark PROFILE=throughput FAST=1"

# Show build configuration
config:
	@echo "Build Configuration:"
	@echo "  Profile: $(PROFILE)"
	@echo "  Target:  $(TARGET)"
	@echo "  CC:      $(CC)"
	@echo "  Fast:    $(FAST)"
	@echo "  Debug:   $(DEBUG)"
	@echo "  CFLAGS:  $(CFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"

# Profile-specific targets
latency:
	$(MAKE) PROFILE=latency

throughput:
	$(MAKE) PROFILE=throughput

ai-train:
	$(MAKE) PROFILE=ai-train

energy:
	$(MAKE) PROFILE=energy

balanced:
	$(MAKE) PROFILE=balanced

# Architecture-specific targets
x86_64:
	$(MAKE) TARGET=x86_64

arm64:
	$(MAKE) TARGET=arm64

riscv64:
	$(MAKE) TARGET=riscv64

# Combined targets
ai-train-x86_64:
	$(MAKE) PROFILE=ai-train TARGET=x86_64

latency-x86_64:
	$(MAKE) PROFILE=latency TARGET=x86_64

throughput-x86_64:
	$(MAKE) PROFILE=throughput TARGET=x86_64

# Dependencies
$(ALL_OBJECTS): $(SRCDIR)/uabi/uops.h $(SRCDIR)/capgraph/capability.h \
	        $(SRCDIR)/orchestrator/orchestrator.h $(SRCDIR)/attention_mm/attention_mm.h \
	        $(SRCDIR)/bricks/module.h

$(TEST_OBJECTS): $(ALL_OBJECTS)
