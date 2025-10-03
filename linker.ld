
/* BDI Kernel Linker Script - Phase 6: Build System & Compiler Optimization
 * Optimized linker script with hot/cold section separation and alignment
 */

OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

/* Memory layout for kernel */
MEMORY
{
    /* Kernel code and data */
    KERNEL (rwx) : ORIGIN = 0x100000, LENGTH = 16M
    
    /* Hot data (frequently accessed) */
    HOT_DATA (rw) : ORIGIN = 0x1100000, LENGTH = 4M
    
    /* Cold data (rarely accessed) */
    COLD_DATA (rw) : ORIGIN = 0x1500000, LENGTH = 4M
    
    /* Stack */
    STACK (rw) : ORIGIN = 0x1900000, LENGTH = 1M
}

SECTIONS
{
    /* ====================================================================
     * Hot Code Section - Frequently executed code
     * ==================================================================== */
    
    .text.hot : ALIGN(4K)
    {
        *(.text.hot)
        *(.text.hot.*)
        
        /* Critical kernel functions */
        *(.text.scheduler*)
        *(.text.task*)
        *(.text.memory*)
        *(.text.ipc*)
        
        /* Fast path functions */
        *(.text.fastpath*)
        *(.text.inline*)
        
        /* Optimized intrinsics */
        *(.text.simd*)
        *(.text.avx*)
        
    } > KERNEL
    
    /* ====================================================================
     * Normal Code Section
     * ==================================================================== */
    
    .text : ALIGN(4K)
    {
        *(.text.startup)
        *(.text)
        *(.text.*)
        
        /* Keep related functions together */
        *(.text.graph*)
        *(.text.ham*)
        *(.text.integration*)
        
    } > KERNEL
    
    /* ====================================================================
     * Cold Code Section - Rarely executed code
     * ==================================================================== */
    
    .text.cold : ALIGN(4K)
    {
        *(.text.cold)
        *(.text.cold.*)
        *(.text.unlikely)
        *(.text.unlikely.*)
        
        /* Error handling and initialization */
        *(.text.init*)
        *(.text.error*)
        *(.text.panic*)
        
    } > KERNEL
    
    /* ====================================================================
     * Read-Only Data
     * ==================================================================== */
    
    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
        *(.rodata.str1.*)
        
        /* Constant data */
        *(.rodata.cst4)
        *(.rodata.cst8)
        *(.rodata.cst16)
        *(.rodata.cst32)
        
    } > KERNEL
    
    /* ====================================================================
     * Hot Data Section - Frequently accessed data
     * ==================================================================== */
    
    .data.hot : ALIGN(64)  /* Cache line aligned */
    {
        *(.data.hot)
        *(.data.hot.*)
        
        /* Per-CPU data */
        *(.data.percpu*)
        
        /* Scheduler data */
        *(.data.scheduler*)
        *(.data.task*)
        
        /* Lock-free structures */
        *(.data.lockfree*)
        *(.data.atomic*)
        
    } > HOT_DATA
    
    /* ====================================================================
     * Normal Data Section
     * ==================================================================== */
    
    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
        
        /* Global variables */
        *(.data.global*)
        
    } > HOT_DATA
    
    /* ====================================================================
     * BSS Section - Zero-initialized data
     * ==================================================================== */
    
    .bss : ALIGN(4K)
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
        
    } > HOT_DATA
    
    /* ====================================================================
     * Cold Data Section - Rarely accessed data
     * ==================================================================== */
    
    .data.cold : ALIGN(4K)
    {
        *(.data.cold)
        *(.data.cold.*)
        
        /* Initialization data */
        *(.data.init*)
        
        /* Debug and logging data */
        *(.data.debug*)
        *(.data.log*)
        
    } > COLD_DATA
    
    /* ====================================================================
     * Thread-Local Storage
     * ==================================================================== */
    
    .tdata : ALIGN(64)
    {
        *(.tdata)
        *(.tdata.*)
        
    } > HOT_DATA
    
    .tbss : ALIGN(64)
    {
        *(.tbss)
        *(.tbss.*)
        
    } > HOT_DATA
    
    /* ====================================================================
     * PGO and Profiling Data
     * ==================================================================== */
    
    .pgo_data : ALIGN(4K)
    {
        __pgo_data_start = .;
        *(.pgo_data)
        *(.pgo_data.*)
        __pgo_data_end = .;
        
    } > COLD_DATA
    
    /* ====================================================================
     * Stack Section
     * ==================================================================== */
    
    .stack : ALIGN(4K)
    {
        __stack_bottom = .;
        . += 1M;  /* 1MB stack */
        __stack_top = .;
        
    } > STACK
    
    /* ====================================================================
     * Debug Sections (not loaded into memory)
     * ==================================================================== */
    
    .debug_info     0 : { *(.debug_info) }
    .debug_abbrev   0 : { *(.debug_abbrev) }
    .debug_line     0 : { *(.debug_line) }
    .debug_str      0 : { *(.debug_str) }
    .debug_loc      0 : { *(.debug_loc) }
    .debug_ranges   0 : { *(.debug_ranges) }
    
    /* ====================================================================
     * Discard Sections
     * ==================================================================== */
    
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
    
    /* ====================================================================
     * Symbols for kernel use
     * ==================================================================== */
    
    __kernel_start = ORIGIN(KERNEL);
    __kernel_end = __stack_top;
    __kernel_size = __kernel_end - __kernel_start;
    
    __text_start = ADDR(.text.hot);
    __text_end = ADDR(.text.cold) + SIZEOF(.text.cold);
    
    __data_start = ADDR(.data.hot);
    __data_end = ADDR(.data.cold) + SIZEOF(.data.cold);
    
    __hot_data_start = ADDR(.data.hot);
    __hot_data_end = ADDR(.data.hot) + SIZEOF(.data.hot);
    
    __cold_data_start = ADDR(.data.cold);
    __cold_data_end = ADDR(.data.cold) + SIZEOF(.data.cold);
}
