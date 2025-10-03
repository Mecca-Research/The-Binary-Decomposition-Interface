// BTL ISA Tests
#include "../btl/btl_isa.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static int tests_passed = 0;

#define TEST(name) printf("Testing %s... ", name); fflush(stdout);
#define PASS() printf("PASS\n"); tests_passed++;

void test_architecture_detection(void) {
    TEST("architecture detection");
    
    BTL_Architecture arch = btl_detect_architecture();
    const char *name = btl_architecture_name(arch);
    
    printf("Detected: %s... ", name);
    assert(name != NULL);
    
    PASS();
}

void test_x86_64_instructions(void) {
    TEST("x86-64 instruction lookup");
    
    // Test ADD instruction
    const BTL_InstructionDescriptor *add = btl_x86_64_get_instruction(0x01);
    assert(strcmp(add->mnemonic, "ADD") == 0);
    assert(add->category == BTL_CAT_ARITHMETIC);
    
    // Test MOV instruction
    const BTL_InstructionDescriptor *mov = btl_x86_64_get_instruction(0x89);
    assert(strcmp(mov->mnemonic, "MOV") == 0);
    assert(mov->category == BTL_CAT_MEMORY);
    
    // Test JMP instruction
    const BTL_InstructionDescriptor *jmp = btl_x86_64_get_instruction(0xE9);
    assert(strcmp(jmp->mnemonic, "JMP") == 0);
    assert(jmp->category == BTL_CAT_CONTROL);
    
    PASS();
}

void test_instruction_categories(void) {
    TEST("instruction categories");
    
    assert(btl_x86_64_get_category(0x01) == BTL_CAT_ARITHMETIC);
    assert(btl_x86_64_get_category(0x31) == BTL_CAT_LOGIC);
    assert(btl_x86_64_get_category(0x89) == BTL_CAT_MEMORY);
    assert(btl_x86_64_get_category(0x74) == BTL_CAT_CONTROL);
    
    PASS();
}

void test_mnemonic_lookup(void) {
    TEST("mnemonic lookup");
    
    assert(strcmp(btl_x86_64_get_mnemonic(0x01), "ADD") == 0);
    assert(strcmp(btl_x86_64_get_mnemonic(0x31), "XOR") == 0);
    assert(strcmp(btl_x86_64_get_mnemonic(0x89), "MOV") == 0);
    assert(strcmp(btl_x86_64_get_mnemonic(0xC3), "RET") == 0);
    
    PASS();
}

int main(void) {
    printf("=== BTL ISA Tests ===\n\n");
    
    test_architecture_detection();
    test_x86_64_instructions();
    test_instruction_categories();
    test_mnemonic_lookup();
    
    printf("\n=== Results ===\n");
    printf("Passed: %d\n", tests_passed);
    return 0;
}
