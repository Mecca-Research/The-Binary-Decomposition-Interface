# BDI vs. Linux Performance Comparison Report

## Overview
The requested comparison between the Binary Decomposition Interface (BDI) kernel and a conventional Linux system requires
collecting runtime measurements from both environments. The current container only provides the BDI source tree and does not
ship with a runnable BDI kernel image or a Linux baseline that exposes comparable workloads. As a result, no direct
measurements could be executed here. This report documents the constraints discovered and supplies a blueprint that can be
followed on a local workstation or lab machine to produce the desired results.

## Environment Constraints
- **Missing BDI Runtime Image:** The repository does not include prebuilt kernel binaries or a bootable image that can be
  executed inside the container. Building the monolithic `bdi_kernel` requires hardware access (or a virtualized environment)
  that is not present in the sandbox.
- **No Linux Baseline Inside Container:** While the container runs atop Linux, privileged access needed for low-level
  benchmarking (kernel scheduling, filesystem latencies, networking stacks) is unavailable. Running comparable workloads would
  need root-level configuration changes that are blocked by the environment sandboxing.
- **Resource Restrictions:** Kernel-level benchmarks typically require dedicated CPU cores, NUMA awareness, and access to
  physical or emulated devices. These capabilities are not exposed in the current QA setup.

## Recommended Methodology
To generate an apples-to-apples comparison, perform the following steps on a development machine where you control both BDI and
Linux environments:

1. **Prepare Test Hardware**
   - Use a machine with multi-core CPUs and sufficient RAM (≥16 GB) to exercise the scheduler and memory manager.
   - Ensure access to storage devices (NVMe/SATA) and network interfaces if I/O and networking comparisons are required.

2. **Build and Boot BDI Kernel**
   - Follow the build instructions in `bdi_kernel/README` (or project documentation) to produce a bootable image.
   - Boot the kernel either directly on bare-metal hardware or inside a hypervisor (QEMU/KVM, VMware, VirtualBox) with hardware
     acceleration enabled.

3. **Establish Linux Baseline**
   - Install a recent Linux distribution on the same hardware or identical VM configuration.
   - Disable background services that could skew measurements and align CPU governor, memory overcommit, and scheduler
     settings with the BDI environment.

4. **Select Workloads**
   - **Memory:** Use stress tests (e.g., custom HAM allocator benchmarks, `stress-ng`, or STREAM) to measure allocation latency,
     bandwidth, and NUMA locality.
   - **Scheduling:** Run multi-threaded workloads (e.g., `hackbench`, `sysbench`, or BDI scheduler microbenchmarks) to evaluate
     context-switch latency and throughput.
   - **Filesystem:** Benchmark read/write throughput and latency with `fio` or equivalent BDI storage tests.
   - **Networking:** Measure packet throughput and latency using `iperf3`, `netperf`, or BDI’s socket ring benchmarks.

5. **Instrumentation and Metrics**
   - Collect kernel traces or logs (BDI tracing facilities, Linux `perf`, `ftrace`, or `bpftrace`).
   - Record CPU utilization, scheduling latency, memory footprint, I/O operations per second, and network packet rates.

6. **Run Tests**
   - Execute each workload multiple times to account for variance.
   - Capture raw logs, summary statistics, and configuration parameters for reproducibility.

7. **Analyze Results**
   - Compare median and percentile latency metrics, throughput figures, and resource utilization.
   - Highlight areas where BDI’s architectural features (e.g., HAM, zero-copy IPC) provide advantages or require further tuning.

## Suggested Reporting Template
When the measurements are available, populate the following structure:

| Subsystem   | Workload / Tool | BDI Result (units) | Linux Result (units) | Delta | Notes |
|-------------|-----------------|--------------------|-----------------------|-------|-------|
| Memory      | STREAM Triad    |                    |                       |       |       |
| Scheduling  | hackbench       |                    |                       |       |       |
| Filesystem  | fio seq read    |                    |                       |       |       |
| Networking  | iperf3 TCP      |                    |                       |       |       |

Include configuration details (kernel versions, compiler flags, hardware specs) in an appendix to ensure the findings are
reproducible.

## Next Steps
- Provision a controlled benchmarking environment with access to both BDI and Linux kernels.
- Implement or enable the microbenchmark suites referenced in the repository (e.g., `tests/` harness) to gather subsystem-level
  metrics.
- Once data is collected, update this document with empirical results and share the final report via the preferred
  communication channel.

---
*Status: No performance tests executed inside the current container due to environment limitations.*
