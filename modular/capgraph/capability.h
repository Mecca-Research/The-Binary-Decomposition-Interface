
// ===================================================================
// BDI Capability Graph - Precise Hardware Detection System
// Probe phase: Build comprehensive capability map of the machine
// ===================================================================

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ===================================================================
// CPU Capability Flags
// ===================================================================

typedef struct {
    // === x86/x64 Instruction Sets ===
    bool sse;           // SSE support
    bool sse2;          // SSE2 support  
    bool sse3;          // SSE3 support
    bool ssse3;         // SSSE3 support
    bool sse4_1;        // SSE4.1 support
    bool sse4_2;        // SSE4.2 support
    bool avx;           // AVX support
    bool avx2;          // AVX2 support
    bool avx512f;       // AVX-512 Foundation
    bool avx512dq;      // AVX-512 DQ
    bool avx512cd;      // AVX-512 CD
    bool avx512bw;      // AVX-512 BW
    bool avx512vl;      // AVX-512 VL
    bool amx_tile;      // Intel AMX Tile
    bool amx_int8;      // Intel AMX INT8
    bool amx_bf16;      // Intel AMX BF16
    
    // === ARM Instruction Sets ===
    bool neon;          // ARM NEON
    bool sve;           // ARM SVE (Scalable Vector Extension)
    bool sve2;          // ARM SVE2
    bool sme;           // ARM SME (Scalable Matrix Extension)
    
    // === RISC-V Extensions ===
    bool rvv;           // RISC-V Vector Extension
    bool rvb;           // RISC-V Bit Manipulation
    bool rvc;           // RISC-V Compressed
    
    // === Cryptographic Extensions ===
    bool aes_ni;        // AES New Instructions
    bool pclmulqdq;     // Carry-less multiplication
    bool sha_ni;        // SHA New Instructions
    bool arm_crypto;    // ARM Crypto Extensions
    
    // === Other CPU Features ===
    bool fma;           // Fused Multiply-Add
    bool bmi1;          // Bit Manipulation Instructions 1
    bool bmi2;          // Bit Manipulation Instructions 2
    bool popcnt;        // Population Count
    bool lzcnt;         // Leading Zero Count
    bool rdrand;        // Hardware Random Number Generator
    bool rdseed;        // Hardware Seed Generator
    bool tsx;           // Transactional Synchronization Extensions
    bool mpx;           // Memory Protection Extensions
    bool cet;           // Control-flow Enforcement Technology
    
} bdi_cpu_caps_t;

// ===================================================================
// Memory & NUMA Topology
// ===================================================================

typedef struct {
    uint32_t numa_nodes;        // Number of NUMA nodes
    uint32_t cores_per_node;    // Cores per NUMA node
    uint32_t threads_per_core;  // SMT threads per core
    
    // Cache hierarchy
    uint32_t l1i_kb;           // L1 instruction cache size (KB)
    uint32_t l1d_kb;           // L1 data cache size (KB)  
    uint32_t l2_kb;            // L2 cache size (KB)
    uint32_t l3_kb;            // L3 cache size (KB)
    uint32_t cache_line_size;  // Cache line size (bytes)
    
    // Memory features
    bool hugepages_2mb;        // 2MB hugepage support
    bool hugepages_1gb;        // 1GB hugepage support
    bool memory_encryption;    // Memory encryption support
    uint64_t total_memory_mb;  // Total system memory (MB)
    
} bdi_memory_caps_t;

// ===================================================================
// Security & Virtualization
// ===================================================================

typedef struct {
    // === Intel Security Features ===
    bool intel_tdx;            // Intel Trust Domain Extensions
    bool intel_sgx;            // Intel Software Guard Extensions
    bool intel_cet;            // Control-flow Enforcement Technology
    bool intel_ibt;            // Indirect Branch Tracking
    bool intel_shstk;          // Shadow Stack
    
    // === AMD Security Features ===
    bool amd_sev;              // AMD Secure Encrypted Virtualization
    bool amd_sev_es;           // AMD SEV Encrypted State
    bool amd_sev_snp;          // AMD SEV Secure Nested Paging
    bool amd_sme;              // AMD Secure Memory Encryption
    
    // === ARM Security Features ===
    bool arm_pointer_auth;     // ARM Pointer Authentication
    bool arm_mte;              // ARM Memory Tagging Extension
    bool arm_bti;              // ARM Branch Target Identification
    
    // === Virtualization Support ===
    bool vt_x;                 // Intel VT-x
    bool vt_d;                 // Intel VT-d (IOMMU)
    bool amd_v;                // AMD-V
    bool amd_iommu;            // AMD IOMMU
    bool arm_virtualization;   // ARM Virtualization Extensions
    
} bdi_security_caps_t;

// ===================================================================
// Timer & Interrupt Capabilities
// ===================================================================

typedef struct {
    bool tsc_stable;           // TSC is stable across cores/P-states
    bool tsc_invariant;        // TSC is invariant
    bool hpet;                 // High Precision Event Timer
    bool apic;                 // Advanced Programmable Interrupt Controller
    bool x2apic;               // x2APIC support
    uint32_t apic_timer_freq;  // APIC timer frequency (Hz)
    
    // Interrupt capabilities
    uint32_t msix_vectors;     // Maximum MSI-X vectors
    bool interrupt_remapping;  // Interrupt remapping support
    
} bdi_timer_caps_t;

// ===================================================================
// GPU & Accelerator Capabilities
// ===================================================================

typedef struct {
    // === GPU APIs ===
    bool cuda;                 // NVIDIA CUDA support
    bool rocm;                 // AMD ROCm support  
    bool opencl;               // OpenCL support
    bool opengl;               // OpenGL support
    bool vulkan;               // Vulkan support
    bool directx;              // DirectX support (Windows)
    
    // === GPU Hardware ===
    uint32_t gpu_count;        // Number of discrete GPUs
    uint64_t gpu_memory_mb;    // Total GPU memory (MB)
    bool unified_memory;       // Unified CPU/GPU memory
    
    // === Other Accelerators ===
    bool tpu;                  // Tensor Processing Unit
    bool fpga;                 // FPGA accelerators
    bool dsp;                  // Digital Signal Processors
    
} bdi_gpu_caps_t;

// ===================================================================
// Storage & I/O Capabilities  
// ===================================================================

typedef struct {
    // === Storage Interfaces ===
    bool nvme;                 // NVMe support
    bool sata;                 // SATA support
    bool usb3;                 // USB 3.x support
    bool thunderbolt;          // Thunderbolt support
    
    // === Storage Features ===
    uint32_t nvme_queues;      // Maximum NVMe I/O queues
    bool storage_encryption;   // Hardware storage encryption
    bool trim_support;         // TRIM/UNMAP support
    
    // === Network Capabilities ===
    bool rdma;                 // RDMA support
    bool sr_iov;               // SR-IOV support
    bool network_offload;      // Hardware network offload
    uint32_t max_network_queues; // Maximum network queues
    
} bdi_io_caps_t;

// ===================================================================
// Complete Capability Structure
// ===================================================================

typedef struct bdi_caps {
    bdi_cpu_caps_t cpu;
    bdi_memory_caps_t memory;
    bdi_security_caps_t security;
    bdi_timer_caps_t timer;
    bdi_gpu_caps_t gpu;
    bdi_io_caps_t io;
    
    // === System Information ===
    char vendor_string[16];    // CPU vendor (e.g., "GenuineIntel")
    char brand_string[64];     // CPU brand string
    uint32_t family;           // CPU family
    uint32_t model;            // CPU model
    uint32_t stepping;         // CPU stepping
    uint32_t max_freq_mhz;     // Maximum CPU frequency (MHz)
    uint32_t base_freq_mhz;    // Base CPU frequency (MHz)
    
    // === Architecture ===
    enum {
        BDI_ARCH_X86_64,
        BDI_ARCH_ARM64,
        BDI_ARCH_RISCV64,
        BDI_ARCH_UNKNOWN
    } architecture;
    
} bdi_caps_t;

// ===================================================================
// Capability Detection Functions
// ===================================================================

// Main capability detection function - fills capability structure
void bdi_probe_caps(bdi_caps_t* caps);

// Generate stable hash of capabilities for caching
uint64_t bdi_caps_digest(const bdi_caps_t* caps);

// Check if specific capability is supported
bool bdi_has_capability(const bdi_caps_t* caps, const char* cap_name);

// Get human-readable capability summary
void bdi_caps_to_string(const bdi_caps_t* caps, char* buffer, size_t buffer_size);

// Architecture-specific detection functions
void bdi_probe_x86_caps(bdi_caps_t* caps);
void bdi_probe_arm_caps(bdi_caps_t* caps);
void bdi_probe_riscv_caps(bdi_caps_t* caps);

// Capability validation and testing
bool bdi_validate_caps(const bdi_caps_t* caps);
void bdi_benchmark_caps(const bdi_caps_t* caps);

#ifdef __cplusplus
}
#endif
