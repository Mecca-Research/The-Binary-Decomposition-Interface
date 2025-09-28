
// ===================================================================
// BDI Capability Graph Implementation
// Hardware detection and capability mapping
// ===================================================================

#include "capability.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __x86_64__
#include <cpuid.h>
#endif

// ===================================================================
// Architecture Detection
// ===================================================================

static int detect_architecture(void) {
#ifdef __x86_64__
    return BDI_ARCH_X86_64;
#elif defined(__aarch64__)
    return BDI_ARCH_ARM64;
#elif defined(__riscv) && (__riscv_xlen == 64)
    return BDI_ARCH_RISCV64;
#else
    return BDI_ARCH_UNKNOWN;
#endif
}

// ===================================================================
// Memory and NUMA Detection
// ===================================================================

static void probe_memory_caps(bdi_memory_caps_t* mem_caps) {
    // Initialize defaults
    memset(mem_caps, 0, sizeof(*mem_caps));
    
    // Get total system memory
    long pages = sysconf(_SC_PHYS_PAGES);
    long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
        mem_caps->total_memory_mb = (pages * page_size) / (1024 * 1024);
    }
    
    // Get cache line size
    long cache_line = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    if (cache_line > 0) {
        mem_caps->cache_line_size = (uint32_t)cache_line;
    } else {
        mem_caps->cache_line_size = 64; // Common default
    }
    
    // Try to detect NUMA topology
    FILE* numa_file = fopen("/sys/devices/system/node/possible", "r");
    if (numa_file) {
        char buffer[64];
        if (fgets(buffer, sizeof(buffer), numa_file)) {
            // Parse range like "0-3" or single number
            int start, end;
            if (sscanf(buffer, "%d-%d", &start, &end) == 2) {
                mem_caps->numa_nodes = end - start + 1;
            } else if (sscanf(buffer, "%d", &start) == 1) {
                mem_caps->numa_nodes = 1;
            }
        }
        fclose(numa_file);
    }
    
    if (mem_caps->numa_nodes == 0) {
        mem_caps->numa_nodes = 1; // Default to single node
    }
    
    // Check for hugepage support
    if (access("/sys/kernel/mm/hugepages/hugepages-2048kB", F_OK) == 0) {
        mem_caps->hugepages_2mb = true;
    }
    if (access("/sys/kernel/mm/hugepages/hugepages-1048576kB", F_OK) == 0) {
        mem_caps->hugepages_1gb = true;
    }
    
    // Estimate cache sizes (these would need more sophisticated detection)
    mem_caps->l1i_kb = 32;  // Typical L1I size
    mem_caps->l1d_kb = 32;  // Typical L1D size
    mem_caps->l2_kb = 256;  // Typical L2 size
    mem_caps->l3_kb = 8192; // Typical L3 size
}

// ===================================================================
// Timer and Interrupt Capabilities
// ===================================================================

static void probe_timer_caps(bdi_timer_caps_t* timer_caps) {
    memset(timer_caps, 0, sizeof(*timer_caps));
    
    // Check for HPET
    if (access("/sys/devices/system/clocksource/clocksource0/available_clocksource", F_OK) == 0) {
        FILE* cs_file = fopen("/sys/devices/system/clocksource/clocksource0/available_clocksource", "r");
        if (cs_file) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), cs_file)) {
                if (strstr(buffer, "hpet")) {
                    timer_caps->hpet = true;
                }
                if (strstr(buffer, "tsc")) {
                    timer_caps->tsc_stable = true;
                    timer_caps->tsc_invariant = true;
                }
            }
            fclose(cs_file);
        }
    }
    
    // Default interrupt capabilities
    timer_caps->apic = true;
    timer_caps->msix_vectors = 2048; // Common default
}

// ===================================================================
// GPU and Accelerator Detection
// ===================================================================

static void probe_gpu_caps(bdi_gpu_caps_t* gpu_caps) {
    memset(gpu_caps, 0, sizeof(*gpu_caps));
    
    // Check for NVIDIA CUDA
    if (access("/dev/nvidia0", F_OK) == 0) {
        gpu_caps->cuda = true;
        gpu_caps->gpu_count++;
    }
    
    // Check for AMD ROCm
    if (access("/dev/kfd", F_OK) == 0) {
        gpu_caps->rocm = true;
        gpu_caps->gpu_count++;
    }
    
    // Check for OpenCL
    if (access("/etc/OpenCL/vendors", F_OK) == 0) {
        gpu_caps->opencl = true;
    }
    
    // Estimate GPU memory (would need proper detection)
    if (gpu_caps->gpu_count > 0) {
        gpu_caps->gpu_memory_mb = 8192; // Default estimate
    }
}

// ===================================================================
// Storage and I/O Detection
// ===================================================================

static void probe_io_caps(bdi_io_caps_t* io_caps) {
    memset(io_caps, 0, sizeof(*io_caps));
    
    // Check for NVMe devices
    if (access("/sys/class/nvme", F_OK) == 0) {
        io_caps->nvme = true;
        io_caps->nvme_queues = 32; // Common default
    }
    
    // Check for SATA
    if (access("/sys/class/ata_port", F_OK) == 0) {
        io_caps->sata = true;
    }
    
    // Check for USB 3.0
    if (access("/sys/bus/usb/devices/usb3", F_OK) == 0) {
        io_caps->usb3 = true;
    }
    
    // Default network capabilities
    io_caps->max_network_queues = 16;
}

// ===================================================================
// Main Capability Detection Function
// ===================================================================

void bdi_probe_caps(bdi_caps_t* caps) {
    if (!caps) return;
    
    // Clear structure
    memset(caps, 0, sizeof(*caps));
    
    // Detect architecture
    caps->architecture = detect_architecture();
    
    // Architecture-specific CPU detection
    switch (caps->architecture) {
        case BDI_ARCH_X86_64:
            bdi_probe_x86_caps(caps);
            break;
        case BDI_ARCH_ARM64:
            bdi_probe_arm_caps(caps);
            break;
        case BDI_ARCH_RISCV64:
            bdi_probe_riscv_caps(caps);
            break;
        default:
            // Unknown architecture - set minimal capabilities
            strcpy(caps->vendor_string, "Unknown");
            break;
    }
    
    // Probe subsystems
    probe_memory_caps(&caps->memory);
    probe_timer_caps(&caps->timer);
    probe_gpu_caps(&caps->gpu);
    probe_io_caps(&caps->io);
    
    // Set reasonable defaults for missing information
    if (caps->max_freq_mhz == 0) {
        caps->max_freq_mhz = 3000; // 3 GHz default
    }
    if (caps->base_freq_mhz == 0) {
        caps->base_freq_mhz = 2400; // 2.4 GHz default
    }
}

// ===================================================================
// Capability Digest Generation
// ===================================================================

// Simple hash function for capability digest
static uint64_t hash_bytes(const void* data, size_t len, uint64_t seed) {
    const uint8_t* bytes = (const uint8_t*)data;
    uint64_t hash = seed;
    
    for (size_t i = 0; i < len; i++) {
        hash = hash * 1099511628211ULL; // FNV prime
        hash ^= bytes[i];
    }
    
    return hash;
}

uint64_t bdi_caps_digest(const bdi_caps_t* caps) {
    if (!caps) return 0;
    
    uint64_t digest = 0x811c9dc5; // FNV offset basis
    
    // Hash key capability fields
    digest = hash_bytes(&caps->cpu, sizeof(caps->cpu), digest);
    digest = hash_bytes(&caps->memory.numa_nodes, sizeof(caps->memory.numa_nodes), digest);
    digest = hash_bytes(&caps->memory.l3_kb, sizeof(caps->memory.l3_kb), digest);
    digest = hash_bytes(&caps->security, sizeof(caps->security), digest);
    digest = hash_bytes(&caps->architecture, sizeof(caps->architecture), digest);
    digest = hash_bytes(caps->vendor_string, strlen(caps->vendor_string), digest);
    digest = hash_bytes(&caps->family, sizeof(caps->family), digest);
    digest = hash_bytes(&caps->model, sizeof(caps->model), digest);
    
    return digest;
}

// ===================================================================
// Capability Query Functions
// ===================================================================

bool bdi_has_capability(const bdi_caps_t* caps, const char* cap_name) {
    if (!caps || !cap_name) return false;
    
    // CPU capabilities
    if (strcmp(cap_name, "avx2") == 0) return caps->cpu.avx2;
    if (strcmp(cap_name, "avx512f") == 0) return caps->cpu.avx512f;
    if (strcmp(cap_name, "amx_tile") == 0) return caps->cpu.amx_tile;
    if (strcmp(cap_name, "neon") == 0) return caps->cpu.neon;
    if (strcmp(cap_name, "sve") == 0) return caps->cpu.sve;
    if (strcmp(cap_name, "rvv") == 0) return caps->cpu.rvv;
    if (strcmp(cap_name, "aes_ni") == 0) return caps->cpu.aes_ni;
    if (strcmp(cap_name, "rdrand") == 0) return caps->cpu.rdrand;
    
    // Memory capabilities
    if (strcmp(cap_name, "hugepages_2mb") == 0) return caps->memory.hugepages_2mb;
    if (strcmp(cap_name, "hugepages_1gb") == 0) return caps->memory.hugepages_1gb;
    
    // Security capabilities
    if (strcmp(cap_name, "intel_sgx") == 0) return caps->security.intel_sgx;
    if (strcmp(cap_name, "amd_sev_snp") == 0) return caps->security.amd_sev_snp;
    if (strcmp(cap_name, "intel_tdx") == 0) return caps->security.intel_tdx;
    
    // GPU capabilities
    if (strcmp(cap_name, "cuda") == 0) return caps->gpu.cuda;
    if (strcmp(cap_name, "rocm") == 0) return caps->gpu.rocm;
    if (strcmp(cap_name, "opencl") == 0) return caps->gpu.opencl;
    
    // I/O capabilities
    if (strcmp(cap_name, "nvme") == 0) return caps->io.nvme;
    if (strcmp(cap_name, "rdma") == 0) return caps->io.rdma;
    
    return false;
}

// ===================================================================
// Capability String Representation
// ===================================================================

void bdi_caps_to_string(const bdi_caps_t* caps, char* buffer, size_t buffer_size) {
    if (!caps || !buffer || buffer_size == 0) return;
    
    int pos = 0;
    
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "BDI Capability Summary:\n");
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "  Architecture: %s\n",
                   caps->architecture == BDI_ARCH_X86_64 ? "x86_64" :
                   caps->architecture == BDI_ARCH_ARM64 ? "ARM64" :
                   caps->architecture == BDI_ARCH_RISCV64 ? "RISC-V64" : "Unknown");
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "  Vendor: %s\n", caps->vendor_string);
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "  Family/Model: %u/%u\n", caps->family, caps->model);
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "  Frequency: %u MHz (base: %u MHz)\n", 
                   caps->max_freq_mhz, caps->base_freq_mhz);
    
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "  SIMD: %s%s%s%s%s%s\n",
                   caps->cpu.sse2 ? "SSE2 " : "",
                   caps->cpu.avx ? "AVX " : "",
                   caps->cpu.avx2 ? "AVX2 " : "",
                   caps->cpu.avx512f ? "AVX512F " : "",
                   caps->cpu.neon ? "NEON " : "",
                   caps->cpu.sve ? "SVE " : "");
    
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "  Memory: %lu MB, %u NUMA nodes, L3: %u KB\n",
                   caps->memory.total_memory_mb, caps->memory.numa_nodes, caps->memory.l3_kb);
    
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "  GPU: %s%s%s (count: %u)\n",
                   caps->gpu.cuda ? "CUDA " : "",
                   caps->gpu.rocm ? "ROCm " : "",
                   caps->gpu.opencl ? "OpenCL " : "",
                   caps->gpu.gpu_count);
    
    pos += snprintf(buffer + pos, buffer_size - pos,
                   "  Security: %s%s%s\n",
                   caps->security.intel_sgx ? "SGX " : "",
                   caps->security.amd_sev_snp ? "SEV-SNP " : "",
                   caps->security.intel_tdx ? "TDX " : "");
}

// ===================================================================
// Validation and Testing
// ===================================================================

bool bdi_validate_caps(const bdi_caps_t* caps) {
    if (!caps) return false;
    
    // Basic sanity checks
    if (caps->architecture == BDI_ARCH_UNKNOWN) return false;
    if (caps->memory.numa_nodes == 0) return false;
    if (caps->memory.total_memory_mb == 0) return false;
    if (caps->memory.cache_line_size == 0) return false;
    
    // Architecture-specific validation
    switch (caps->architecture) {
        case BDI_ARCH_X86_64:
            // x86_64 should have at least SSE2
            if (!caps->cpu.sse2) return false;
            break;
        case BDI_ARCH_ARM64:
            // ARM64 should have NEON
            if (!caps->cpu.neon) return false;
            break;
        case BDI_ARCH_RISCV64:
            // RISC-V validation would go here
            break;
        default:
            return false;
    }
    
    return true;
}

// ===================================================================
// Stub implementations for other architectures
// ===================================================================

void bdi_probe_arm_caps(bdi_caps_t* caps) {
    // ARM capability detection would go here
    strcpy(caps->vendor_string, "ARM");
    caps->cpu.neon = true; // ARM64 always has NEON
    caps->architecture = BDI_ARCH_ARM64;
}

void bdi_probe_riscv_caps(bdi_caps_t* caps) {
    // RISC-V capability detection would go here
    strcpy(caps->vendor_string, "RISC-V");
    caps->architecture = BDI_ARCH_RISCV64;
}
