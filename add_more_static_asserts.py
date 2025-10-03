#!/usr/bin/env python3
"""Add comprehensive static assertions to header files"""

import re

# Add static assertions to key headers
headers_to_enhance = {
    'C/kernel/device/device.h': [
        'static_assert(sizeof(void*) >= 4, "Pointer must be at least 4 bytes");',
        'static_assert(sizeof(int) >= 4, "int must be at least 4 bytes");',
        'static_assert(sizeof(size_t) >= sizeof(int), "size_t must be at least as large as int");',
    ],
    'C/vm/bci_vm.h': [
        'static_assert(sizeof(void*) >= 4, "VM requires at least 32-bit pointers");',
        'static_assert(sizeof(double) == 8, "VM requires 64-bit doubles");',
    ],
    'C/compiler/lexer/bci_token.h': [
        'static_assert(sizeof(int) >= 4, "Token types require at least 32-bit int");',
    ],
    'C/kernel/scheduler/scheduler.h': [
        'static_assert(sizeof(void*) >= 4, "Scheduler requires at least 32-bit pointers");',
    ],
}

count = 0
for filepath, assertions in headers_to_enhance.items():
    try:
        with open(filepath, 'r') as f:
            content = f.read()
        
        # Check if assertions already exist
        has_assertions = any(assertion in content for assertion in assertions)
        if has_assertions:
            continue
        
        # Add before final #endif
        if content.rstrip().endswith('#endif'):
            lines = content.rstrip().split('\n')
            insert_pos = len(lines) - 1
            
            # Add comment header
            lines.insert(insert_pos, '')
            lines.insert(insert_pos + 1, '// Compile-time invariants')
            insert_pos += 2
            
            for assertion in assertions:
                lines.insert(insert_pos, assertion)
                insert_pos += 1
                count += 1
            
            content = '\n'.join(lines) + '\n'
            
            with open(filepath, 'w') as f:
                f.write(content)
            
            print(f"✓ Added {len(assertions)} assertions to {filepath}")
    
    except FileNotFoundError:
        print(f"⚠ File not found: {filepath}")
    except Exception as e:
        print(f"✗ Error processing {filepath}: {e}")

print(f"\nTotal static assertions added: {count}")
