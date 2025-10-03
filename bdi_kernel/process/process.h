

/**
 * @file process.h
 * @brief Process Control Block (PCB) and Process Management
 * 
 * Phase 8: Process Management & Lifecycle
 * 
 * This header defines the Process Control Block (PCB) and related structures
 * for the BDI Kernel process model with C23 features, atomic operations,
 * and lock-free process table management.
 * 
 * Key Features:
 * - C23 modernization (nullptr, [[nodiscard]], _Atomic)
 * - Lock-free process table with atomic PID allocation
 * - Atomic state transitions
 * - Integration with memory management and IPC
 * - Copy-On-Write (COW) support for fork
 * - Process lifecycle management (fork, exec, wait, exit)
 */

#ifndef BDI_PROCESS_H
#define BDI_PROCESS_H

#include "../kernel/c23_compat.h"
#include "../kernel/memory.h"
#include "../kernel/ipc.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <sys/types.h>

/* Forward declaration to avoid including graph.h with constexpr issues */
typedef struct BdiGraph BdiGraph;

/* ===================================================================
 * Process Constants
 * =================================================================== */

/* Process limits (constexpr) */
#define MAX_PROCESSES           4096    /* Maximum number of processes */
#define MAX_PROCESS_NAME        64      /* Maximum process name length */
#define MAX_OPEN_FILES          1024    /* Maximum open files per process */
#define MAX_CHILDREN            256     /* Maximum child processes */
#define INVALID_PID             0       /* Invalid process ID */
#define KERNEL_PID              1       /* Kernel process ID */

/* Process priorities */
#define PRIORITY_MIN            0       /* Minimum priority */
#define PRIORITY_DEFAULT        100     /* Default priority */
#define PRIORITY_MAX            255     /* Maximum priority */

/* Process flags */
#define PROC_FLAG_KERNEL        (1U << 0)  /* Kernel process */
#define PROC_FLAG_USER          (1U << 1)  /* User process */
#define PROC_FLAG_SYSTEM        (1U << 2)  /* System process */
#define PROC_FLAG_TRACED        (1U << 3)  /* Being traced */
#define PROC_FLAG_STOPPED       (1U << 4)  /* Stopped by signal */
#define PROC_FLAG_EXITING       (1U << 5)  /* Exiting */

/* Wait options */
#define WAIT_NOHANG             (1U << 0)  /* Don't block */
#define WAIT_UNTRACED           (1U << 1)  /* Don't wait for traced children */

/* Exit status macros */
#define EXIT_SUCCESS            0       /* Successful exit */
#define EXIT_FAILURE            1       /* Failed exit */

/* ===================================================================
 * Process Types and Enumerations
 * =================================================================== */

/**
 * @brief Process ID type
 */
typedef uint64_t ProcessId;

/**
 * @brief Process states
 * 
 * State transitions are atomic using compare-exchange operations.
 */
typedef enum {
    PROC_UNUSED = 0,        /* PCB slot is unused */
    PROC_CREATING,          /* Process being created */
    PROC_READY,             /* Ready to run */
    PROC_RUNNING,           /* Currently running */
    PROC_SLEEPING,          /* Sleeping (waiting for event) */
    PROC_WAITING,           /* Waiting for child */
    PROC_ZOMBIE,            /* Exited, awaiting parent wait */
    PROC_DEAD               /* Fully cleaned up */
} ProcessState;

/**
 * @brief Process exit status
 */
typedef struct {
    int exit_code;          /* Exit code */
    int signal;             /* Signal that caused exit (0 if normal) */
    bool core_dumped;       /* Core dump generated */
} ExitStatus;

/**
 * @brief Process capabilities and permissions
 */
typedef struct {
    uint64_t allowed_services_mask;     /* Bitmask for allowed services */
    uint64_t allowed_syscalls_mask;     /* Bitmask for allowed syscalls */
    uint32_t uid;                       /* User ID */
    uint32_t gid;                       /* Group ID */
    uint32_t euid;                      /* Effective user ID */
    uint32_t egid;                      /* Effective group ID */
} CapabilitySet;

/**
 * @brief Memory region descriptor
 * 
 * Two-level refcounting for COW support:
 * - ref_count: Tracks lifetime of this descriptor (process-local)
 * - cow_ref_count: Tracks shared physical memory (shared between processes)
 * 
 * For non-COW regions, cow_ref_count is nullptr.
 * For COW regions, multiple descriptors share the same cow_ref_count pointer.
 */
typedef struct MemoryRegion {
    void *base;                         /* Base address */
    size_t size;                        /* Region size */
    uint32_t flags;                     /* Memory flags */
    _Atomic uint32_t ref_count;         /* Descriptor reference count */
    _Atomic(int) *cow_ref_count;        /* Shared COW refcount (nullptr if not COW) */
    struct MemoryRegion *next;          /* Next region in list */
} MemoryRegion;

/**
 * @brief File descriptor entry
 */
typedef struct {
    int fd;                             /* File descriptor number */
    void *file_handle;                  /* Opaque file handle */
    uint32_t flags;                     /* File flags */
    _Atomic uint32_t ref_count;         /* Reference count */
} FileDescriptor;

/**
 * @brief Process statistics
 */
typedef struct {
    _Atomic uint64_t cpu_time_ns;      /* CPU time in nanoseconds */
    _Atomic uint64_t user_time_ns;     /* User time in nanoseconds */
    _Atomic uint64_t system_time_ns;   /* System time in nanoseconds */
    _Atomic uint64_t page_faults;      /* Page fault count */
    _Atomic uint64_t context_switches;  /* Context switch count */
    _Atomic uint64_t syscalls;          /* System call count */
    _Atomic uint64_t ipc_sends;         /* IPC send count */
    _Atomic uint64_t ipc_recvs;         /* IPC receive count */
} ProcessStats;

/**
 * @brief Process Control Block (PCB)
 * 
 * Main structure representing a process in the system.
 * Uses C23 atomics for lock-free state management.
 * Cache-line aligned for performance.
 */
typedef struct ProcessControlBlock {
    /* Process identification */
    ProcessId pid;                      /* Process ID */
    ProcessId parent_pid;               /* Parent process ID */
    ProcessId pgid;                     /* Process group ID */
    ProcessId sid;                      /* Session ID */
    
    /* Process state (atomic) */
    _Atomic uint32_t state;             /* Current state */
    _Atomic uint32_t flags;             /* Process flags */
    
    /* Priority and scheduling */
    uint8_t priority;                   /* Process priority */
    uint8_t nice;                       /* Nice value */
    int cpu_affinity;                   /* CPU affinity */
    int numa_node;                      /* NUMA node affinity */
    
    /* Reference counting (atomic) */
    _Atomic uint32_t ref_count;         /* Reference count */
    
    /* Process name */
    char name[MAX_PROCESS_NAME];        /* Process name */
    
    /* Computation graph */
    BdiGraph *graph;                    /* The computation graph */
    
    /* Memory management */
    MemoryRegion *memory_regions;       /* Memory region list */
    void *page_table;                   /* Page table pointer */
    size_t heap_size;                   /* Heap size */
    size_t stack_size;                  /* Stack size */
    
    /* File descriptors */
    FileDescriptor *fd_table;           /* File descriptor table */
    uint32_t num_fds;                   /* Number of open files */
    
    /* IPC handles */
    struct ipc_handle **ipc_handles;    /* IPC handle array */
    uint32_t num_ipc_handles;           /* Number of IPC handles */
    
    /* Capabilities */
    CapabilitySet capabilities;         /* Process capabilities */
    
    /* Parent-child relationships */
    struct ProcessControlBlock *parent; /* Parent PCB pointer */
    struct ProcessControlBlock **children; /* Child PCB array */
    uint32_t num_children;              /* Number of children */
    
    /* Exit status */
    ExitStatus exit_status;             /* Exit status (for zombies) */
    
    /* Statistics */
    ProcessStats stats;                 /* Process statistics */
    
    /* Timestamps */
    uint64_t creation_time;             /* Creation timestamp */
    uint64_t exit_time;                 /* Exit timestamp */
    
    /* Padding to cache line boundary */
    uint8_t padding[64 - ((sizeof(ProcessId) * 4 + 
                          sizeof(_Atomic uint32_t) * 3 + 
                          sizeof(uint8_t) * 2 + 
                          sizeof(int) * 2 + 
                          sizeof(char) * MAX_PROCESS_NAME) % 64)];
} __attribute__((aligned(64))) ProcessControlBlock;

/* Compile-time validations */
_Static_assert(sizeof(ProcessControlBlock) % 64 == 0, 
               "PCB must be cache-line aligned");
_Static_assert(MAX_PROCESSES > 0 && MAX_PROCESSES <= 65536, 
               "Process count limits");
_Static_assert(MAX_PROCESS_NAME >= 16, 
               "Process name must be at least 16 characters");

/* ===================================================================
 * Process Table Structure
 * =================================================================== */

/**
 * @brief Lock-free process table
 * 
 * Uses atomic operations for lock-free process management.
 */
typedef struct {
    /* Process table array */
    ProcessControlBlock *processes[MAX_PROCESSES];
    
    /* Atomic PID allocation */
    _Atomic uint64_t next_pid;
    
    /* Process counts (atomic) */
    _Atomic uint32_t total_processes;
    _Atomic uint32_t active_processes;
    _Atomic uint32_t zombie_processes;
    
    /* Statistics */
    _Atomic uint64_t total_forks;
    _Atomic uint64_t total_execs;
    _Atomic uint64_t total_exits;
    _Atomic uint64_t total_waits;
    
    /* Padding to cache line boundary */
    uint8_t padding[64];
} __attribute__((aligned(64))) ProcessTable;

/* ===================================================================
 * Process Management Functions
 * =================================================================== */

/**
 * @brief Initialize process management subsystem
 * 
 * @return 0 on success, negative error code otherwise
 */
[[nodiscard]] int process_init(void);

/**
 * @brief Shutdown process management subsystem
 */
void process_shutdown(void);

/**
 * @brief Allocate a new PCB
 * 
 * @return Pointer to allocated PCB, or nullptr on failure
 */
[[nodiscard]] ProcessControlBlock *pcb_alloc(void);

/**
 * @brief Free a PCB
 * 
 * @param pcb PCB to free
 */
void pcb_free(ProcessControlBlock *pcb);

/* Free a memory region with proper COW refcount handling */
void free_memory_region(MemoryRegion *region);

/**
 * @brief Increment PCB reference count
 * 
 * @param pcb PCB to reference
 * @return New reference count
 */
uint32_t pcb_ref(ProcessControlBlock *pcb);

/**
 * @brief Decrement PCB reference count
 * 
 * @param pcb PCB to unreference
 * @return New reference count
 */
uint32_t pcb_unref(ProcessControlBlock *pcb);

/**
 * @brief Allocate a new PID atomically
 * 
 * @return New process ID
 */
[[nodiscard]] ProcessId pid_alloc(void);

/**
 * @brief Find process by PID (lock-free)
 * 
 * @param pid Process ID to find
 * @return Pointer to PCB, or nullptr if not found
 */
[[nodiscard]] ProcessControlBlock *process_find(ProcessId pid);

/**
 * @brief Insert process into process table
 * 
 * @param pcb Process control block to insert
 * @return 0 on success, negative error code on failure
 */
[[nodiscard]] int process_insert(ProcessControlBlock *pcb);

/**
 * @brief Get current process
 * 
 * @return Pointer to current PCB
 */
[[nodiscard]] ProcessControlBlock *process_current(void);

/**
 * @brief Set current process
 * 
 * @param pcb PCB to set as current
 */
void process_set_current(ProcessControlBlock *pcb);

/**
 * @brief Get process state
 * 
 * @param pcb Process control block
 * @return Current process state
 */
ProcessState process_get_state(const ProcessControlBlock *pcb);

/**
 * @brief Set process state (atomic)
 * 
 * @param pcb Process control block
 * @param new_state New state
 * @return true if state changed, false otherwise
 */
bool process_set_state(ProcessControlBlock *pcb, ProcessState new_state);

/**
 * @brief Compare and exchange process state (atomic)
 * 
 * @param pcb Process control block
 * @param expected Expected current state
 * @param desired Desired new state
 * @return true if state changed, false otherwise
 */
bool process_cas_state(ProcessControlBlock *pcb,
                       ProcessState expected,
                       ProcessState desired);

/**
 * @brief Get process statistics
 * 
 * @param pcb Process control block
 * @param stats Pointer to store statistics
 */
void process_get_stats(const ProcessControlBlock *pcb, ProcessStats *stats);

/**
 * @brief Print process information
 * 
 * @param pcb Process control block
 */
void process_print_info(const ProcessControlBlock *pcb);

/**
 * @brief Get process state name
 * 
 * @param state Process state
 * @return State name string
 */
const char *process_state_name(ProcessState state);

/* ===================================================================
 * Process Lifecycle Functions (Implemented in process_lifecycle.c)
 * =================================================================== */

/**
 * @brief Fork current process (create child)
 * 
 * Creates a new process as a copy of the current process.
 * Uses Copy-On-Write (COW) for memory efficiency.
 * 
 * @return Child PID in parent, 0 in child, negative error code on failure
 */
[[nodiscard]] ProcessId process_fork(void);

/**
 * @brief Execute a new program
 * 
 * Replaces the current process image with a new program.
 * 
 * @param graph_path Path to the computation graph
 * @param argv Argument vector
 * @param envp Environment vector
 * @return 0 on success (doesn't return on success), negative error code on failure
 */
[[nodiscard]] int process_exec(const char *graph_path, 
                               char *const argv[], 
                               char *const envp[]);

/**
 * @brief Wait for child process to exit
 * 
 * @param pid Child PID to wait for (-1 for any child)
 * @param status Pointer to store exit status
 * @param options Wait options (WAIT_NOHANG, etc.)
 * @return Child PID on success, 0 if NOHANG and no child, negative error code on failure
 */
[[nodiscard]] ProcessId process_wait(ProcessId pid, 
                                     ExitStatus *status, 
                                     uint32_t options);

/**
 * @brief Exit current process
 * 
 * @param exit_code Exit code
 */
void process_exit(int exit_code) __attribute__((noreturn));

/**
 * @brief Kill a process
 * 
 * @param pid Process ID to kill
 * @param signal Signal to send
 * @return 0 on success, negative error code on failure
 */
[[nodiscard]] int process_kill(ProcessId pid, int signal);

/* ===================================================================
 * Process IPC Functions (Implemented in process_ipc.c)
 * =================================================================== */

/**
 * @brief Send message to process
 * 
 * Uses zero-copy IPC from Phase 4.
 * 
 * @param pcb Process control block
 * @param data Data to send
 * @param size Data size
 * @param flags IPC flags
 * @return Number of bytes sent, or negative error code
 */
[[nodiscard]] ssize_t process_send(ProcessControlBlock *pcb,
                                   const void *data,
                                   size_t size,
                                   uint32_t flags);

/**
 * @brief Receive message from process
 * 
 * Uses zero-copy IPC from Phase 4.
 * 
 * @param pcb Process control block
 * @param buffer Buffer to receive data
 * @param size Buffer size
 * @param flags IPC flags
 * @return Number of bytes received, or negative error code
 */
[[nodiscard]] ssize_t process_recv(ProcessControlBlock *pcb,
                                   void *buffer,
                                   size_t size,
                                   uint32_t flags);

/**
 * @brief Create shared memory region between processes
 * 
 * @param pcb1 First process
 * @param pcb2 Second process
 * @param size Shared memory size
 * @param flags Memory flags
 * @return Pointer to shared memory, or nullptr on failure
 */
[[nodiscard]] void *process_create_shm(ProcessControlBlock *pcb1,
                                       ProcessControlBlock *pcb2,
                                       size_t size,
                                       uint32_t flags);

/**
 * @brief Destroy shared memory region
 * 
 * @param pcb Process control block
 * @param shm_ptr Shared memory pointer
 * @return 0 on success, negative error code on failure
 */
[[nodiscard]] int process_destroy_shm(ProcessControlBlock *pcb, void *shm_ptr);

#endif /* BDI_PROCESS_H */
