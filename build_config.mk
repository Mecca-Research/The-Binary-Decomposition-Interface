
# BDI Kernel Build Configuration - Phase 6
# Advanced build configuration with ISA detection and feature flags

# ============================================================================
# Build System Configuration
# ============================================================================

# Build system version
BUILD_SYSTEM_VERSION := 6.0.0

# Parallel build jobs (auto-detect CPU cores)
MAKEFLAGS += -j$(shell nproc 2>/dev/null || echo 4)

# Silent build (set to 1 for verbose)
VERBOSE ?= 0
ifeq ($(VERBOSE),0)
    MAKEFLAGS += --no-print-directory
    Q := @
else
    Q :=
endif

# ============================================================================
# Compiler Detection and Configuration
# ============================================================================

# Detect GCC version
GCC_VERSION := $(shell $(CC) -dumpversion 2>/dev/null)
GCC_MAJOR := $(shell echo $(GCC_VERSION) | cut -d. -f1)
GCC_MINOR := $(shell echo $(GCC_VERSION) | cut -d. -f2)

# Check for minimum GCC version (12.0 for C23 support)
GCC_VERSION_CHECK := $(shell [ $(GCC_MAJOR) -ge 12 ] && echo 1 || echo 0)

ifeq ($(GCC_VERSION_CHECK),0)
    $(warning WARNING: GCC version $(GCC_VERSION) detected. GCC 12+ recommended for full C23 support)
endif

# ============================================================================
# ISA Detection and Configuration
# ============================================================================

# Detect CPU features
CPU_FLAGS := $(shell $(CC) -march=native -dM -E - < /dev/null 2>/dev/null)

# SSE Support
HAS_SSE2 := $(shell echo "$(CPU_FLAGS)" | grep -q __SSE2__ && echo 1 || echo 0)
HAS_SSE3 := $(shell echo "$(CPU_FLAGS)" | grep -q __SSE3__ && echo 1 || echo 0)
HAS_SSE4_1 := $(shell echo "$(CPU_FLAGS)" | grep -q __SSE4_1__ && echo 1 || echo 0)
HAS_SSE4_2 := $(shell echo "$(CPU_FLAGS)" | grep -q __SSE4_2__ && echo 1 || echo 0)

# AVX Support
HAS_AVX := $(shell echo "$(CPU_FLAGS)" | grep -q __AVX__ && echo 1 || echo 0)
HAS_AVX2 := $(shell echo "$(CPU_FLAGS)" | grep -q __AVX2__ && echo 1 || echo 0)
HAS_FMA := $(shell echo "$(CPU_FLAGS)" | grep -q __FMA__ && echo 1 || echo 0)

# AVX-512 Support
HAS_AVX512F := $(shell echo "$(CPU_FLAGS)" | grep -q __AVX512F__ && echo 1 || echo 0)
HAS_AVX512CD := $(shell echo "$(CPU_FLAGS)" | grep -q __AVX512CD__ && echo 1 || echo 0)
HAS_AVX512BW := $(shell echo "$(CPU_FLAGS)" | grep -q __AVX512BW__ && echo 1 || echo 0)
HAS_AVX512DQ := $(shell echo "$(CPU_FLAGS)" | grep -q __AVX512DQ__ && echo 1 || echo 0)
HAS_AVX512VL := $(shell echo "$(CPU_FLAGS)" | grep -q __AVX512VL__ && echo 1 || echo 0)

# Other useful extensions
HAS_BMI1 := $(shell echo "$(CPU_FLAGS)" | grep -q __BMI__ && echo 1 || echo 0)
HAS_BMI2 := $(shell echo "$(CPU_FLAGS)" | grep -q __BMI2__ && echo 1 || echo 0)
HAS_POPCNT := $(shell echo "$(CPU_FLAGS)" | grep -q __POPCNT__ && echo 1 || echo 0)
HAS_AES := $(shell echo "$(CPU_FLAGS)" | grep -q __AES__ && echo 1 || echo 0)

# ============================================================================
# Feature Flags
# ============================================================================

# Enable features based on ISA support
FEATURE_FLAGS :=

ifeq ($(HAS_SSE4_2),1)
    FEATURE_FLAGS += -DHAS_SSE4_2
endif

ifeq ($(HAS_AVX2),1)
    FEATURE_FLAGS += -DHAS_AVX2
endif

ifeq ($(HAS_FMA),1)
    FEATURE_FLAGS += -DHAS_FMA
endif

ifeq ($(HAS_AVX512F),1)
    FEATURE_FLAGS += -DHAS_AVX512F
endif

ifeq ($(HAS_BMI2),1)
    FEATURE_FLAGS += -DHAS_BMI2
endif

ifeq ($(HAS_POPCNT),1)
    FEATURE_FLAGS += -DHAS_POPCNT
endif

ifeq ($(HAS_AES),1)
    FEATURE_FLAGS += -DHAS_AES
endif

# Add feature flags to CFLAGS
CFLAGS += $(FEATURE_FLAGS)

# ============================================================================
# Optimization Profiles
# ============================================================================

# Size optimization profile
ifeq ($(OPTIMIZE_SIZE),1)
    CFLAGS := $(filter-out -O3,$(CFLAGS))
    CFLAGS += -Os -fno-inline-functions
endif

# Performance optimization profile (default)
ifeq ($(OPTIMIZE_PERFORMANCE),1)
    CFLAGS += -O3 -funroll-loops -fprefetch-loop-arrays
endif

# ============================================================================
# Security Hardening (optional)
# ============================================================================

ifeq ($(ENABLE_HARDENING),1)
    CFLAGS += -fstack-protector-strong
    CFLAGS += -D_FORTIFY_SOURCE=2
    CFLAGS += -fPIE
    LDFLAGS += -pie -Wl,-z,relro -Wl,-z,now
endif

# ============================================================================
# Debug Configuration
# ============================================================================

ifeq ($(ENABLE_DEBUG_SYMBOLS),1)
    CFLAGS += -g3 -ggdb
endif

ifeq ($(ENABLE_COVERAGE),1)
    CFLAGS += --coverage
    LDFLAGS += --coverage
endif

# ============================================================================
# PGO Configuration
# ============================================================================

# PGO data directory
PGO_DATA_DIR ?= ./pgo-data

# PGO training workload
PGO_WORKLOAD ?= --benchmark

# ============================================================================
# LTO Configuration
# ============================================================================

# LTO mode (auto, thin, full)
LTO_MODE ?= auto

ifeq ($(LTO_MODE),thin)
    CFLAGS := $(filter-out -flto=auto,$(CFLAGS))
    CFLAGS += -flto=thin
    LDFLAGS := $(filter-out -flto=auto,$(LDFLAGS))
    LDFLAGS += -flto=thin
endif

ifeq ($(LTO_MODE),full)
    CFLAGS := $(filter-out -flto=auto,$(CFLAGS))
    CFLAGS += -flto
    LDFLAGS := $(filter-out -flto=auto,$(LDFLAGS))
    LDFLAGS += -flto
endif

# ============================================================================
# Build Paths
# ============================================================================

BUILD_DIR ?= build
OBJ_DIR ?= $(BUILD_DIR)/obj
BIN_DIR ?= $(BUILD_DIR)/bin
LIB_DIR ?= $(BUILD_DIR)/lib

# ============================================================================
# Installation Paths
# ============================================================================

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include

# ============================================================================
# Build Information
# ============================================================================

BUILD_DATE := $(shell date +"%Y-%m-%d %H:%M:%S")
BUILD_HOST := $(shell hostname)
BUILD_USER := $(shell whoami)

# Add build info to binary
CFLAGS += -DBUILD_DATE='"$(BUILD_DATE)"'
CFLAGS += -DBUILD_HOST='"$(BUILD_HOST)"'
CFLAGS += -DBUILD_USER='"$(BUILD_USER)"'
CFLAGS += -DBUILD_VERSION='"$(BUILD_SYSTEM_VERSION)"'

# ============================================================================
# Export Configuration
# ============================================================================

export CC CFLAGS LDFLAGS
export BUILD_MODE
export HAS_AVX2 HAS_AVX512F
export GCC_VERSION GCC_MAJOR GCC_MINOR
