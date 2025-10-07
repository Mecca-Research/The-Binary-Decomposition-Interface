
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "../include/bdi/drivers/ioapic.h"

static bool irq_fired = false;
static uint32_t irq_count = 0;

void test_irq_handler(uint32_t irq, void *data) {
    irq_fired = true;
    irq_count++;
}

void test_ioapic_init(void) {
    printf("Testing IOAPIC initialization...\n");
    
    int ret = ioapic_init();
    assert(ret == 0 && "IOAPIC initialization failed");
    
    printf("  ✓ IOAPIC subsystem initialized\n");
}

void test_ioapic_register(void) {
    printf("Testing IOAPIC registration...\n");
    
    // Register IOAPIC at standard address
    int ret = ioapic_register(0, 0xFEC00000, 0);
    assert(ret == 0 && "IOAPIC registration failed");
    
    ioapic_t *ioapic = ioapic_get(0);
    assert(ioapic != NULL && "IOAPIC not found");
    assert(ioapic->initialized && "IOAPIC not initialized");
    
    printf("  ✓ IOAPIC registered: %u entries\n", ioapic->num_entries);
}

void test_ioapic_redir_table(void) {
    printf("Testing IOAPIC redirection table...\n");
    
    ioapic_t *ioapic = ioapic_get(0);
    assert(ioapic != NULL);
    
    // Read initial entry
    uint64_t entry = ioapic_read_redir_entry(ioapic, 0);
    assert((entry & IOAPIC_MASK) != 0 && "Entry should be masked initially");
    
    // Write test entry
    uint64_t test_entry = 0x20 | IOAPIC_MASK;  // Vector 0x20, masked
    ioapic_write_redir_entry(ioapic, 0, test_entry);
    
    // Read back
    entry = ioapic_read_redir_entry(ioapic, 0);
    assert((entry & 0xFF) == 0x20 && "Vector not set correctly");
    
    printf("  ✓ Redirection table working\n");
}

void test_ioapic_irq_routing(void) {
    printf("Testing IRQ routing...\n");
    
    int ret = ioapic_set_irq(
        16,                          // GSI
        0x30,                        // Vector
        0,                           // Destination
        IOAPIC_DELIVERY_FIXED,
        IOAPIC_DEST_PHYSICAL,
        IOAPIC_TRIGGER_LEVEL,
        IOAPIC_POLARITY_LOW
    );
    assert(ret == 0 && "IRQ routing failed");
    
    ioapic_t *ioapic = ioapic_get_by_gsi(16);
    assert(ioapic != NULL && "IOAPIC not found for GSI");
    
    uint64_t entry = ioapic_read_redir_entry(ioapic, 16);
    assert((entry & 0xFF) == 0x30 && "Vector not set");
    assert((entry & IOAPIC_TRIGGER_LEVEL) != 0 && "Trigger mode not set");
    
    printf("  ✓ IRQ routing configured\n");
}

void test_ioapic_masking(void) {
    printf("Testing IRQ masking...\n");
    
    // Mask IRQ
    int ret = ioapic_mask_irq(16);
    assert(ret == 0 && "IRQ masking failed");
    
    ioapic_t *ioapic = ioapic_get_by_gsi(16);
    uint64_t entry = ioapic_read_redir_entry(ioapic, 16);
    assert((entry & IOAPIC_MASK) != 0 && "IRQ not masked");
    
    // Unmask IRQ
    ret = ioapic_unmask_irq(16);
    assert(ret == 0 && "IRQ unmasking failed");
    
    entry = ioapic_read_redir_entry(ioapic, 16);
    assert((entry & IOAPIC_MASK) == 0 && "IRQ not unmasked");
    
    printf("  ✓ IRQ masking working\n");
}

void test_ioapic_handler(void) {
    printf("Testing IRQ handler registration...\n");
    
    irq_fired = false;
    irq_count = 0;
    
    int ret = ioapic_set_irq_handler(16, test_irq_handler, NULL);
    assert(ret == 0 && "Handler registration failed");
    
    printf("  ✓ IRQ handler registered\n");
}

void test_ioapic_legacy(void) {
    printf("Testing legacy IRQ support...\n");
    
    int ret = ioapic_setup_legacy_irq(1, 0x21);  // Keyboard
    assert(ret == 0 && "Legacy IRQ setup failed");
    
    ioapic_t *ioapic = ioapic_get_by_gsi(1);
    uint64_t entry = ioapic_read_redir_entry(ioapic, 1);
    assert((entry & 0xFF) == 0x21 && "Legacy vector not set");
    
    printf("  ✓ Legacy IRQ support working\n");
}

void test_ioapic_affinity(void) {
    printf("Testing IRQ affinity...\n");
    
    int ret = ioapic_set_irq_affinity(16, 0x03);  // CPUs 0 and 1
    assert(ret == 0 && "IRQ affinity failed");
    
    ioapic_t *ioapic = ioapic_get_by_gsi(16);
    uint64_t entry = ioapic_read_redir_entry(ioapic, 16);
    uint8_t dest = (entry >> 56) & 0xFF;
    assert(dest == 0x03 && "Affinity not set correctly");
    
    printf("  ✓ IRQ affinity working\n");
}

void test_ioapic_dump(void) {
    printf("Testing IOAPIC debugging...\n");
    
    ioapic_t *ioapic = ioapic_get(0);
    assert(ioapic != NULL);
    
    printf("\n");
    ioapic_dump_info(ioapic);
    printf("\n");
    
    printf("  ✓ IOAPIC debugging working\n");
}

int main(void) {
    printf("=== IOAPIC Subsystem Tests ===\n\n");
    
    test_ioapic_init();
    test_ioapic_register();
    test_ioapic_redir_table();
    test_ioapic_irq_routing();
    test_ioapic_masking();
    test_ioapic_handler();
    test_ioapic_legacy();
    test_ioapic_affinity();
    test_ioapic_dump();
    
    printf("\n=== All IOAPIC Tests Passed ===\n");
    
    return 0;
}
