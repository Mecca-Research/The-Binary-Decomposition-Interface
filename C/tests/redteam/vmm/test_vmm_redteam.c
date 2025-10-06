
/**
 * @file test_vmm_redteam.c
 * @brief Red-Team Tests for Virtual Memory Manager (VMM)
 * @details Region overflow, non-aligned mappings, overlapping ranges,
 *          page table integrity, and partial cleanup testing.
 * 
 * @author BDI Kernel Team - Red-Team Testing Initiative
 * @date 2024
 * @standard C23
 */

#include "../common/redteam_harness.h"
#include "../common/fuzzing_utils.h"
#include "../../../kernel/vmm.h"
#include "../../../kernel/memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ============================================================================
// Test: Region Count Overflow
// ============================================================================

REDTEAM_TEST(vmm_region_overflow, "vmm",
             "Test region count overflow handling") {
    const uint32_t max_regions = 10000;
    uint64_t *regions = malloc(max_regions * sizeof(uint64_t));
    uint32_t region_count = 0;
    
    REDTEAM_ASSERT_NOT_NULL(regions, "Failed to allocate tracking array");
    
    // Try to create many regions
    for (uint32_t i = 0; i < max_regions; i++) {
        size_t size = PAGE_SIZE * (i + 1);
        uint64_t vaddr = vmm_map_region(size, PROT_READ | PROT_WRITE, 
                                        MAP_PRIVATE | MAP_ANONYMOUS);
        
        if (vaddr != 0) {
            regions[region_count++] = vaddr;
        } else {
            // Region limit reached
            break;
        }
    }
    
    redteam_log("Created %u regions before limit", region_count);
    
    // Cleanup
    for (uint32_t i = 0; i < region_count; i++) {
        vmm_unmap_region(regions[i], PAGE_SIZE * (i + 1));
    }
    
    free(regions);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Non-Page-Aligned Mappings
// ============================================================================

REDTEAM_TEST(vmm_unaligned_mappings, "vmm",
             "Test non-page-aligned mapping detection") {
    
    // Try to map with unaligned addresses
    uint64_t unaligned_addrs[] = {
        0x1001,         // +1 byte
        0x2FFF,         // -1 byte
        0x3800,         // Half page
        0x4123,         // Random offset
    };
    
    for (size_t i = 0; i < sizeof(unaligned_addrs) / sizeof(unaligned_addrs[0]); i++) {
        // This should fail or be rounded to page boundary
        uint64_t vaddr = vmm_map_region_at(unaligned_addrs[i], PAGE_SIZE,
                                           PROT_READ | PROT_WRITE,
                                           MAP_PRIVATE | MAP_FIXED);
        
        if (vaddr != 0) {
            // If mapping succeeded, verify it's page-aligned
            REDTEAM_ASSERT((vaddr & (PAGE_SIZE - 1)) == 0,
                          "Mapping not page-aligned");
            vmm_unmap_region(vaddr, PAGE_SIZE);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Overlapping Range Protection
// ============================================================================

REDTEAM_TEST(vmm_overlapping_ranges, "vmm",
             "Test overlapping range detection") {
    
    // Create initial mapping
    uint64_t vaddr1 = vmm_map_region(PAGE_SIZE * 4, PROT_READ | PROT_WRITE,
                                     MAP_PRIVATE | MAP_ANONYMOUS);
    REDTEAM_ASSERT(vaddr1 != 0, "Failed to create initial mapping");
    
    // Try to create overlapping mapping
    uint64_t vaddr2 = vmm_map_region_at(vaddr1 + PAGE_SIZE, PAGE_SIZE * 2,
                                        PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_FIXED);
    
    // This should either fail or replace the existing mapping
    if (vaddr2 != 0) {
        vmm_unmap_region(vaddr2, PAGE_SIZE * 2);
    }
    
    vmm_unmap_region(vaddr1, PAGE_SIZE * 4);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Page Table Integrity
// ============================================================================

REDTEAM_TEST(vmm_page_table_integrity, "vmm",
             "Test page table integrity after operations") {
    const uint32_t num_mappings = 100;
    
    for (uint32_t i = 0; i < num_mappings; i++) {
        size_t size = PAGE_SIZE * (1 + (i % 16));
        
        // Create mapping
        uint64_t vaddr = vmm_map_region(size, PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS);
        
        if (vaddr != 0) {
            // Write to mapping to ensure page tables are set up
            uint8_t *ptr = (uint8_t *)vaddr;
            ptr[0] = 0xAA;
            ptr[size - 1] = 0xBB;
            
            // Verify writes
            REDTEAM_ASSERT(ptr[0] == 0xAA, "Write verification failed");
            REDTEAM_ASSERT(ptr[size - 1] == 0xBB, "Write verification failed");
            
            // Unmap
            vmm_unmap_region(vaddr, size);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Partial Mapping Cleanup
// ============================================================================

REDTEAM_TEST(vmm_partial_cleanup, "vmm",
             "Test partial mapping cleanup") {
    
    // Create large mapping
    size_t total_size = PAGE_SIZE * 16;
    uint64_t vaddr = vmm_map_region(total_size, PROT_READ | PROT_WRITE,
                                    MAP_PRIVATE | MAP_ANONYMOUS);
    REDTEAM_ASSERT(vaddr != 0, "Failed to create mapping");
    
    // Unmap middle portion
    uint64_t middle = vaddr + (PAGE_SIZE * 4);
    vmm_unmap_region(middle, PAGE_SIZE * 8);
    
    // Unmap remaining portions
    vmm_unmap_region(vaddr, PAGE_SIZE * 4);
    vmm_unmap_region(vaddr + (PAGE_SIZE * 12), PAGE_SIZE * 4);
    
    return TEST_PASS;
}

// ============================================================================
// Test: Protection Flags
// ============================================================================

REDTEAM_TEST(vmm_protection_flags, "vmm",
             "Test various protection flag combinations") {
    
    uint32_t prot_flags[] = {
        PROT_NONE,
        PROT_READ,
        PROT_WRITE,
        PROT_EXEC,
        PROT_READ | PROT_WRITE,
        PROT_READ | PROT_EXEC,
        PROT_READ | PROT_WRITE | PROT_EXEC,
    };
    
    for (size_t i = 0; i < sizeof(prot_flags) / sizeof(prot_flags[0]); i++) {
        uint64_t vaddr = vmm_map_region(PAGE_SIZE, prot_flags[i],
                                        MAP_PRIVATE | MAP_ANONYMOUS);
        
        if (vaddr != 0) {
            // TODO: Verify protection flags are enforced
            vmm_unmap_region(vaddr, PAGE_SIZE);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Mapping Flags
// ============================================================================

REDTEAM_TEST(vmm_mapping_flags, "vmm",
             "Test various mapping flag combinations") {
    
    uint32_t map_flags[] = {
        MAP_PRIVATE | MAP_ANONYMOUS,
        MAP_SHARED | MAP_ANONYMOUS,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_LOCKED,
    };
    
    for (size_t i = 0; i < sizeof(map_flags) / sizeof(map_flags[0]); i++) {
        uint64_t vaddr = vmm_map_region(PAGE_SIZE, PROT_READ | PROT_WRITE,
                                        map_flags[i]);
        
        if (vaddr != 0) {
            vmm_unmap_region(vaddr, PAGE_SIZE);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Large Mapping Stress
// ============================================================================

REDTEAM_TEST(vmm_large_mappings, "vmm",
             "Stress test with large mappings") {
    
    size_t large_sizes[] = {
        1024 * 1024,        // 1 MB
        16 * 1024 * 1024,   // 16 MB
        64 * 1024 * 1024,   // 64 MB
    };
    
    for (size_t i = 0; i < sizeof(large_sizes) / sizeof(large_sizes[0]); i++) {
        uint64_t vaddr = vmm_map_region(large_sizes[i], PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS);
        
        if (vaddr != 0) {
            // Write to first and last pages
            uint8_t *ptr = (uint8_t *)vaddr;
            ptr[0] = 0xCC;
            ptr[large_sizes[i] - 1] = 0xDD;
            
            vmm_unmap_region(vaddr, large_sizes[i]);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: Rapid Map/Unmap
// ============================================================================

REDTEAM_TEST(vmm_rapid_map_unmap, "vmm",
             "Stress test with rapid mapping and unmapping") {
    const uint32_t iterations = 1000;
    
    for (uint32_t i = 0; i < iterations; i++) {
        size_t size = PAGE_SIZE * (1 + (i % 8));
        
        uint64_t vaddr = vmm_map_region(size, PROT_READ | PROT_WRITE,
                                        MAP_PRIVATE | MAP_ANONYMOUS);
        
        if (vaddr != 0) {
            vmm_unmap_region(vaddr, size);
        }
    }
    
    return TEST_PASS;
}

// ============================================================================
// Test: TLB Stress
// ============================================================================

REDTEAM_TEST(vmm_tlb_stress, "vmm",
             "Stress test TLB with many mappings") {
    const uint32_t num_mappings = 1000;
    uint64_t *mappings = malloc(num_mappings * sizeof(uint64_t));
    
    REDTEAM_ASSERT_NOT_NULL(mappings, "Failed to allocate tracking array");
    
    // Create many mappings
    for (uint32_t i = 0; i < num_mappings; i++) {
        mappings[i] = vmm_map_region(PAGE_SIZE, PROT_READ | PROT_WRITE,
                                     MAP_PRIVATE | MAP_ANONYMOUS);
    }
    
    // Access all mappings to stress TLB
    for (uint32_t i = 0; i < num_mappings; i++) {
        if (mappings[i] != 0) {
            uint8_t *ptr = (uint8_t *)mappings[i];
            ptr[0] = (uint8_t)i;
        }
    }
    
    // Cleanup
    for (uint32_t i = 0; i < num_mappings; i++) {
        if (mappings[i] != 0) {
            vmm_unmap_region(mappings[i], PAGE_SIZE);
        }
    }
    
    free(mappings);
    
    return TEST_PASS;
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int argc, char **argv) {
    if (!redteam_init(true, "vmm_redteam.log")) {
        fprintf(stderr, "Failed to initialize test harness\n");
        return 1;
    }
    
    fuzz_init(time(NULL));
    
    // Register tests
    redteam_register_test(&test_case_vmm_region_overflow);
    redteam_register_test(&test_case_vmm_unaligned_mappings);
    redteam_register_test(&test_case_vmm_overlapping_ranges);
    redteam_register_test(&test_case_vmm_page_table_integrity);
    redteam_register_test(&test_case_vmm_partial_cleanup);
    redteam_register_test(&test_case_vmm_protection_flags);
    redteam_register_test(&test_case_vmm_mapping_flags);
    redteam_register_test(&test_case_vmm_large_mappings);
    redteam_register_test(&test_case_vmm_rapid_map_unmap);
    redteam_register_test(&test_case_vmm_tlb_stress);
    
    // Run tests
    test_stats_t stats = redteam_run_all_tests();
    
    redteam_print_stats(&stats);
    redteam_cleanup();
    
    return (stats.failed + stats.crashed + stats.leaked) > 0 ? 1 : 0;
}
