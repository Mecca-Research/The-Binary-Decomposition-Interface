# BDI Kernel Completion Plan

## Document Overview

This document provides a comprehensive, phase-by-phase blueprint for completing the BDI (Binary Decomposition Interface) monolithic kernel. The plan is structured into 8 sequential phases, each building upon the previous to create a fully functional, production-ready kernel that embodies BDI's foundational computational substrate principles.

### Alignment with Project Vision

The BDI project aims to create a universal computational fabric capable of representing any computation. This kernel completion plan ensures that the core operating system layer provides:

- **Universal Computation Support**: Memory management and process abstractions that can represent arbitrary computational patterns
- **Deterministic Execution**: Scheduling and concurrency primitives that maintain predictable behavior
- **Secure Isolation**: Process boundaries and security mechanisms that protect computational integrity
- **Efficient Resource Management**: Storage, networking, and device abstractions optimized for BDI's computational model
- **Observable Behavior**: Comprehensive instrumentation for understanding system dynamics

### Roadmap Integration

This plan integrates with the broader BDI roadmap by:

1. Establishing the kernel foundation required for HAM (Hardware Abstraction Model) integration
2. Providing the process and memory primitives needed for BDI's computational substrate
3. Creating the system call interface for userland BDI applications
4. Enabling the observability infrastructure for BDI runtime analysis

---

## Phase 1: Memory & HAM Readiness

**Objective**: Establish robust memory management subsystems and prepare for Hardware Abstraction Model (HAM) integration.

### 1.1 Physical Memory Management

#### 1.1.1 Page Frame Allocator
- **Implementation Requirements**:
  - Buddy allocator for efficient page-level allocation (orders 0-11, supporting 4KB to 8MB allocations)
  - Free list management with per-order bitmap tracking
  - NUMA-aware allocation policies for multi-node systems
  - Memory zone support (DMA, Normal, HighMem)
  - Fragmentation tracking and defragmentation heuristics

- **Data Structures**:
  ```c
  struct page_frame {
      unsigned long flags;           // Page state flags
      atomic_t refcount;             // Reference counter
      struct list_head lru;          // LRU list linkage
      void *virtual;                 // Virtual address mapping
      unsigned int order;            // Buddy system order
  };
  
  struct zone {
      unsigned long free_pages;
      struct free_area free_area[MAX_ORDER];
      spinlock_t lock;
      unsigned long watermarks[NR_WMARK];
  };
  ```

- **Key Functions**:
  - `alloc_pages(gfp_mask, order)` - Allocate 2^order contiguous pages
  - `free_pages(page, order)` - Return pages to buddy system
  - `get_free_page(gfp_mask)` - Single page allocation
  - `zone_watermark_ok(zone, order)` - Check allocation feasibility

#### 1.1.2 SLAB/SLUB Allocator
- **Implementation Requirements**:
  - Object-level memory allocator for kernel structures
  - Per-CPU caching for lock-free fast paths
  - Size-class segregation (8B to 8KB objects)
  - Cache coloring to reduce cache conflicts
  - Memory leak detection and debugging support

- **Cache Management**:
  ```c
  struct kmem_cache {
      unsigned int object_size;
      unsigned int align;
      const char *name;
      void (*ctor)(void *);          // Constructor
      struct kmem_cache_cpu __percpu *cpu_slab;
      struct kmem_cache_node *node[MAX_NUMNODES];
  };
  ```

- **Key Functions**:
  - `kmem_cache_create(name, size, align, flags, ctor)` - Create cache
  - `kmem_cache_alloc(cache, gfp_mask)` - Allocate object
  - `kmem_cache_free(cache, object)` - Free object
  - `kmalloc(size, gfp_mask)` - General-purpose allocation
  - `kfree(ptr)` - General-purpose deallocation

#### 1.1.3 Memory Zones and NUMA
- **Zone Configuration**:
  - DMA zone: 0-16MB (ISA device compatibility)
  - Normal zone: 16MB-896MB (directly mapped kernel memory)
  - HighMem zone: >896MB (requires temporary mapping)

- **NUMA Support**:
  - Per-node memory statistics
  - Local allocation preference with fallback policies
  - Memory migration support for load balancing
  - Distance-based allocation decisions

### 1.2 Virtual Memory Management

#### 1.2.1 Page Table Management
- **Multi-Level Page Tables**:
  - 4-level paging: PGD → PUD → PMD → PTE
  - Support for 4KB, 2MB (huge), and 1GB (gigantic) pages
  - TLB invalidation strategies (single, range, full)
  - Page table entry flags: Present, RW, User, Accessed, Dirty, NX

- **Implementation**:
  ```c
  struct mm_struct {
      pgd_t *pgd;                    // Page global directory
      atomic_t mm_users;             // Address space users
      atomic_t mm_count;             // Primary reference count
      struct vm_area_struct *mmap;   // VMA list
      struct rb_root mm_rb;          // VMA red-black tree
      unsigned long total_vm;        // Total mapped pages
      unsigned long locked_vm;       // Locked pages (mlock)
      unsigned long start_code, end_code;
      unsigned long start_data, end_data;
      unsigned long start_brk, brk;  // Heap boundaries
      unsigned long start_stack;
  };
  ```

#### 1.2.2 Virtual Memory Areas (VMAs)
- **VMA Management**:
  - Red-black tree for O(log n) lookup
  - Linked list for sequential traversal
  - VMA merging and splitting logic
  - Copy-on-write (COW) support
  - Memory-mapped file support

- **VMA Structure**:
  ```c
  struct vm_area_struct {
      unsigned long vm_start;        // Start address
      unsigned long vm_end;          // End address (exclusive)
      struct mm_struct *vm_mm;       // Owning mm_struct
      pgprot_t vm_page_prot;         // Access permissions
      unsigned long vm_flags;        // Flags (VM_READ, VM_WRITE, etc.)
      struct rb_node vm_rb;          // RB tree node
      struct list_head vm_list;      // List linkage
      struct file *vm_file;          // Mapped file (if any)
      const struct vm_operations_struct *vm_ops;
  };
  ```

- **Key Operations**:
  - `find_vma(mm, addr)` - Locate VMA containing address
  - `insert_vm_struct(mm, vma)` - Add VMA to address space
  - `do_mmap(file, addr, len, prot, flags, offset)` - Create mapping
  - `do_munmap(mm, start, len)` - Remove mapping
  - `split_vma(mm, vma, addr)` - Split VMA at address

#### 1.2.3 Page Fault Handling
- **Fault Types**:
  - Minor fault: Page in memory but not mapped (e.g., COW)
  - Major fault: Page must be read from disk
  - Protection fault: Invalid access permissions
  - Non-present fault: Page not allocated

- **Fault Handler Flow**:
  ```c
  do_page_fault(regs, error_code, address) {
      1. Find VMA containing faulting address
      2. Check access permissions against VMA flags
      3. Determine fault type (present, write, user)
      4. Handle specific fault:
         - COW: Allocate new page, copy content
         - Demand paging: Allocate zero page
         - File-backed: Read from file
         - Swap: Read from swap device
      5. Update page table entry
      6. Flush TLB if necessary
  }
  ```

### 1.3 HAM Integration Preparation

#### 1.3.1 Memory Abstraction Layer
- **HAM Memory Interface**:
  - Abstract memory operations for HAM consumption
  - Provide memory region descriptors to HAM
  - Support for memory attribute queries (cacheable, device, etc.)
  - Memory barrier primitives for HAM synchronization

- **Interface Definition**:
  ```c
  struct ham_memory_ops {
      void* (*alloc_region)(size_t size, unsigned long flags);
      void (*free_region)(void *addr, size_t size);
      int (*map_region)(void *virt, phys_addr_t phys, size_t size, unsigned long attrs);
      void (*unmap_region)(void *virt, size_t size);
      void (*sync_region)(void *addr, size_t size, enum dma_data_direction dir);
  };
  ```

#### 1.3.2 DMA and Device Memory
- **DMA Coherent Allocation**:
  - Allocate memory accessible by both CPU and devices
  - Handle cache coherency requirements
  - Support for streaming and coherent DMA
  - IOMMU integration for device address translation

- **Device Memory Mapping**:
  - `ioremap()` for device register access
  - `ioremap_wc()` for write-combining (framebuffers)
  - `ioremap_cache()` for cacheable device memory
  - Proper unmapping and resource cleanup

#### 1.3.3 Memory Hotplug Support
- **Dynamic Memory Management**:
  - Add memory sections at runtime
  - Remove memory sections (with migration)
  - Memory block size configuration (typically 128MB)
  - Integration with ACPI/device tree for hotplug events

### 1.4 Memory Debugging and Instrumentation

#### 1.4.1 Memory Leak Detection
- **KMEMLEAK Integration**:
  - Track all kernel memory allocations
  - Periodic scanning for unreferenced objects
  - Reporting interface via debugfs
  - Configurable scan intervals and thresholds

#### 1.4.2 Memory Corruption Detection
- **KASAN (Kernel Address Sanitizer)**:
  - Detect out-of-bounds accesses
  - Use-after-free detection
  - Shadow memory for tracking allocation state
  - Compile-time instrumentation of memory accesses

- **SLUB Debug Features**:
  - Red-zoning to detect overflows
  - Poisoning to detect use-after-free
  - Allocation tracking for leak detection

#### 1.4.3 Memory Statistics and Profiling
- **Metrics Collection**:
  - Per-zone memory statistics
  - Per-process memory usage (RSS, VSZ, shared)
  - Slab cache statistics
  - Page fault counters (minor, major)
  - Memory pressure indicators

- **Interfaces**:
  - `/proc/meminfo` - System-wide memory information
  - `/proc/[pid]/status` - Per-process memory stats
  - `/proc/slabinfo` - Slab allocator statistics
  - `/sys/kernel/debug/kmemleak` - Leak detection reports

### 1.5 Phase 1 Success Criteria

- [ ] Buddy allocator passes stress tests (1M+ allocations/frees)
- [ ] SLUB allocator demonstrates <5% fragmentation under load
- [ ] Page table operations complete in <100ns (average)
- [ ] VMA operations scale to 10,000+ mappings per process
- [ ] Page fault handler resolves faults in <10μs (average)
- [ ] HAM memory interface successfully allocates and maps regions
- [ ] KMEMLEAK detects intentional leaks in test suite
- [ ] KASAN catches all injected memory corruption bugs
- [ ] Memory statistics accurately reflect system state
- [ ] NUMA allocation policies reduce remote access by >80%

### 1.6 Phase 1 Deliverables

1. **Code Modules**:
   - `mm/page_alloc.c` - Physical memory allocator
   - `mm/slab.c` or `mm/slub.c` - Object allocator
   - `mm/mmap.c` - VMA management
   - `mm/memory.c` - Page fault handling
   - `mm/ham_interface.c` - HAM memory abstraction

2. **Test Suites**:
   - Unit tests for allocators
   - Stress tests for concurrent allocation
   - Fault injection tests for error paths
   - Performance benchmarks

3. **Documentation**:
   - Memory subsystem architecture document
   - HAM integration guide
   - Debugging and profiling guide

---

## Phase 2: Scheduling & Concurrency Integration

**Objective**: Implement a robust, fair, and efficient process scheduler with comprehensive concurrency primitives.

### 2.1 Core Scheduler Implementation

#### 2.1.1 Completely Fair Scheduler (CFS)
- **Design Principles**:
  - Fair CPU time distribution based on virtual runtime
  - Red-black tree for O(log n) task selection
  - Per-CPU run queues for scalability
  - Load balancing across CPUs

- **Task Scheduling Entity**:
  ```c
  struct sched_entity {
      struct load_weight load;       // Task weight
      struct rb_node run_node;       // RB tree node
      u64 vruntime;                  // Virtual runtime
      u64 exec_start;                // Execution start time
      u64 sum_exec_runtime;          // Total execution time
      u64 prev_sum_exec_runtime;     // Previous total
      struct sched_entity *parent;   // Group scheduling
      struct cfs_rq *cfs_rq;         // CFS run queue
  };
  ```

- **CFS Run Queue**:
  ```c
  struct cfs_rq {
      struct load_weight load;       // Total load
      unsigned long nr_running;      // Number of tasks
      u64 min_vruntime;              // Minimum vruntime
      struct rb_root_cached tasks_timeline;  // RB tree
      struct sched_entity *curr;     // Currently running
      struct sched_entity *next;     // Next to run
  };
  ```

#### 2.1.2 Real-Time Scheduling
- **SCHED_FIFO (First-In-First-Out)**:
  - Fixed priority scheduling (1-99)
  - No time slicing within priority level
  - Preempts lower priority tasks immediately
  - Runs until completion, yield, or block

- **SCHED_RR (Round-Robin)**:
  - Fixed priority with time slicing
  - Time quantum typically 100ms
  - Round-robin among same-priority tasks
  - Preemption rules same as SCHED_FIFO

- **RT Run Queue**:
  ```c
  struct rt_rq {
      struct rt_prio_array active;   // Active priority queue
      unsigned int rt_nr_running;    // Number of RT tasks
      int highest_prio;              // Highest priority task
      u64 rt_runtime;                // RT bandwidth consumed
      u64 rt_time;                   // RT time tracking
  };
  ```

#### 2.1.3 Deadline Scheduling (SCHED_DEADLINE)
- **EDF (Earliest Deadline First)**:
  - Absolute deadline-based scheduling
  - Admission control for schedulability
  - Bandwidth reservation and enforcement
  - Preempts all other scheduling classes

- **Deadline Parameters**:
  ```c
  struct sched_dl_entity {
      struct rb_node rb_node;        // RB tree node
      u64 dl_runtime;                // Maximum runtime
      u64 dl_deadline;               // Relative deadline
      u64 dl_period;                 // Period
      u64 runtime;                   // Remaining runtime
      u64 deadline;                  // Absolute deadline
      unsigned int dl_throttled;     // Throttled flag
  };
  ```

### 2.2 Load Balancing

#### 2.2.1 Periodic Load Balancing
- **Balancing Domains**:
  - SMT (Simultaneous Multi-Threading) domain
  - MC (Multi-Core) domain
  - NUMA domain
  - Hierarchical balancing strategy

- **Load Calculation**:
  - Task weight based on nice value
  - CPU capacity consideration
  - Load averaging over time
  - Imbalance threshold detection

- **Migration Strategy**:
  ```c
  load_balance(cpu, idle) {
      1. Find busiest scheduling domain
      2. Calculate load imbalance
      3. Select tasks to migrate (prefer cache-cold)
      4. Check migration constraints (CPU affinity)
      5. Migrate tasks to current CPU
      6. Update load statistics
  }
  ```

#### 2.2.2 Active Load Balancing
- **Push Migration**:
  - Triggered when CPU becomes idle
  - Pull tasks from busiest CPU
  - Respects task affinity and priority

- **Pull Migration**:
  - Periodic check for imbalance
  - Move tasks from overloaded CPUs
  - Minimize cache thrashing

### 2.3 Concurrency Primitives

#### 2.3.1 Spinlocks
- **Basic Spinlock**:
  ```c
  typedef struct {
      atomic_t lock;
      unsigned int owner_cpu;
  } spinlock_t;
  
  void spin_lock(spinlock_t *lock);
  void spin_unlock(spinlock_t *lock);
  int spin_trylock(spinlock_t *lock);
  ```

- **Variants**:
  - `spin_lock_irq()` - Disable interrupts
  - `spin_lock_irqsave()` - Save and disable interrupts
  - `spin_lock_bh()` - Disable bottom halves
  - Read-write spinlocks for reader-writer scenarios

#### 2.3.2 Mutexes
- **Sleeping Locks**:
  ```c
  struct mutex {
      atomic_long_t owner;           // Owner task
      spinlock_t wait_lock;          // Protects wait list
      struct list_head wait_list;    // Waiting tasks
  };
  
  void mutex_lock(struct mutex *lock);
  void mutex_unlock(struct mutex *lock);
  int mutex_trylock(struct mutex *lock);
  ```

- **Optimistic Spinning**:
  - Spin briefly if owner is running
  - Reduces context switch overhead
  - Falls back to sleeping if owner not running

#### 2.3.3 Semaphores
- **Counting Semaphore**:
  ```c
  struct semaphore {
      raw_spinlock_t lock;
      unsigned int count;
      struct list_head wait_list;
  };
  
  void down(struct semaphore *sem);      // Decrement (may sleep)
  void up(struct semaphore *sem);        // Increment
  int down_trylock(struct semaphore *sem);
  ```

#### 2.3.4 Read-Write Locks
- **Reader-Writer Semaphore**:
  ```c
  struct rw_semaphore {
      atomic_long_t count;           // Reader count
      struct list_head wait_list;
      raw_spinlock_t wait_lock;
  };
  
  void down_read(struct rw_semaphore *sem);
  void down_write(struct rw_semaphore *sem);
  void up_read(struct rw_semaphore *sem);
  void up_write(struct rw_semaphore *sem);
  ```

#### 2.3.5 RCU (Read-Copy-Update)
- **Lock-Free Reads**:
  - Readers access data without locks
  - Writers create new versions
  - Grace period ensures safe reclamation

- **RCU API**:
  ```c
  rcu_read_lock();                   // Begin read-side critical section
  rcu_read_unlock();                 // End read-side critical section
  synchronize_rcu();                 // Wait for grace period
  call_rcu(head, func);              // Defer callback after grace period
  ```

### 2.4 Context Switching

#### 2.4.1 Task Context
- **Context Structure**:
  ```c
  struct task_struct {
      volatile long state;           // Task state
      void *stack;                   // Kernel stack
      struct mm_struct *mm;          // Memory descriptor
      struct mm_struct *active_mm;   // Active memory
      pid_t pid;                     // Process ID
      struct task_struct *parent;    // Parent process
      struct list_head children;     // Child processes
      struct files_struct *files;    // Open files
      struct fs_struct *fs;          // Filesystem info
      struct signal_struct *signal;  // Signal handlers
      struct sched_entity se;        // Scheduling entity
      unsigned int policy;           // Scheduling policy
      int prio;                      // Dynamic priority
      int static_prio;               // Static priority
      cpumask_t cpus_allowed;        // CPU affinity
  };
  ```

#### 2.4.2 Context Switch Mechanism
- **Switch Flow**:
  ```c
  context_switch(prev, next) {
      1. Save prev task's register state
      2. Switch memory context (mm_struct)
      3. Switch kernel stack
      4. Switch hardware context (FPU, debug registers)
      5. Restore next task's register state
      6. Update CPU-local task pointer
  }
  ```

- **Optimization Techniques**:
  - Lazy FPU state saving (only if used)
  - TLB flush avoidance for same mm
  - Cache-aware task placement

### 2.5 Interrupt and Bottom-Half Handling

#### 2.5.1 Interrupt Context
- **Top-Half (Hard IRQ)**:
  - Minimal processing in interrupt context
  - Acknowledge interrupt
  - Schedule bottom-half if needed
  - Fast execution (<10μs typical)

#### 2.5.2 Bottom-Half Mechanisms
- **Softirqs**:
  - Statically allocated (10 types)
  - Run in interrupt context with interrupts enabled
  - Per-CPU execution
  - Types: HI, TIMER, NET_TX, NET_RX, BLOCK, TASKLET, SCHED, HRTIMER, RCU

- **Tasklets**:
  - Dynamic softirq-based mechanism
  - Serialized per-tasklet (no concurrent execution)
  - Simpler API than softirqs

- **Work Queues**:
  - Run in process context (can sleep)
  - Backed by kernel threads
  - Suitable for longer operations
  - Per-CPU and unbound work queues

### 2.6 CPU Hotplug Support

#### 2.6.1 CPU Online/Offline
- **Hotplug Operations**:
  - Migrate tasks from offline CPU
  - Stop per-CPU threads
  - Flush per-CPU caches
  - Update scheduling domains
  - Notify subsystems of CPU state change

### 2.7 Phase 2 Success Criteria

- [ ] CFS scheduler achieves <1% unfairness under load
- [ ] Context switch latency <5μs (average)
- [ ] Load balancing maintains <10% imbalance
- [ ] RT tasks meet 99.9% of deadlines
- [ ] Spinlock contention <5% under high concurrency
- [ ] RCU grace periods complete in <10ms
- [ ] Interrupt latency <10μs (99th percentile)
- [ ] Softirq processing <100μs per batch
- [ ] CPU hotplug completes in <100ms
- [ ] Scheduler scales to 1024+ CPUs

### 2.8 Phase 2 Deliverables

1. **Code Modules**:
   - `kernel/sched/core.c` - Core scheduler
   - `kernel/sched/fair.c` - CFS implementation
   - `kernel/sched/rt.c` - Real-time scheduler
   - `kernel/sched/deadline.c` - Deadline scheduler
   - `kernel/locking/` - Locking primitives
   - `kernel/rcu/` - RCU implementation

2. **Test Suites**:
   - Scheduler fairness tests
   - Real-time deadline tests
   - Lock correctness tests (lockdep)
   - Concurrency stress tests

3. **Documentation**:
   - Scheduler design document
   - Locking guidelines
   - Real-time scheduling guide

---

## Phase 3: Process Lifecycle, Isolation & Security

**Objective**: Implement complete process management, security mechanisms, and isolation guarantees.

### 3.1 Process Creation and Termination

#### 3.1.1 Fork and Clone
- **Fork System Call**:
  ```c
  pid_t fork(void) {
      1. Allocate new task_struct
      2. Copy parent's mm_struct (COW)
      3. Copy file descriptors
      4. Copy signal handlers
      5. Allocate PID
      6. Add to scheduler run queue
      7. Return PID to parent, 0 to child
  }
  ```

- **Clone System Call**:
  - Flexible process/thread creation
  - Flags control resource sharing:
    - `CLONE_VM` - Share memory space (threads)
    - `CLONE_FILES` - Share file descriptor table
    - `CLONE_FS` - Share filesystem information
    - `CLONE_SIGHAND` - Share signal handlers
    - `CLONE_THREAD` - Create thread in same group
    - `CLONE_NEWNS` - New mount namespace
    - `CLONE_NEWPID` - New PID namespace

#### 3.1.2 Exec System Call
- **Program Execution**:
  ```c
  execve(pathname, argv, envp) {
      1. Load executable file
      2. Verify format (ELF, script, etc.)
      3. Flush old address space
      4. Create new mm_struct
      5. Map executable segments
      6. Map shared libraries
      7. Setup stack with argv/envp
      8. Set instruction pointer to entry point
      9. Return to userspace (no return to caller)
  }
  ```

- **ELF Loading**:
  - Parse ELF headers
  - Map PT_LOAD segments
  - Handle dynamic linking (ld.so)
  - Setup auxiliary vector (AT_PHDR, AT_ENTRY, etc.)

#### 3.1.3 Process Termination
- **Exit System Call**:
  ```c
  exit(status) {
      1. Set task state to TASK_DEAD
      2. Release memory (mm_struct)
      3. Close file descriptors
      4. Notify parent (SIGCHLD)
      5. Reparent children to init
      6. Release task_struct (after parent wait)
  }
  ```

- **Zombie and Reaping**:
  - Zombie state until parent calls wait()
  - Minimal resources retained (PID, exit status)
  - Automatic reaping by init if parent dies

### 3.2 Process Namespaces

#### 3.2.1 Namespace Types
- **PID Namespace**:
  - Isolated PID space
  - PID 1 is namespace init
  - Hierarchical structure
  - Nested namespaces supported

- **Mount Namespace**:
  - Private filesystem view
  - Independent mount points
  - Propagation types (shared, slave, private)

- **Network Namespace**:
  - Isolated network stack
  - Separate interfaces, routing tables
  - Independent firewall rules

- **UTS Namespace**:
  - Isolated hostname and domain name
  - Per-namespace system identification

- **IPC Namespace**:
  - Isolated System V IPC objects
  - Separate message queues, semaphores, shared memory

- **User Namespace**:
  - UID/GID mapping
  - Capability isolation
  - Nested user namespaces

- **Cgroup Namespace**:
  - Virtualized cgroup view
  - Relative cgroup paths

#### 3.2.2 Namespace Operations
- **Creation**:
  ```c
  unshare(CLONE_NEWPID | CLONE_NEWNS);  // Create new namespaces
  setns(fd, CLONE_NEWNET);              // Join existing namespace
  ```

- **Namespace Structure**:
  ```c
  struct nsproxy {
      atomic_t count;
      struct uts_namespace *uts_ns;
      struct ipc_namespace *ipc_ns;
      struct mnt_namespace *mnt_ns;
      struct pid_namespace *pid_ns;
      struct net *net_ns;
      struct cgroup_namespace *cgroup_ns;
  };
  ```

### 3.3 Control Groups (Cgroups)

#### 3.3.1 Resource Controllers
- **CPU Controller**:
  - CPU time limits (cpu.cfs_quota_us)
  - CPU shares (cpu.shares)
  - Real-time bandwidth (cpu.rt_runtime_us)

- **Memory Controller**:
  - Memory limits (memory.limit_in_bytes)
  - Swap limits (memory.memsw.limit_in_bytes)
  - OOM control (memory.oom_control)
  - Memory pressure notifications

- **Block I/O Controller**:
  - I/O bandwidth limits (blkio.throttle.read_bps_device)
  - I/O weight (blkio.weight)
  - Per-device controls

- **Network Controller**:
  - Network priority (net_prio.ifpriomap)
  - Network class ID (net_cls.classid)

#### 3.3.2 Cgroup Hierarchy
- **Unified Hierarchy (cgroup v2)**:
  - Single hierarchy for all controllers
  - No-internal-process rule
  - Delegation model for containers

- **Cgroup Operations**:
  ```c
  mkdir /sys/fs/cgroup/mygroup          // Create cgroup
  echo $$ > /sys/fs/cgroup/mygroup/cgroup.procs  // Add process
  echo "100M" > /sys/fs/cgroup/mygroup/memory.max  // Set limit
  ```

### 3.4 Capabilities and Privileges

#### 3.4.1 Linux Capabilities
- **Capability System**:
  - Fine-grained privilege division
  - 41 capabilities (CAP_CHOWN, CAP_NET_ADMIN, etc.)
  - Per-thread capability sets:
    - Permitted: Maximum capabilities
    - Effective: Currently active capabilities
    - Inheritable: Capabilities preserved across exec
    - Bounding: Limit on capabilities
    - Ambient: Capabilities for non-privileged programs

- **Capability Operations**:
  ```c
  capget(header, data);                 // Get capabilities
  capset(header, data);                 // Set capabilities
  prctl(PR_CAP_AMBIENT, PR_CAP_AMBIENT_RAISE, CAP_NET_ADMIN);
  ```

#### 3.4.2 Privilege Escalation Prevention
- **Secure Bits**:
  - SECBIT_NOROOT: Disable root privilege
  - SECBIT_NO_SETUID_FIXUP: Prevent setuid fixup
  - SECBIT_KEEP_CAPS: Keep capabilities on UID change

### 3.5 Security Modules

#### 3.5.1 LSM (Linux Security Modules) Framework
- **Hook Architecture**:
  - Security hooks at critical kernel points
  - Pluggable security modules
  - Stacking support for multiple modules

- **LSM Hooks**:
  ```c
  struct security_operations {
      int (*task_create)(unsigned long clone_flags);
      int (*task_alloc)(struct task_struct *task);
      void (*task_free)(struct task_struct *task);
      int (*file_permission)(struct file *file, int mask);
      int (*inode_permission)(struct inode *inode, int mask);
      // ... hundreds more hooks
  };
  ```

#### 3.5.2 SELinux Integration
- **Mandatory Access Control (MAC)**:
  - Type enforcement (TE)
  - Role-based access control (RBAC)
  - Multi-level security (MLS)

- **Security Contexts**:
  - Format: user:role:type:level
  - Applied to processes, files, sockets, etc.
  - Policy-based access decisions

- **SELinux Modes**:
  - Enforcing: Deny and log violations
  - Permissive: Log but allow violations
  - Disabled: No SELinux enforcement

#### 3.5.3 AppArmor Integration
- **Path-Based MAC**:
  - Simpler than SELinux
  - Profile-based confinement
  - Path-based rules (easier to understand)

- **Profile Example**:
  ```
  /usr/bin/foo {
      /etc/foo.conf r,
      /var/log/foo.log w,
      /tmp/** rw,
  }
  ```

### 3.6 Seccomp (Secure Computing)

#### 3.6.1 Seccomp Modes
- **Strict Mode**:
  - Only read, write, exit, sigreturn allowed
  - Extremely restrictive

- **Filter Mode (BPF)**:
  - Berkeley Packet Filter for syscall filtering
  - Flexible allow/deny rules
  - Return values: ALLOW, KILL, TRAP, ERRNO, TRACE

#### 3.6.2 Seccomp Filter
- **BPF Program**:
  ```c
  struct sock_filter filter[] = {
      BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(struct seccomp_data, nr)),
      BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_write, 0, 1),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
      BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_KILL),
  };
  
  prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &prog);
  ```

### 3.7 Process Credentials

#### 3.7.1 UID/GID Management
- **Credential Structure**:
  ```c
  struct cred {
      atomic_t usage;
      uid_t uid;                         // Real UID
      gid_t gid;                         // Real GID
      uid_t suid;                        // Saved UID
      gid_t sgid;                        // Saved GID
      uid_t euid;                        // Effective UID
      gid_t egid;                        // Effective GID
      uid_t fsuid;                       // Filesystem UID
      gid_t fsgid;                       // Filesystem GID
      kernel_cap_t cap_inheritable;
      kernel_cap_t cap_permitted;
      kernel_cap_t cap_effective;
      kernel_cap_t cap_bset;
      kernel_cap_t cap_ambient;
  };
  ```

- **Credential Operations**:
  - `setuid()`, `setgid()` - Set user/group ID
  - `seteuid()`, `setegid()` - Set effective IDs
  - `setreuid()`, `setregid()` - Set real and effective IDs
  - `setresuid()`, `setresgid()` - Set real, effective, and saved IDs

### 3.8 Signal Handling

#### 3.8.1 Signal Delivery
- **Signal Types**:
  - Standard signals (SIGTERM, SIGKILL, etc.)
  - Real-time signals (SIGRTMIN to SIGRTMAX)
  - Synchronous (SIGSEGV, SIGFPE) vs. asynchronous (SIGTERM)

- **Signal Delivery Mechanism**:
  ```c
  send_signal(sig, info, task) {
      1. Check permissions (same UID or CAP_KILL)
      2. Add signal to pending set
      3. Wake up task if sleeping
      4. Deliver signal on return to userspace
  }
  ```

#### 3.8.2 Signal Handlers
- **Handler Registration**:
  ```c
  struct sigaction {
      void (*sa_handler)(int);           // Handler function
      void (*sa_sigaction)(int, siginfo_t *, void *);
      sigset_t sa_mask;                  // Blocked signals during handler
      int sa_flags;                      // SA_RESTART, SA_SIGINFO, etc.
  };
  
  sigaction(SIGTERM, &act, NULL);
  ```

- **Signal Masks**:
  - Blocked signals (sigprocmask)
  - Pending signals (sigpending)
  - Signal inheritance across fork/exec

### 3.9 Phase 3 Success Criteria

- [ ] Fork/clone completes in <100μs
- [ ] Exec loads ELF binaries in <1ms
- [ ] Process termination and reaping in <50μs
- [ ] Namespace creation overhead <10μs
- [ ] Cgroup resource limits enforced within 1 scheduling period
- [ ] Capability checks add <100ns overhead
- [ ] SELinux policy decisions in <1μs
- [ ] Seccomp filter evaluation in <500ns
- [ ] Signal delivery latency <5μs
- [ ] Process isolation verified by security audit

### 3.10 Phase 3 Deliverables

1. **Code Modules**:
   - `kernel/fork.c` - Process creation
   - `fs/exec.c` - Program execution
   - `kernel/exit.c` - Process termination
   - `kernel/nsproxy.c` - Namespace management
   - `kernel/cgroup/` - Cgroup implementation
   - `kernel/capability.c` - Capability system
   - `security/` - LSM framework and modules

2. **Test Suites**:
   - Process lifecycle tests
   - Namespace isolation tests
   - Cgroup resource limit tests
   - Security policy tests
   - Capability tests

3. **Documentation**:
   - Process management guide
   - Namespace and container guide
   - Security architecture document
   - Capability reference

---

## Phase 4: Storage, Filesystems & VFS

**Objective**: Implement a robust Virtual Filesystem Switch (VFS) layer with support for multiple filesystems and storage devices.

### 4.1 Virtual Filesystem Switch (VFS)

#### 4.1.1 VFS Architecture
- **Core Abstractions**:
  - Superblock: Filesystem metadata
  - Inode: File metadata
  - Dentry: Directory entry (name to inode mapping)
  - File: Open file instance

- **VFS Objects**:
  ```c
  struct super_block {
      struct list_head s_list;           // List of superblocks
      dev_t s_dev;                       // Device identifier
      unsigned long s_blocksize;         // Block size
      loff_t s_maxbytes;                 // Max file size
      struct file_system_type *s_type;
      const struct super_operations *s_op;
      struct dentry *s_root;             // Root dentry
      struct list_head s_inodes;         // Inode list
      struct list_head s_dirty;          // Dirty inodes
  };
  
  struct inode {
      umode_t i_mode;                    // File type and permissions
      uid_t i_uid;                       // Owner UID
      gid_t i_gid;                       // Owner GID
      loff_t i_size;                     // File size
      struct timespec i_atime;           // Access time
      struct timespec i_mtime;           // Modification time
      struct timespec i_ctime;           // Change time
      unsigned long i_ino;               // Inode number
      atomic_t i_count;                  // Reference count
      const struct inode_operations *i_op;
      const struct file_operations *i_fop;
      struct address_space *i_mapping;   // Page cache
      union {
          struct pipe_inode_info *i_pipe;
          struct block_device *i_bdev;
          struct cdev *i_cdev;
      };
  };
  
  struct dentry {
      struct qstr d_name;                // Name
      struct dentry *d_parent;           // Parent dentry
      struct inode *d_inode;             // Associated inode
      struct list_head d_child;          // Child dentries
      struct list_head d_subdirs;        // Subdirectories
      const struct dentry_operations *d_op;
      struct super_block *d_sb;          // Superblock
      unsigned long d_time;              // Revalidation time
      void *d_fsdata;                    // Filesystem-specific data
  };
  
  struct file {
      struct path f_path;                // Dentry and vfsmount
      const struct file_operations *f_op;
      atomic_long_t f_count;             // Reference count
      unsigned int f_flags;              // O_RDONLY, O_WRONLY, etc.
      fmode_t f_mode;                    // FMODE_READ, FMODE_WRITE
      loff_t f_pos;                      // File position
      struct fown_struct f_owner;        // Owner for signals
      const struct cred *f_cred;         // Credentials
      void *private_data;                // Filesystem-specific data
  };
  ```

#### 4.1.2 VFS Operations
- **Superblock Operations**:
  ```c
  struct super_operations {
      struct inode *(*alloc_inode)(struct super_block *sb);
      void (*destroy_inode)(struct inode *);
      void (*dirty_inode)(struct inode *, int flags);
      int (*write_inode)(struct inode *, struct writeback_control *);
      void (*drop_inode)(struct inode *);
      void (*evict_inode)(struct inode *);
      void (*put_super)(struct super_block *);
      int (*sync_fs)(struct super_block *, int wait);
      int (*statfs)(struct dentry *, struct kstatfs *);
  };
  ```

- **Inode Operations**:
  ```c
  struct inode_operations {
      int (*create)(struct inode *, struct dentry *, umode_t, bool);
      struct dentry *(*lookup)(struct inode *, struct dentry *, unsigned int);
      int (*link)(struct dentry *, struct inode *, struct dentry *);
      int (*unlink)(struct inode *, struct dentry *);
      int (*symlink)(struct inode *, struct dentry *, const char *);
      int (*mkdir)(struct inode *, struct dentry *, umode_t);
      int (*rmdir)(struct inode *, struct dentry *);
      int (*rename)(struct inode *, struct dentry *, struct inode *, struct dentry *);
      int (*setattr)(struct dentry *, struct iattr *);
      int (*getattr)(struct vfsmount *, struct dentry *, struct kstat *);
  };
  ```

- **File Operations**:
  ```c
  struct file_operations {
      loff_t (*llseek)(struct file *, loff_t, int);
      ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
      ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
      int (*open)(struct inode *, struct file *);
      int (*release)(struct inode *, struct file *);
      int (*fsync)(struct file *, loff_t, loff_t, int);
      int (*mmap)(struct file *, struct vm_area_struct *);
      unsigned int (*poll)(struct file *, struct poll_table_struct *);
      long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
  };
  ```

### 4.2 Page Cache

#### 4.2.1 Address Space Operations
- **Page Cache Management**:
  ```c
  struct address_space {
      struct inode *host;                // Owner inode
      struct radix_tree_root page_tree;  // Page cache tree
      spinlock_t tree_lock;              // Protects page_tree
      unsigned long nrpages;             // Number of pages
      pgoff_t writeback_index;           // Writeback start
      const struct address_space_operations *a_ops;
  };
  
  struct address_space_operations {
      int (*writepage)(struct page *, struct writeback_control *);
      int (*readpage)(struct file *, struct page *);
      int (*writepages)(struct address_space *, struct writeback_control *);
      int (*readpages)(struct file *, struct address_space *, struct list_head *, unsigned);
      int (*write_begin)(struct file *, struct address_space *, loff_t, unsigned, unsigned, struct page **, void **);
      int (*write_end)(struct file *, struct address_space *, loff_t, unsigned, unsigned, struct page *, void *);
      sector_t (*bmap)(struct address_space *, sector_t);
      void (*invalidatepage)(struct page *, unsigned int, unsigned int);
      int (*releasepage)(struct page *, gfp_t);
  };
  ```

#### 4.2.2 Read-Ahead
- **Sequential Read Optimization**:
  - Detect sequential access patterns
  - Prefetch pages ahead of current position
  - Adaptive window sizing
  - Thrashing detection and prevention

### 4.3 Block Layer

#### 4.3.1 Block Device Abstraction
- **Block Device Structure**:
  ```c
  struct block_device {
      dev_t bd_dev;                      // Device number
      struct inode *bd_inode;            // Block device inode
      struct super_block *bd_super;      // Mounted filesystem
      int bd_openers;                    // Open count
      struct mutex bd_mutex;             // Serialization
      struct gendisk *bd_disk;           // Generic disk
      struct request_queue *bd_queue;    // Request queue
      struct list_head bd_list;          // List of block devices
  };
  
  struct gendisk {
      int major;                         // Major number
      int first_minor;                   // First minor number
      int minors;                        // Number of minors
      char disk_name[DISK_NAME_LEN];     // Device name
      struct disk_part_tbl *part_tbl;    // Partition table
      struct request_queue *queue;       // Request queue
      const struct block_device_operations *fops;
      sector_t capacity;                 // Disk capacity
  };
  ```

#### 4.3.2 I/O Scheduler
- **Request Queue**:
  ```c
  struct request_queue {
      struct list_head queue_head;       // Request list
      struct request *last_merge;        // Last merged request
      struct elevator_queue *elevator;   // I/O scheduler
      struct request_list rq;            // Request allocation
      make_request_fn *make_request_fn;  // Request creation
      spinlock_t queue_lock;             // Queue lock
      unsigned long nr_requests;         // Number of requests
  };
  ```

- **I/O Schedulers**:
  - **Noop**: Simple FIFO, no reordering
  - **Deadline**: Deadline-based scheduling to prevent starvation
  - **CFQ (Completely Fair Queuing)**: Fair bandwidth allocation per process
  - **BFQ (Budget Fair Queuing)**: Low-latency, fair scheduling

#### 4.3.3 Bio (Block I/O) Layer
- **Bio Structure**:
  ```c
  struct bio {
      struct block_device *bi_bdev;      // Target device
      sector_t bi_sector;                // First sector
      unsigned int bi_size;              // I/O size
      unsigned short bi_vcnt;            // Number of segments
      unsigned short bi_idx;             // Current segment
      struct bio_vec *bi_io_vec;         // Segment array
      bio_end_io_t *bi_end_io;           // Completion callback
      void *bi_private;                  // Private data
  };
  
  struct bio_vec {
      struct page *bv_page;              // Page
      unsigned int bv_len;               // Length
      unsigned int bv_offset;            // Offset in page
  };
  ```

### 4.4 Filesystem Implementations

#### 4.4.1 Ext4 Filesystem
- **Features**:
  - Extent-based allocation (reduces fragmentation)
  - Delayed allocation (better block allocation)
  - Journal checksumming (data integrity)
  - Large file support (up to 16TB)
  - Large filesystem support (up to 1EB)
  - Online defragmentation
  - Snapshot support (experimental)

- **On-Disk Structures**:
  ```c
  struct ext4_super_block {
      __le32 s_inodes_count;             // Total inodes
      __le32 s_blocks_count_lo;          // Total blocks
      __le32 s_free_blocks_count_lo;     // Free blocks
      __le32 s_free_inodes_count;        // Free inodes
      __le32 s_first_data_block;         // First data block
      __le32 s_log_block_size;           // Block size
      __le32 s_blocks_per_group;         // Blocks per group
      __le32 s_inodes_per_group;         // Inodes per group
      // ... many more fields
  };
  
  struct ext4_inode {
      __le16 i_mode;                     // File mode
      __le16 i_uid;                      // Owner UID
      __le32 i_size_lo;                  // Size (low 32 bits)
      __le32 i_atime;                    // Access time
      __le32 i_ctime;                    // Change time
      __le32 i_mtime;                    // Modification time
      __le32 i_blocks_lo;                // Block count
      __le32 i_flags;                    // File flags
      struct ext4_extent_header i_block[EXT4_N_BLOCKS];
      // ... more fields
  };
  ```

- **Journaling**:
  - Journal modes: data=ordered, data=writeback, data=journal
  - Commit interval (default 5 seconds)
  - Barrier support for write ordering
  - Recovery on mount after crash

#### 4.4.2 Tmpfs (RAM-based Filesystem)
- **Features**:
  - Resides entirely in RAM
  - Swappable to disk if memory pressure
  - Dynamic size (grows/shrinks as needed)
  - Fast access (no disk I/O)

- **Use Cases**:
  - `/tmp` directory
  - `/dev/shm` for shared memory
  - Temporary build directories

#### 4.4.3 Procfs (Process Filesystem)
- **Virtual Filesystem**:
  - Exposes kernel and process information
  - No backing storage
  - Dynamic content generation

- **Key Directories**:
  - `/proc/[pid]/` - Per-process information
  - `/proc/sys/` - Kernel parameters (sysctl)
  - `/proc/meminfo` - Memory statistics
  - `/proc/cpuinfo` - CPU information

#### 4.4.4 Sysfs (System Filesystem)
- **Device and Driver Information**:
  - `/sys/devices/` - Device hierarchy
  - `/sys/bus/` - Bus types
  - `/sys/class/` - Device classes
  - `/sys/module/` - Loaded modules

### 4.5 Mount and Namespace

#### 4.5.1 Mount Operations
- **Mount System Call**:
  ```c
  mount(source, target, fstype, flags, data) {
      1. Lookup target path
      2. Allocate vfsmount structure
      3. Call filesystem's mount method
      4. Attach to mount tree
      5. Update namespace
  }
  ```

- **Mount Flags**:
  - MS_RDONLY: Read-only mount
  - MS_NOSUID: Ignore setuid/setgid bits
  - MS_NODEV: Disallow device files
  - MS_NOEXEC: Disallow program execution
  - MS_BIND: Bind mount

#### 4.5.2 Mount Namespaces
- **Private Mount Trees**:
  - Each namespace has independent mount tree
  - Propagation types: shared, slave, private, unbindable
  - Pivot root for container isolation

### 4.6 File Locking

#### 4.6.1 Advisory Locks
- **flock() System Call**:
  - Whole-file locking
  - Shared (read) and exclusive (write) locks
  - Non-blocking option

- **fcntl() Locks (POSIX)**:
  - Byte-range locking
  - Record locking
  - Mandatory locking (if enabled)

#### 4.6.2 Mandatory Locks
- **Enforcement**:
  - Enabled per-file with special permissions
  - Blocks conflicting I/O operations
  - Rarely used (advisory preferred)

### 4.7 Direct I/O and Async I/O

#### 4.7.1 Direct I/O
- **O_DIRECT Flag**:
  - Bypass page cache
  - Direct disk access
  - Alignment requirements (sector-aligned)
  - Use case: Databases with own caching

#### 4.7.2 Asynchronous I/O (AIO)
- **io_submit() System Call**:
  - Submit I/O requests without blocking
  - Completion notification via io_getevents()
  - Multiple outstanding requests

### 4.8 Phase 4 Success Criteria

- [ ] VFS layer supports 5+ filesystem types
- [ ] File open/close latency <10μs
- [ ] Read/write throughput >1GB/s (sequential)
- [ ] Page cache hit rate >90% for typical workloads
- [ ] Ext4 journal recovery completes in <5 seconds
- [ ] Block layer merges >80% of sequential requests
- [ ] I/O scheduler maintains <10ms latency (99th percentile)
- [ ] Mount/unmount operations in <100ms
- [ ] File locking correctness verified by stress tests
- [ ] Direct I/O achieves >80% of raw device bandwidth

### 4.9 Phase 4 Deliverables

1. **Code Modules**:
   - `fs/` - VFS core
   - `fs/ext4/` - Ext4 filesystem
   - `mm/filemap.c` - Page cache
   - `block/` - Block layer
   - `fs/proc/` - Procfs
   - `fs/sysfs/` - Sysfs

2. **Test Suites**:
   - VFS operation tests
   - Filesystem stress tests
   - Page cache tests
   - Block layer tests
   - File locking tests

3. **Documentation**:
   - VFS architecture document
   - Filesystem implementation guide
   - Block layer design document

---

## Phase 5: IPC & Networking

**Objective**: Implement comprehensive inter-process communication mechanisms and a full networking stack.

### 5.1 Inter-Process Communication (IPC)

#### 5.1.1 Pipes and FIFOs
- **Anonymous Pipes**:
  ```c
  int pipe(int pipefd[2]) {
      1. Allocate pipe buffer (typically 64KB)
      2. Create two file descriptors (read and write)
      3. Return file descriptors to caller
  }
  ```

- **Named Pipes (FIFOs)**:
  - Persistent in filesystem
  - Multiple readers/writers
  - Created with mkfifo()

- **Pipe Buffer Management**:
  - Circular buffer implementation
  - Blocking/non-blocking modes
  - Atomic writes up to PIPE_BUF (4KB)

#### 5.1.2 System V IPC

- **Message Queues**:
  ```c
  struct msg_queue {
      struct kern_ipc_perm q_perm;       // Permissions
      time_t q_stime;                    // Last send time
      time_t q_rtime;                    // Last receive time
      unsigned long q_cbytes;            // Current bytes
      unsigned long q_qnum;              // Number of messages
      unsigned long q_qbytes;            // Max bytes
      pid_t q_lspid;                     // Last send PID
      pid_t q_lrpid;                     // Last receive PID
      struct list_head q_messages;       // Message list
      struct list_head q_receivers;      // Waiting receivers
      struct list_head q_senders;        // Waiting senders
  };
  
  // Operations
  msgget(key, msgflg);                   // Create/access queue
  msgsnd(msqid, msgp, msgsz, msgflg);    // Send message
  msgrcv(msqid, msgp, msgsz, msgtyp, msgflg);  // Receive message
  msgctl(msqid, cmd, buf);               // Control operations
  ```

- **Semaphores**:
  ```c
  struct sem_array {
      struct kern_ipc_perm sem_perm;     // Permissions
      time_t sem_otime;                  // Last operation time
      time_t sem_ctime;                  // Last change time
      unsigned long sem_nsems;           // Number of semaphores
      struct sem *sem_base;              // Semaphore array
      struct list_head pending_alter;    // Pending operations
      struct list_head pending_const;    // Pending reads
  };
  
  // Operations
  semget(key, nsems, semflg);            // Create/access semaphore set
  semop(semid, sops, nsops);             // Perform operations
  semctl(semid, semnum, cmd, arg);       // Control operations
  ```

- **Shared Memory**:
  ```c
  struct shmid_kernel {
      struct kern_ipc_perm shm_perm;     // Permissions
      struct file *shm_file;             // Backing file
      unsigned long shm_nattch;          // Number of attachments
      unsigned long shm_segsz;           // Segment size
      time_t shm_atim;                   // Last attach time
      time_t shm_dtim;                   // Last detach time
      time_t shm_ctim;                   // Last change time
      pid_t shm_cprid;                   // Creator PID
      pid_t shm_lprid;                   // Last operation PID
  };
  
  // Operations
  shmget(key, size, shmflg);             // Create/access segment
  shmat(shmid, shmaddr, shmflg);         // Attach segment
  shmdt(shmaddr);                        // Detach segment
  shmctl(shmid, cmd, buf);               // Control operations
  ```

#### 5.1.3 POSIX IPC

- **POSIX Message Queues**:
  - Priority-based message delivery
  - Notification mechanisms (signals, threads)
  - Mounted at `/dev/mqueue`

- **POSIX Semaphores**:
  - Named and unnamed variants
  - Simpler API than System V
  - Better integration with threads

- **POSIX Shared Memory**:
  - File-based interface (shm_open)
  - Memory-mapped files
  - Easier cleanup than System V

#### 5.1.4 Unix Domain Sockets
- **Socket Types**:
  - SOCK_STREAM: Connection-oriented (like TCP)
  - SOCK_DGRAM: Connectionless (like UDP)
  - SOCK_SEQPACKET: Sequenced, reliable packets

- **Advantages**:
  - Faster than network sockets (no protocol overhead)
  - Credential passing (SCM_CREDENTIALS)
  - File descriptor passing (SCM_RIGHTS)

### 5.2 Networking Stack

#### 5.2.1 Socket Layer
- **Socket Structure**:
  ```c
  struct socket {
      socket_state state;                // Socket state
      short type;                        // SOCK_STREAM, SOCK_DGRAM, etc.
      unsigned long flags;               // Socket flags
      struct file *file;                 // Associated file
      struct sock *sk;                   // Network layer socket
      const struct proto_ops *ops;       // Protocol operations
  };
  
  struct sock {
      struct sock_common __sk_common;    // Common fields
      socket_lock_t sk_lock;             // Socket lock
      struct sk_buff_head sk_receive_queue;  // Receive queue
      struct sk_buff_head sk_write_queue;    // Send queue
      int sk_rcvbuf;                     // Receive buffer size
      int sk_sndbuf;                     // Send buffer size
      struct sk_buff *sk_send_head;      // Send queue head
      void (*sk_data_ready)(struct sock *);  // Data ready callback
      void (*sk_write_space)(struct sock *); // Write space callback
      void (*sk_error_report)(struct sock *);// Error callback
  };
  ```

- **Socket Operations**:
  ```c
  struct proto_ops {
      int (*bind)(struct socket *, struct sockaddr *, int);
      int (*connect)(struct socket *, struct sockaddr *, int, int);
      int (*accept)(struct socket *, struct socket *, int);
      int (*listen)(struct socket *, int);
      int (*sendmsg)(struct socket *, struct msghdr *, size_t);
      int (*recvmsg)(struct socket *, struct msghdr *, size_t, int);
      unsigned int (*poll)(struct file *, struct socket *, struct poll_table_struct *);
      int (*ioctl)(struct socket *, unsigned int, unsigned long);
      int (*shutdown)(struct socket *, int);
      int (*setsockopt)(struct socket *, int, int, char __user *, unsigned int);
      int (*getsockopt)(struct socket *, int, int, char __user *, int __user *);
  };
  ```

#### 5.2.2 Network Protocols

- **IPv4 Implementation**:
  - IP header processing
  - Fragmentation and reassembly
  - Routing table lookup
  - ICMP (ping, error messages)
  - IP options handling

- **IPv6 Implementation**:
  - Extended headers
  - Neighbor discovery
  - Stateless address autoconfiguration (SLAAC)
  - ICMPv6

- **TCP Implementation**:
  ```c
  struct tcp_sock {
      struct inet_connection_sock inet_conn;
      u32 rcv_nxt;                       // Next expected sequence
      u32 snd_nxt;                       // Next sequence to send
      u32 snd_una;                       // First unacknowledged
      u32 snd_wnd;                       // Send window
      u32 rcv_wnd;                       // Receive window
      u32 srtt_us;                       // Smoothed RTT
      u32 mdev_us;                       // Mean deviation
      u32 rto;                           // Retransmission timeout
      struct sk_buff_head out_of_order_queue;
      // ... many more fields
  };
  ```

  - Three-way handshake (SYN, SYN-ACK, ACK)
  - Sliding window protocol
  - Congestion control (Cubic, BBR, Reno)
  - Fast retransmit and fast recovery
  - Selective acknowledgment (SACK)
  - TCP timestamps
  - Window scaling

- **UDP Implementation**:
  - Connectionless datagram service
  - Minimal overhead
  - Checksum validation
  - Multicast support

#### 5.2.3 Network Device Abstraction
- **Network Device Structure**:
  ```c
  struct net_device {
      char name[IFNAMSIZ];               // Interface name
      unsigned long state;               // Device state
      struct net_device_ops *netdev_ops; // Device operations
      struct ethtool_ops *ethtool_ops;   // Ethtool operations
      unsigned int flags;                // IFF_UP, IFF_BROADCAST, etc.
      unsigned int mtu;                  // Maximum transmission unit
      unsigned short type;               // Hardware type
      unsigned char addr_len;            // Hardware address length
      unsigned char *dev_addr;           // Hardware address
      struct netdev_queue *_tx;          // Transmit queues
      unsigned int num_tx_queues;        // Number of TX queues
      unsigned int real_num_tx_queues;   // Active TX queues
      struct Qdisc *qdisc;               // Queueing discipline
      unsigned long tx_queue_len;        // Transmit queue length
      struct net_device_stats stats;     // Statistics
  };
  ```

- **Device Operations**:
  ```c
  struct net_device_ops {
      int (*ndo_open)(struct net_device *);
      int (*ndo_stop)(struct net_device *);
      netdev_tx_t (*ndo_start_xmit)(struct sk_buff *, struct net_device *);
      void (*ndo_set_rx_mode)(struct net_device *);
      int (*ndo_set_mac_address)(struct net_device *, void *);
      int (*ndo_validate_addr)(struct net_device *);
      int (*ndo_do_ioctl)(struct net_device *, struct ifreq *, int);
      int (*ndo_change_mtu)(struct net_device *, int);
      void (*ndo_tx_timeout)(struct net_device *);
      struct net_device_stats* (*ndo_get_stats)(struct net_device *);
  };
  ```

#### 5.2.4 Packet Processing
- **Socket Buffer (skb)**:
  ```c
  struct sk_buff {
      struct sk_buff *next;              // Next buffer in list
      struct sk_buff *prev;              // Previous buffer
      struct sock *sk;                   // Owning socket
      struct net_device *dev;            // Device
      unsigned int len;                  // Data length
      unsigned int data_len;             // Data in fragments
      __u16 mac_len;                     // MAC header length
      __u16 hdr_len;                     // Writable header length
      __u32 priority;                    // Packet priority
      __u8 *head;                        // Buffer start
      __u8 *data;                        // Data start
      __u8 *tail;                        // Data end
      __u8 *end;                         // Buffer end
      unsigned char *mac_header;         // MAC header
      unsigned char *network_header;     // Network header
      unsigned char *transport_header;   // Transport header
  };
  ```

- **Receive Path**:
  ```
  1. NIC receives packet → DMA to memory
  2. Interrupt handler schedules NAPI poll
  3. NAPI poll retrieves packets from ring buffer
  4. Allocate sk_buff, copy packet data
  5. Pass to network layer (netif_receive_skb)
  6. Protocol handler (IP, ARP, etc.)
  7. Transport layer (TCP, UDP)
  8. Socket layer (deliver to application)
  ```

- **Transmit Path**:
  ```
  1. Application writes to socket
  2. Transport layer (TCP/UDP) creates packet
  3. Network layer (IP) adds header, routing
  4. Queueing discipline (traffic shaping)
  5. Device driver queues packet
  6. DMA to NIC transmit ring
  7. NIC transmits packet
  8. Completion interrupt, free sk_buff
  ```

#### 5.2.5 Netfilter and iptables
- **Netfilter Hooks**:
  - NF_IP_PRE_ROUTING: Before routing decision
  - NF_IP_LOCAL_IN: Destined for local system
  - NF_IP_FORWARD: Forwarded packets
  - NF_IP_LOCAL_OUT: Locally generated packets
  - NF_IP_POST_ROUTING: After routing decision

- **iptables Tables**:
  - filter: Packet filtering (INPUT, OUTPUT, FORWARD)
  - nat: Network address translation (PREROUTING, POSTROUTING)
  - mangle: Packet alteration (all hooks)
  - raw: Connection tracking exemption (PREROUTING, OUTPUT)

#### 5.2.6 Network Namespaces
- **Isolated Network Stacks**:
  - Separate network interfaces
  - Independent routing tables
  - Separate firewall rules
  - Container networking foundation

### 5.3 Advanced Networking Features

#### 5.3.1 Traffic Control (tc)
- **Queueing Disciplines (qdiscs)**:
  - pfifo_fast: Default 3-band FIFO
  - TBF (Token Bucket Filter): Rate limiting
  - HTB (Hierarchical Token Bucket): Hierarchical rate limiting
  - SFQ (Stochastic Fair Queueing): Fair queueing
  - FQ_CODEL: Fair queueing with controlled delay

#### 5.3.2 Network Bonding
- **Link Aggregation**:
  - Mode 0 (balance-rr): Round-robin
  - Mode 1 (active-backup): Failover
  - Mode 2 (balance-xor): XOR hashing
  - Mode 4 (802.3ad): LACP
  - Mode 5 (balance-tlb): Transmit load balancing
  - Mode 6 (balance-alb): Adaptive load balancing

#### 5.3.3 VLANs (802.1Q)
- **Virtual LANs**:
  - VLAN tagging (12-bit VLAN ID)
  - Multiple logical networks on single physical
  - VLAN interface creation (vconfig)

#### 5.3.4 Bridging
- **Layer 2 Forwarding**:
  - MAC address learning
  - Spanning Tree Protocol (STP)
  - VLAN-aware bridging
  - Container networking

### 5.4 Phase 5 Success Criteria

- [ ] Pipe throughput >5GB/s (local)
- [ ] Unix domain socket latency <1μs
- [ ] TCP throughput >10Gbps (with hardware offload)
- [ ] UDP packet rate >1M pps
- [ ] TCP connection setup <100μs
- [ ] Netfilter rule processing <1μs per packet
- [ ] Network namespace creation <10ms
- [ ] Socket buffer allocation <500ns
- [ ] NAPI poll processes >10K packets per cycle
- [ ] Zero-copy networking achieves >90% efficiency

### 5.5 Phase 5 Deliverables

1. **Code Modules**:
   - `ipc/` - IPC mechanisms
   - `net/core/` - Socket layer
   - `net/ipv4/` - IPv4 implementation
   - `net/ipv6/` - IPv6 implementation
   - `net/netfilter/` - Netfilter framework
   - `drivers/net/` - Network drivers

2. **Test Suites**:
   - IPC correctness tests
   - Network protocol tests
   - Performance benchmarks
   - Netfilter rule tests

3. **Documentation**:
   - IPC mechanisms guide
   - Networking stack architecture
   - Socket programming guide
   - Netfilter and iptables guide

---

## Phase 6: Device & Hardware Abstraction

**Objective**: Implement a comprehensive device driver framework and hardware abstraction layer.

### 6.1 Device Model

#### 6.1.1 Core Device Structures
- **Device Structure**:
  ```c
  struct device {
      struct device *parent;             // Parent device
      struct device_private *p;          // Private data
      struct kobject kobj;               // Sysfs representation
      const char *init_name;             // Initial name
      const struct device_type *type;    // Device type
      struct bus_type *bus;              // Bus type
      struct device_driver *driver;      // Driver
      void *platform_data;               // Platform-specific data
      void *driver_data;                 // Driver private data
      struct dev_pm_info power;          // Power management
      u64 *dma_mask;                     // DMA mask
      u64 coherent_dma_mask;             // Coherent DMA mask
      struct device_node *of_node;       // Device tree node
  };
  ```

- **Device Driver Structure**:
  ```c
  struct device_driver {
      const char *name;                  // Driver name
      struct bus_type *bus;              // Bus type
      struct module *owner;              // Module owner
      const struct of_device_id *of_match_table;  // Device tree match
      const struct acpi_device_id *acpi_match_table;  // ACPI match
      int (*probe)(struct device *);     // Probe function
      int (*remove)(struct device *);    // Remove function
      void (*shutdown)(struct device *); // Shutdown function
      int (*suspend)(struct device *, pm_message_t);  // Suspend
      int (*resume)(struct device *);    // Resume
  };
  ```

#### 6.1.2 Bus Abstraction
- **Bus Type Structure**:
  ```c
  struct bus_type {
      const char *name;                  // Bus name
      const char *dev_name;              // Device naming
      struct device *dev_root;           // Root device
      const struct attribute_group **bus_groups;  // Bus attributes
      const struct attribute_group **dev_groups;  // Device attributes
      const struct attribute_group **drv_groups;  // Driver attributes
      int (*match)(struct device *, struct device_driver *);  // Match function
      int (*probe)(struct device *);     // Probe function
      int (*remove)(struct device *);    // Remove function
      void (*shutdown)(struct device *); // Shutdown function
      int (*suspend)(struct device *, pm_message_t);  // Suspend
      int (*resume)(struct device *);    // Resume
  };
  ```

- **Common Bus Types**:
  - PCI bus
  - USB bus
  - Platform bus (SoC devices)
  - I2C bus
  - SPI bus
  - SCSI bus

#### 6.1.3 Class Abstraction
- **Device Class**:
  ```c
  struct class {
      const char *name;                  // Class name
      struct module *owner;              // Module owner
      const struct attribute_group **class_groups;  // Class attributes
      const struct attribute_group **dev_groups;    // Device attributes
      int (*dev_uevent)(struct device *, struct kobj_uevent_env *);
      void (*class_release)(struct class *);
      void (*dev_release)(struct device *);
      int (*suspend)(struct device *, pm_message_t);
      int (*resume)(struct device *);
      int (*shutdown)(struct device *);
  };
  ```

- **Common Classes**:
  - block: Block devices
  - net: Network devices
  - input: Input devices
  - tty: Terminal devices
  - graphics: Graphics devices

### 6.2 Character Devices

#### 6.2.1 Character Device Registration
- **cdev Structure**:
  ```c
  struct cdev {
      struct kobject kobj;               // Kernel object
      struct module *owner;              // Module owner
      const struct file_operations *ops; // File operations
      struct list_head list;             // List linkage
      dev_t dev;                         // Device number
      unsigned int count;                // Number of minors
  };
  ```

- **Registration**:
  ```c
  // Allocate device number
  alloc_chrdev_region(&dev, 0, 1, "mydevice");
  
  // Initialize cdev
  cdev_init(&my_cdev, &my_fops);
  my_cdev.owner = THIS_MODULE;
  
  // Add cdev to system
  cdev_add(&my_cdev, dev, 1);
  
  // Create device class and device
  my_class = class_create(THIS_MODULE, "myclass");
  device_create(my_class, NULL, dev, NULL, "mydevice");
  ```

#### 6.2.2 Character Device Operations
- **File Operations**:
  ```c
  struct file_operations {
      struct module *owner;
      loff_t (*llseek)(struct file *, loff_t, int);
      ssize_t (*read)(struct file *, char __user *, size_t, loff_t *);
      ssize_t (*write)(struct file *, const char __user *, size_t, loff_t *);
      unsigned int (*poll)(struct file *, struct poll_table_struct *);
      long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);
      int (*mmap)(struct file *, struct vm_area_struct *);
      int (*open)(struct inode *, struct file *);
      int (*release)(struct inode *, struct file *);
      int (*fsync)(struct file *, loff_t, loff_t, int);
      int (*fasync)(int, struct file *, int);
  };
  ```

### 6.3 Block Devices

#### 6.3.1 Block Device Registration
- **gendisk Registration**:
  ```c
  // Allocate gendisk
  struct gendisk *disk = alloc_disk(minors);
  
  // Setup disk
  disk->major = major;
  disk->first_minor = 0;
  disk->fops = &my_block_fops;
  disk->queue = my_request_queue;
  sprintf(disk->disk_name, "myblock");
  set_capacity(disk, sectors);
  
  // Add disk to system
  add_disk(disk);
  ```

#### 6.3.2 Request Queue Management
- **Request Queue Setup**:
  ```c
  struct request_queue *blk_init_queue(request_fn_proc *rfn, spinlock_t *lock);
  void blk_cleanup_queue(struct request_queue *q);
  
  // Request processing
  void my_request_fn(struct request_queue *q) {
      struct request *req;
      while ((req = blk_fetch_request(q)) != NULL) {
          // Process request
          if (my_transfer(req) != 0) {
              __blk_end_request_all(req, -EIO);
          } else {
              __blk_end_request_all(req, 0);
          }
      }
  }
  ```

### 6.4 Network Devices

#### 6.4.1 Network Device Registration
- **netdev Registration**:
  ```c
  // Allocate netdev
  struct net_device *dev = alloc_etherdev(sizeof(struct my_priv));
  
  // Setup netdev
  dev->netdev_ops = &my_netdev_ops;
  dev->ethtool_ops = &my_ethtool_ops;
  dev->watchdog_timeo = TX_TIMEOUT;
  
  // Register netdev
  register_netdev(dev);
  ```

#### 6.4.2 NAPI (New API)
- **NAPI Structure**:
  ```c
  struct napi_struct {
      struct list_head poll_list;        // Poll list
      unsigned long state;               // NAPI state
      int weight;                        // Budget
      int (*poll)(struct napi_struct *, int);  // Poll function
      struct net_device *dev;            // Network device
  };
  ```

- **NAPI Usage**:
  ```c
  // Initialize NAPI
  netif_napi_add(dev, &priv->napi, my_poll, NAPI_POLL_WEIGHT);
  
  // Enable NAPI
  napi_enable(&priv->napi);
  
  // In interrupt handler
  if (napi_schedule_prep(&priv->napi)) {
      disable_irq_nosync(dev->irq);
      __napi_schedule(&priv->napi);
  }
  
  // Poll function
  static int my_poll(struct napi_struct *napi, int budget) {
      int work_done = 0;
      while (work_done < budget && has_packets()) {
          process_packet();
          work_done++;
      }
      if (work_done < budget) {
          napi_complete(napi);
          enable_irq(dev->irq);
      }
      return work_done;
  }
  ```

### 6.5 Interrupt Handling

#### 6.5.1 Interrupt Registration
- **Request IRQ**:
  ```c
  int request_irq(unsigned int irq,
                  irq_handler_t handler,
                  unsigned long flags,
                  const char *name,
                  void *dev);
  
  void free_irq(unsigned int irq, void *dev);
  ```

- **IRQ Flags**:
  - IRQF_SHARED: Shared interrupt line
  - IRQF_TRIGGER_RISING: Rising edge trigger
  - IRQF_TRIGGER_FALLING: Falling edge trigger
  - IRQF_TRIGGER_HIGH: High level trigger
  - IRQF_TRIGGER_LOW: Low level trigger

#### 6.5.2 Threaded IRQ
- **Threaded Interrupt Handler**:
  ```c
  int request_threaded_irq(unsigned int irq,
                           irq_handler_t handler,
                           irq_handler_t thread_fn,
                           unsigned long flags,
                           const char *name,
                           void *dev);
  ```

- **Usage Pattern**:
  - handler: Fast top-half (acknowledge interrupt)
  - thread_fn: Slower bottom-half (process data)

### 6.6 DMA (Direct Memory Access)

#### 6.6.1 DMA API
- **Coherent DMA**:
  ```c
  void *dma_alloc_coherent(struct device *dev,
                           size_t size,
                           dma_addr_t *dma_handle,
                           gfp_t flag);
  
  void dma_free_coherent(struct device *dev,
                         size_t size,
                         void *cpu_addr,
                         dma_addr_t dma_handle);
  ```

- **Streaming DMA**:
  ```c
  dma_addr_t dma_map_single(struct device *dev,
                            void *ptr,
                            size_t size,
                            enum dma_data_direction dir);
  
  void dma_unmap_single(struct device *dev,
                        dma_addr_t dma_addr,
                        size_t size,
                        enum dma_data_direction dir);
  ```

#### 6.6.2 Scatter-Gather DMA
- **Scatter-Gather List**:
  ```c
  struct scatterlist {
      unsigned long page_link;           // Page pointer
      unsigned int offset;               // Offset in page
      unsigned int length;               // Length
      dma_addr_t dma_address;            // DMA address
      unsigned int dma_length;           // DMA length
  };
  
  int dma_map_sg(struct device *dev,
                 struct scatterlist *sg,
                 int nents,
                 enum dma_data_direction dir);
  
  void dma_unmap_sg(struct device *dev,
                    struct scatterlist *sg,
                    int nents,
                    enum dma_data_direction dir);
  ```

### 6.7 Power Management

#### 6.7.1 Device Power States
- **Power States**:
  - D0: Fully operational
  - D1: Low power, context retained
  - D2: Lower power, some context lost
  - D3hot: Lowest power, context lost
  - D3cold: Power off

#### 6.7.2 Runtime PM
- **Runtime PM API**:
  ```c
  int pm_runtime_get_sync(struct device *dev);
  int pm_runtime_put(struct device *dev);
  int pm_runtime_put_sync(struct device *dev);
  void pm_runtime_enable(struct device *dev);
  void pm_runtime_disable(struct device *dev);
  ```

#### 6.7.3 System Sleep
- **Suspend/Resume Callbacks**:
  ```c
  struct dev_pm_ops {
      int (*suspend)(struct device *);
      int (*resume)(struct device *);
      int (*freeze)(struct device *);
      int (*thaw)(struct device *);
      int (*poweroff)(struct device *);
      int (*restore)(struct device *);
      int (*suspend_late)(struct device *);
      int (*resume_early)(struct device *);
      int (*freeze_late)(struct device *);
      int (*thaw_early)(struct device *);
      int (*poweroff_late)(struct device *);
      int (*restore_early)(struct device *);
      int (*suspend_noirq)(struct device *);
      int (*resume_noirq)(struct device *);
      int (*freeze_noirq)(struct device *);
      int (*thaw_noirq)(struct device *);
      int (*poweroff_noirq)(struct device *);
      int (*restore_noirq)(struct device *);
  };
  ```

### 6.8 Platform Devices

#### 6.8.1 Platform Device Registration
- **Platform Device**:
  ```c
  struct platform_device {
      const char *name;                  // Device name
      int id;                            // Device instance
      struct device dev;                 // Device structure
      u32 num_resources;                 // Number of resources
      struct resource *resource;         // Resources
  };
  
  int platform_device_register(struct platform_device *pdev);
  void platform_device_unregister(struct platform_device *pdev);
  ```

- **Platform Driver**:
  ```c
  struct platform_driver {
      int (*probe)(struct platform_device *);
      int (*remove)(struct platform_device *);
      void (*shutdown)(struct platform_device *);
      int (*suspend)(struct platform_device *, pm_message_t);
      int (*resume)(struct platform_device *);
      struct device_driver driver;
      const struct platform_device_id *id_table;
  };
  
  int platform_driver_register(struct platform_driver *drv);
  void platform_driver_unregister(struct platform_driver *drv);
  ```

### 6.9 Device Tree

#### 6.9.1 Device Tree Parsing
- **OF (Open Firmware) API**:
  ```c
  struct device_node *of_find_node_by_path(const char *path);
  struct device_node *of_find_compatible_node(struct device_node *from,
                                              const char *type,
                                              const char *compatible);
  int of_property_read_u32(const struct device_node *np,
                           const char *propname,
                           u32 *out_value);
  const void *of_get_property(const struct device_node *np,
                              const char *name,
                              int *lenp);
  ```

### 6.10 Phase 6 Success Criteria

- [ ] Device model supports 100+ device types
- [ ] Character device open/close <5μs
- [ ] Block device I/O latency <100μs
- [ ] Network device packet processing >1M pps
- [ ] Interrupt latency <10μs (99th percentile)
- [ ] DMA transfer setup <1μs
- [ ] Runtime PM state transitions <1ms
- [ ] Device tree parsing <10ms at boot
- [ ] Hot-plug device detection <100ms
- [ ] Driver probe/remove completes in <500ms

### 6.11 Phase 6 Deliverables

1. **Code Modules**:
   - `drivers/base/` - Device model core
   - `drivers/char/` - Character device drivers
   - `drivers/block/` - Block device drivers
   - `drivers/net/` - Network device drivers
   - `drivers/platform/` - Platform devices
   - `kernel/irq/` - Interrupt handling

2. **Test Suites**:
   - Device model tests
   - Driver probe/remove tests
   - Interrupt handling tests
   - DMA tests
   - Power management tests

3. **Documentation**:
   - Device driver development guide
   - Device model architecture
   - Interrupt handling guide
   - DMA programming guide
   - Power management guide

---

## Phase 7: System Services & Userland Interface

**Objective**: Implement system services, system call interface, and userland support infrastructure.

### 7.1 System Call Interface

#### 7.1.1 System Call Mechanism
- **System Call Entry**:
  ```c
  // x86_64 system call entry
  ENTRY(entry_SYSCALL_64)
      swapgs                             // Switch to kernel GS
      movq %rsp, PER_CPU_VAR(rsp_scratch)
      movq PER_CPU_VAR(cpu_current_top_of_stack), %rsp
      pushq $__USER_DS                   // SS
      pushq PER_CPU_VAR(rsp_scratch)     // RSP
      pushq %r11                         // RFLAGS
      pushq $__USER_CS                   // CS
      pushq %rcx                         // RIP
      pushq %rax                         // System call number
      // ... save registers
      call do_syscall_64
      // ... restore registers
      sysretq
  END(entry_SYSCALL_64)
  ```

- **System Call Table**:
  ```c
  const sys_call_ptr_t sys_call_table[__NR_syscall_max+1] = {
      [0 ... __NR_syscall_max] = &sys_ni_syscall,
      [__NR_read] = sys_read,
      [__NR_write] = sys_write,
      [__NR_open] = sys_open,
      [__NR_close] = sys_close,
      // ... hundreds more
  };
  ```

#### 7.1.2 System Call Categories
- **Process Management**:
  - fork, clone, execve, exit, wait4
  - getpid, getppid, gettid
  - setuid, setgid, setgroups
  - nice, sched_setscheduler, sched_setaffinity

- **Memory Management**:
  - brk, mmap, munmap, mprotect, madvise
  - mlock, munlock, mlockall, munlockall
  - mincore, msync

- **File Operations**:
  - open, close, read, write, lseek
  - stat, fstat, lstat
  - access, chmod, chown
  - link, unlink, rename, mkdir, rmdir
  - readdir, getcwd, chdir

- **I/O Multiplexing**:
  - select, poll, epoll_create, epoll_ctl, epoll_wait

- **Signals**:
  - kill, sigaction, sigprocmask, sigpending
  - sigsuspend, sigaltstack

- **Time**:
  - time, gettimeofday, clock_gettime
  - nanosleep, alarm, setitimer

- **Networking**:
  - socket, bind, listen, accept, connect
  - send, recv, sendto, recvfrom
  - setsockopt, getsockopt, shutdown

### 7.2 Virtual System Calls (vDSO)

#### 7.2.1 vDSO Implementation
- **Fast System Calls**:
  - gettimeofday: Read time without kernel entry
  - clock_gettime: High-resolution time
  - getcpu: Get current CPU number

- **vDSO Mapping**:
  ```c
  // Map vDSO into process address space
  static int __init init_vdso(void) {
      vdso_pages = (vdso_end - vdso_start + PAGE_SIZE - 1) / PAGE_SIZE;
      for (i = 0; i < vdso_pages; i++) {
          struct page *pg = virt_to_page(vdso_start + i * PAGE_SIZE);
          vdso_pagelist[i] = pg;
      }
      return 0;
  }
  ```

### 7.3 ELF Binary Loading

#### 7.3.1 ELF Format Support
- **ELF Header Parsing**:
  ```c
  struct elf64_hdr {
      unsigned char e_ident[EI_NIDENT];  // Magic number
      Elf64_Half e_type;                 // Object file type
      Elf64_Half e_machine;              // Architecture
      Elf64_Word e_version;              // Object file version
      Elf64_Addr e_entry;                // Entry point
      Elf64_Off e_phoff;                 // Program header offset
      Elf64_Off e_shoff;                 // Section header offset
      Elf64_Word e_flags;                // Processor-specific flags
      Elf64_Half e_ehsize;               // ELF header size
      Elf64_Half e_phentsize;            // Program header entry size
      Elf64_Half e_phnum;                // Program header count
      Elf64_Half e_shentsize;            // Section header entry size
      Elf64_Half e_shnum;                // Section header count
      Elf64_Half e_shstrndx;             // Section name string table index
  };
  ```

- **Program Header Loading**:
  ```c
  struct elf64_phdr {
      Elf64_Word p_type;                 // Segment type
      Elf64_Word p_flags;                // Segment flags
      Elf64_Off p_offset;                // Segment file offset
      Elf64_Addr p_vaddr;                // Segment virtual address
      Elf64_Addr p_paddr;                // Segment physical address
      Elf64_Xword p_filesz;              // Segment size in file
      Elf64_Xword p_memsz;               // Segment size in memory
      Elf64_Xword p_align;               // Segment alignment
  };
  ```

#### 7.3.2 Dynamic Linking
- **Interpreter Loading**:
  - Load PT_INTERP segment (typically /lib64/ld-linux-x86-64.so.2)
  - Map interpreter into address space
  - Transfer control to interpreter
  - Interpreter loads shared libraries and relocates

- **Auxiliary Vector**:
  ```c
  // Passed on stack to dynamic linker
  AT_PHDR      // Program headers address
  AT_PHENT     // Program header entry size
  AT_PHNUM     // Number of program headers
  AT_PAGESZ    // Page size
  AT_BASE      // Interpreter base address
  AT_ENTRY     // Entry point
  AT_UID       // Real UID
  AT_EUID      // Effective UID
  AT_GID       // Real GID
  AT_EGID      // Effective GID
  AT_RANDOM    // Random bytes for stack canary
  ```

### 7.4 Core Dump Generation

#### 7.4.1 Core Dump Format
- **ELF Core Dump**:
  - ELF header with ET_CORE type
  - PT_NOTE segment with process info
  - PT_LOAD segments for memory regions
  - Register state in notes

- **Core Dump Trigger**:
  ```c
  void do_coredump(const siginfo_t *siginfo) {
      1. Check core dump limits (ulimit -c)
      2. Create core file (core or core.pid)
      3. Write ELF header
      4. Write PT_NOTE with process info
      5. Write memory segments (text, data, stack, heap)
      6. Close file
  }
  ```

### 7.5 Pseudo Filesystems

#### 7.5.1 /proc Filesystem
- **Process Information**:
  - `/proc/[pid]/cmdline` - Command line
  - `/proc/[pid]/environ` - Environment variables
  - `/proc/[pid]/status` - Process status
  - `/proc/[pid]/maps` - Memory mappings
  - `/proc/[pid]/fd/` - Open file descriptors
  - `/proc/[pid]/task/` - Threads

- **System Information**:
  - `/proc/cpuinfo` - CPU information
  - `/proc/meminfo` - Memory statistics
  - `/proc/loadavg` - Load average
  - `/proc/uptime` - System uptime
  - `/proc/version` - Kernel version

#### 7.5.2 /sys Filesystem
- **Device Hierarchy**:
  - `/sys/devices/` - Physical device tree
  - `/sys/bus/` - Bus types
  - `/sys/class/` - Device classes
  - `/sys/block/` - Block devices
  - `/sys/module/` - Loaded kernel modules

#### 7.5.3 /dev Filesystem (devtmpfs)
- **Device Nodes**:
  - Automatically populated by kernel
  - Character and block device nodes
  - Symlinks for convenience (e.g., /dev/disk/by-uuid/)

### 7.6 Init System Support

#### 7.6.1 Kernel Init Process
- **Boot Sequence**:
  ```c
  start_kernel() {
      1. Setup architecture-specific code
      2. Initialize memory management
      3. Initialize scheduler
      4. Initialize device drivers
      5. Mount root filesystem
      6. Execute init process (PID 1)
  }
  ```

- **Init Process Search**:
  ```c
  // Try in order
  run_init_process("/sbin/init");
  run_init_process("/etc/init");
  run_init_process("/bin/init");
  run_init_process("/bin/sh");
  panic("No init found");
  ```

#### 7.6.2 Initramfs Support
- **Early Userspace**:
  - Embedded in kernel image
  - Unpacked to rootfs at boot
  - Contains minimal tools for mounting real root
  - Supports modular kernel (load drivers before root mount)

### 7.7 Module Loading

#### 7.7.1 Kernel Module Format
- **Module Structure**:
  ```c
  struct module {
      enum module_state state;           // Module state
      struct list_head list;             // List of modules
      char name[MODULE_NAME_LEN];        // Module name
      struct module_kobject mkobj;       // Sysfs representation
      struct module_attribute *modinfo_attrs;
      const char *version;               // Module version
      const char *srcversion;            // Source version
      struct kobject *holders_dir;       // Module holders
      const struct kernel_symbol *syms;  // Exported symbols
      const unsigned long *crcs;         // Symbol CRCs
      unsigned int num_syms;             // Number of symbols
      struct kernel_param *kp;           // Module parameters
      unsigned int num_kp;               // Number of parameters
      void *module_init;                 // Init function
      void *module_core;                 // Core code
      unsigned int init_size;            // Init section size
      unsigned int core_size;            // Core section size
  };
  ```

- **Module Loading**:
  ```c
  sys_init_module(void __user *umod, unsigned long len, const char __user *uargs) {
      1. Copy module from userspace
      2. Verify ELF format
      3. Allocate memory for module
      4. Relocate symbols
      5. Resolve dependencies
      6. Call module init function
      7. Add to loaded modules list
  }
  ```

#### 7.7.2 Symbol Export
- **EXPORT_SYMBOL**:
  ```c
  EXPORT_SYMBOL(function_name);          // Export to all modules
  EXPORT_SYMBOL_GPL(function_name);      // Export to GPL modules only
  ```

### 7.8 Kernel Parameters

#### 7.8.1 Boot Parameters
- **Command Line Parsing**:
  - Passed by bootloader (e.g., GRUB)
  - Format: `key=value` or `key`
  - Examples: `root=/dev/sda1`, `ro`, `quiet`, `init=/bin/bash`

#### 7.8.2 Module Parameters
- **Parameter Definition**:
  ```c
  static int my_param = 0;
  module_param(my_param, int, 0644);
  MODULE_PARM_DESC(my_param, "Description of my_param");
  ```

- **Runtime Modification**:
  - `/sys/module/[module]/parameters/[param]`

### 7.9 Kernel Debugging Support

#### 7.9.1 Printk and Log Levels
- **Log Levels**:
  ```c
  #define KERN_EMERG    "<0>"  // System is unusable
  #define KERN_ALERT    "<1>"  // Action must be taken immediately
  #define KERN_CRIT     "<2>"  // Critical conditions
  #define KERN_ERR      "<3>"  // Error conditions
  #define KERN_WARNING  "<4>"  // Warning conditions
  #define KERN_NOTICE   "<5>"  // Normal but significant
  #define KERN_INFO     "<6>"  // Informational
  #define KERN_DEBUG    "<7>"  // Debug-level messages
  ```

#### 7.9.2 Debugfs
- **Debug Filesystem**:
  - Mounted at `/sys/kernel/debug/`
  - Provides debugging interfaces
  - Not for production use

### 7.10 Phase 7 Success Criteria

- [ ] System call latency <300ns (vDSO) to <1μs (kernel entry)
- [ ] ELF binary loading <5ms for typical programs
- [ ] Core dump generation <1s for 1GB process
- [ ] /proc file reads <10μs
- [ ] Module loading <100ms
- [ ] Init process starts within 5s of kernel boot
- [ ] vDSO provides 10x speedup for gettimeofday
- [ ] System call table supports 400+ syscalls
- [ ] Kernel parameters parsed correctly at boot
- [ ] Debugfs provides comprehensive kernel state

### 7.11 Phase 7 Deliverables

1. **Code Modules**:
   - `arch/x86/entry/` - System call entry
   - `kernel/sys.c` - System call implementations
   - `fs/binfmt_elf.c` - ELF binary loading
   - `fs/proc/` - Procfs implementation
   - `fs/sysfs/` - Sysfs implementation
   - `kernel/module.c` - Module loading

2. **Test Suites**:
   - System call tests
   - ELF loading tests
   - Module loading tests
   - Pseudo filesystem tests

3. **Documentation**:
   - System call reference
   - ELF loading guide
   - Module development guide
   - Kernel parameters reference

---

## Phase 8: Observability, Testing & Readiness

**Objective**: Implement comprehensive observability, testing infrastructure, and production readiness features.

### 8.1 Tracing and Profiling

#### 8.1.1 Ftrace (Function Tracer)
- **Tracing Infrastructure**:
  ```c
  // Function tracer
  trace_printk("Function %s called with arg %d\n", __func__, arg);
  
  // Trace events
  TRACE_EVENT(my_event,
      TP_PROTO(int value),
      TP_ARGS(value),
      TP_STRUCT__entry(
          __field(int, value)
      ),
      TP_fast_assign(
          __entry->value = value;
      ),
      TP_printk("value=%d", __entry->value)
  );
  ```

- **Ftrace Features**:
  - Function tracing (all kernel functions)
  - Function graph tracing (call graphs)
  - Event tracing (predefined trace points)
  - Dynamic tracing (kprobes)
  - Latency tracing (irqsoff, preemptoff, wakeup)

- **Ftrace Interface**:
  - `/sys/kernel/debug/tracing/` - Control interface
  - `trace` - Current trace buffer
  - `available_tracers` - Available tracers
  - `current_tracer` - Active tracer
  - `trace_options` - Tracing options
  - `set_ftrace_filter` - Function filter

#### 8.1.2 Perf Events
- **Performance Monitoring**:
  ```c
  struct perf_event_attr attr = {
      .type = PERF_TYPE_HARDWARE,
      .config = PERF_COUNT_HW_CPU_CYCLES,
      .size = sizeof(struct perf_event_attr),
      .disabled = 1,
      .exclude_kernel = 0,
      .exclude_hv = 1,
  };
  
  int fd = perf_event_open(&attr, pid, cpu, group_fd, flags);
  ```

- **Event Types**:
  - Hardware events (CPU cycles, instructions, cache misses)
  - Software events (context switches, page faults)
  - Tracepoint events (kernel tracepoints)
  - Dynamic events (kprobes, uprobes)

#### 8.1.3 eBPF (Extended Berkeley Packet Filter)
- **Programmable Tracing**:
  - Safe in-kernel execution
  - JIT compilation for performance
  - Verifier for safety guarantees
  - Maps for data storage

- **eBPF Programs**:
  ```c
  SEC("kprobe/sys_execve")
  int trace_execve(struct pt_regs *ctx) {
      char comm[16];
      bpf_get_current_comm(&comm, sizeof(comm));
      bpf_trace_printk("execve: %s\n", comm);
      return 0;
  }
  ```

- **eBPF Use Cases**:
  - Performance analysis
  - Security monitoring
  - Network packet filtering
  - System call filtering (seccomp-bpf)

### 8.2 Kernel Debugging

#### 8.2.1 KGDB (Kernel GNU Debugger)
- **Remote Debugging**:
  - GDB connection over serial or network
  - Breakpoints and watchpoints
  - Single-stepping
  - Memory inspection

- **KGDB Setup**:
  ```
  # Kernel command line
  kgdboc=ttyS0,115200 kgdbwait
  
  # GDB connection
  (gdb) target remote /dev/ttyS0
  (gdb) break sys_read
  (gdb) continue
  ```

#### 8.2.2 Kernel Oops and Panic
- **Oops Handler**:
  - Capture register state
  - Print stack trace
  - Identify faulting instruction
  - Continue execution (if possible)

- **Panic Handler**:
  - Print panic message
  - Dump kernel log
  - Reboot or halt system
  - Trigger kdump (if configured)

#### 8.2.3 Kdump (Kernel Crash Dump)
- **Crash Dump Mechanism**:
  - Reserve memory for crash kernel
  - Load crash kernel at boot
  - On panic, kexec into crash kernel
  - Crash kernel dumps memory to disk

### 8.3 Kernel Testing

#### 8.3.1 Unit Testing
- **KUnit Framework**:
  ```c
  static void test_example(struct kunit *test) {
      KUNIT_EXPECT_EQ(test, 1 + 1, 2);
      KUNIT_EXPECT_TRUE(test, true);
      KUNIT_EXPECT_STREQ(test, "hello", "hello");
  }
  
  static struct kunit_case example_test_cases[] = {
      KUNIT_CASE(test_example),
      {}
  };
  
  static struct kunit_suite example_test_suite = {
      .name = "example",
      .test_cases = example_test_cases,
  };
  
  kunit_test_suite(example_test_suite);
  ```

#### 8.3.2 Integration Testing
- **Test Scenarios**:
  - Multi-threaded stress tests
  - Memory pressure tests
  - I/O stress tests
  - Network stress tests
  - Fault injection tests

#### 8.3.3 Fuzzing
- **Syzkaller**:
  - System call fuzzer
  - Coverage-guided fuzzing
  - Automatic bug detection
  - Reproducer generation

### 8.4 Performance Monitoring

#### 8.4.1 System Statistics
- **Key Metrics**:
  - CPU utilization (user, system, idle, iowait)
  - Memory usage (free, used, cached, buffers)
  - Disk I/O (reads, writes, throughput, latency)
  - Network I/O (packets, bytes, errors, drops)
  - Context switches
  - Interrupts

- **Statistics Interfaces**:
  - `/proc/stat` - CPU and system statistics
  - `/proc/meminfo` - Memory statistics
  - `/proc/diskstats` - Disk statistics
  - `/proc/net/dev` - Network statistics

#### 8.4.2 Performance Counters
- **Hardware Counters**:
  - CPU cycles
  - Instructions retired
  - Cache hits/misses (L1, L2, L3)
  - Branch mispredictions
  - TLB misses

- **Software Counters**:
  - Page faults
  - Context switches
  - CPU migrations
  - Alignment faults

### 8.5 Logging and Auditing

#### 8.5.1 Kernel Log Buffer
- **Printk Ring Buffer**:
  - Circular buffer for kernel messages
  - Multiple log levels
  - Persistent across reboots (pstore)

- **Log Buffer Access**:
  - `dmesg` command
  - `/proc/kmsg` - Kernel messages
  - `/dev/kmsg` - Kernel message device

#### 8.5.2 Audit Framework
- **Audit System**:
  - System call auditing
  - File access auditing
  - Security event auditing
  - Audit rules and filters

- **Audit Log**:
  ```
  type=SYSCALL msg=audit(1234567890.123:456): arch=c000003e syscall=2 success=yes exit=3 a0=7fff12345678 a1=0 a2=1b6 a3=0 items=1 ppid=1234 pid=5678 auid=1000 uid=1000 gid=1000 euid=1000 suid=1000 fsuid=1000 egid=1000 sgid=1000 fsgid=1000 tty=pts0 ses=1 comm="cat" exe="/bin/cat" key="file_access"
  ```

### 8.6 Error Handling and Recovery

#### 8.6.1 Error Injection
- **Fault Injection Framework**:
  - Simulate hardware failures
  - Simulate memory allocation failures
  - Simulate I/O errors
  - Test error handling paths

#### 8.6.2 Watchdog Timer
- **Hardware Watchdog**:
  - Periodic timer that must be reset
  - Reboots system if not reset (kernel hang)
  - Configurable timeout

- **Soft Lockup Detector**:
  - Detects CPU spinning without scheduling
  - Prints stack trace
  - Can trigger panic

- **Hard Lockup Detector**:
  - Detects CPU spinning with interrupts disabled
  - Uses NMI (Non-Maskable Interrupt)
  - Prints stack trace

### 8.7 Security Hardening

#### 8.7.1 Kernel Hardening Options
- **Compile-Time Hardening**:
  - Stack protector (CONFIG_STACKPROTECTOR)
  - FORTIFY_SOURCE (buffer overflow detection)
  - RELRO (Relocation Read-Only)
  - PIE (Position Independent Executable)

- **Runtime Hardening**:
  - KASLR (Kernel Address Space Layout Randomization)
  - SMEP (Supervisor Mode Execution Prevention)
  - SMAP (Supervisor Mode Access Prevention)
  - KPTI (Kernel Page Table Isolation) - Meltdown mitigation

#### 8.7.2 Vulnerability Mitigation
- **Spectre/Meltdown Mitigations**:
  - Retpoline (indirect branch mitigation)
  - IBRS (Indirect Branch Restricted Speculation)
  - IBPB (Indirect Branch Prediction Barrier)
  - STIBP (Single Thread Indirect Branch Predictors)

### 8.8 Documentation and Code Quality

#### 8.8.1 Code Documentation
- **Kernel-doc Comments**:
  ```c
  /**
   * my_function - Brief description
   * @param1: Description of param1
   * @param2: Description of param2
   *
   * Longer description of the function.
   *
   * Return: Description of return value
   */
  int my_function(int param1, int param2) {
      // Implementation
  }
  ```

#### 8.8.2 Static Analysis
- **Tools**:
  - Sparse: Semantic checker
  - Coccinelle: Pattern-based code transformation
  - Smatch: Static analysis tool
  - Coverity: Commercial static analyzer

#### 8.8.3 Code Style
- **Coding Standards**:
  - Linux kernel coding style (Documentation/process/coding-style.rst)
  - Checkpatch.pl for style checking
  - Consistent indentation (tabs, not spaces)
  - 80-column line limit (flexible)

### 8.9 Production Readiness

#### 8.9.1 Stability Testing
- **Long-Running Tests**:
  - 72-hour stress tests
  - Memory leak detection
  - Resource exhaustion tests
  - Thermal stress tests

#### 8.9.2 Performance Benchmarking
- **Benchmark Suites**:
  - LMbench (microbenchmarks)
  - UnixBench (system benchmarks)
  - Phoronix Test Suite
  - Custom BDI-specific benchmarks

#### 8.9.3 Compatibility Testing
- **Userspace Compatibility**:
  - POSIX compliance testing
  - LSB (Linux Standard Base) compliance
  - Application compatibility testing
  - ABI stability verification

### 8.10 Phase 8 Success Criteria

- [ ] Ftrace captures all kernel function calls with <5% overhead
- [ ] Perf events provide accurate performance counters
- [ ] eBPF programs execute safely with verifier guarantees
- [ ] KGDB enables remote debugging without crashes
- [ ] Kdump successfully captures crash dumps
- [ ] KUnit tests achieve >80% code coverage
- [ ] Syzkaller finds no critical bugs in 1 week of fuzzing
- [ ] Performance counters accurate within 1%
- [ ] Audit framework logs all security events
- [ ] Watchdog detects and recovers from hangs
- [ ] Security hardening passes all vulnerability tests
- [ ] Static analysis reports zero critical issues
- [ ] 72-hour stress test completes without errors
- [ ] Performance benchmarks meet or exceed targets
- [ ] POSIX compliance tests pass 100%

### 8.11 Phase 8 Deliverables

1. **Code Modules**:
   - `kernel/trace/` - Ftrace implementation
   - `kernel/events/` - Perf events
   - `kernel/bpf/` - eBPF implementation
   - `kernel/debug/` - KGDB
   - `kernel/panic.c` - Panic handler
   - `kernel/audit.c` - Audit framework

2. **Test Suites**:
   - KUnit test suite
   - Integration test suite
   - Stress test suite
   - Fuzzing infrastructure

3. **Documentation**:
   - Tracing and profiling guide
   - Debugging guide
   - Testing guide
   - Performance tuning guide
   - Security hardening guide

---

## Dependencies and Integration Points

### Inter-Phase Dependencies

#### Phase 1 → Phase 2
- Memory allocators required for task_struct allocation
- Page tables needed for per-process address spaces
- Memory zones required for NUMA-aware scheduling

#### Phase 2 → Phase 3
- Scheduler required for process creation (fork/clone)
- Context switching needed for process execution
- Load balancing affects process placement

#### Phase 3 → Phase 4
- Process credentials needed for filesystem permissions
- Namespaces required for mount isolation
- Cgroups needed for I/O bandwidth limits

#### Phase 4 → Phase 5
- VFS required for Unix domain socket filesystem representation
- Page cache used for network buffer management
- Block layer needed for network storage (NFS, iSCSI)

#### Phase 5 → Phase 6
- Network stack required for network device drivers
- IPC mechanisms used for device event notification
- Socket buffers used in network device receive path

#### Phase 6 → Phase 7
- Device model required for /dev filesystem
- Character devices needed for system call interface
- Block devices required for root filesystem mount

#### Phase 7 → Phase 8
- System call interface needed for tracing tools
- Pseudo filesystems required for observability
- Module loading needed for dynamic instrumentation

### Critical Integration Points

#### Memory Management ↔ Scheduler
- Memory allocation in scheduler (task_struct, stacks)
- NUMA-aware scheduling based on memory locality
- Memory pressure affects scheduling decisions

#### Scheduler ↔ Process Management
- Process creation requires scheduler integration
- Process priority affects scheduling
- CPU affinity constrains scheduling decisions

#### Process Management ↔ Security
- Credentials checked on process creation
- Capabilities affect process privileges
- Namespaces provide process isolation

#### Filesystem ↔ Memory Management
- Page cache bridges filesystem and memory
- Memory-mapped files require VMA integration
- Swap requires filesystem support

#### Networking ↔ Memory Management
- Socket buffers allocated from slab
- Zero-copy networking uses page remapping
- Network buffer management affects memory pressure

#### Device Drivers ↔ Interrupt Handling
- Interrupt handlers schedule device processing
- DMA requires memory management integration
- Device power management affects interrupt routing

#### System Calls ↔ All Subsystems
- System calls are the primary kernel entry point
- All subsystems expose functionality via syscalls
- System call performance affects overall system performance

---

## Success Criteria Summary

### Phase 1: Memory & HAM Readiness
- Robust physical and virtual memory management
- HAM integration interface functional
- Memory debugging and instrumentation operational

### Phase 2: Scheduling & Concurrency
- Fair and efficient process scheduling
- Comprehensive concurrency primitives
- Load balancing and CPU hotplug support

### Phase 3: Process Lifecycle & Security
- Complete process management (fork, exec, exit)
- Namespace and cgroup isolation
- Security modules and capabilities

### Phase 4: Storage & Filesystems
- VFS layer with multiple filesystem support
- Efficient page cache and block layer
- File locking and advanced I/O

### Phase 5: IPC & Networking
- Comprehensive IPC mechanisms
- Full TCP/IP networking stack
- Advanced networking features

### Phase 6: Device & Hardware Abstraction
- Unified device model
- Character, block, and network device support
- Power management and device tree

### Phase 7: System Services & Userland
- Complete system call interface
- ELF binary loading and dynamic linking
- Pseudo filesystems and module loading

### Phase 8: Observability & Testing
- Comprehensive tracing and profiling
- Robust testing infrastructure
- Production readiness and security hardening

---

## Conclusion

This kernel completion plan provides a comprehensive roadmap for building a production-ready monolithic kernel for the BDI project. Each phase builds upon the previous, ensuring a solid foundation for the next. The plan emphasizes:

1. **Modularity**: Each phase is self-contained with clear deliverables
2. **Testability**: Comprehensive testing at each phase
3. **Observability**: Built-in instrumentation and debugging
4. **Security**: Security considerations integrated throughout
5. **Performance**: Performance optimization and benchmarking
6. **Documentation**: Thorough documentation for maintainability

By following this plan, the BDI kernel will achieve:
- **Reliability**: Robust error handling and recovery
- **Performance**: Optimized for modern hardware
- **Security**: Hardened against vulnerabilities
- **Maintainability**: Well-documented and tested
- **Extensibility**: Modular design for future enhancements

The completion of all 8 phases will result in a kernel that embodies BDI's vision of a universal computational substrate, providing the foundation for innovative computational paradigms while maintaining compatibility with existing software ecosystems.
