#!/usr/bin/env python3
"""
Phase 1.2-1.4: Add C23 attributes, static assertions, and type inference
"""

import os
import re
from pathlib import Path

# Statistics
stats = {
    'nodiscard_added': 0,
    'maybe_unused_added': 0,
    'noreturn_added': 0,
    'fallthrough_added': 0,
    'static_assert_added': 0,
    'typeof_added': 0,
    'files_modified': 0
}

def add_compat_header(filepath):
    """Add c23_compat.h include if not present"""
    with open(filepath, 'r') as f:
        content = f.read()
    
    if 'c23_compat.h' in content:
        return content, False
    
    # Find the first #include and add our header before it
    lines = content.split('\n')
    new_lines = []
    header_added = False
    
    for i, line in enumerate(lines):
        if not header_added and line.strip().startswith('#include'):
            new_lines.append('#include "c23_compat.h"')
            header_added = True
        new_lines.append(line)
    
    if not header_added:
        # No includes found, add at the beginning after comments
        for i, line in enumerate(lines):
            if not line.strip().startswith('//') and not line.strip().startswith('/*') and line.strip():
                new_lines.insert(i, '#include "c23_compat.h"')
                break
    
    return '\n'.join(new_lines), True

def add_nodiscard_attribute(content):
    """Add [[nodiscard]] to functions returning pointers or error codes"""
    modified = False
    
    # Patterns for functions that should have nodiscard
    patterns = [
        # Functions returning pointers
        (r'(\n)([\w\s\*]+\*\s+)(\w+_(?:create|alloc|malloc|new|get|find|search|lookup)\s*\([^)]*\))', r'\1[[nodiscard]] \2\3'),
        # Functions returning int (likely error codes)
        (r'(\n)(int\s+)(\w+_(?:init|execute|process|compile|parse|analyze|translate|run|start|stop)\s*\([^)]*\))', r'\1[[nodiscard]] \2\3'),
        # Functions returning bool
        (r'(\n)(bool\s+)(\w+_(?:is|has|can|should|validate|check|verify)\s*\([^)]*\))', r'\1[[nodiscard]] \2\3'),
    ]
    
    for pattern, replacement in patterns:
        new_content, count = re.subn(pattern, replacement, content)
        if count > 0:
            content = new_content
            stats['nodiscard_added'] += count
            modified = True
    
    return content, modified

def add_fallthrough_attribute(content):
    """Add [[fallthrough]] to switch case statements"""
    modified = False
    
    # Find switch statements with potential fallthrough
    # This is a simplified pattern - may need manual review
    pattern = r'(case\s+\w+:\s*\n\s*[^b][^r][^e][^a][^k].*?\n)(\s*case\s+)'
    
    def add_fallthrough(match):
        stats['fallthrough_added'] += 1
        return match.group(1) + '        [[fallthrough]];\n' + match.group(2)
    
    new_content = re.sub(pattern, add_fallthrough, content, flags=re.MULTILINE)
    if new_content != content:
        modified = True
        content = new_content
    
    return content, modified

def add_static_assertions(filepath, content):
    """Add static assertions to header files"""
    if not filepath.endswith('.h'):
        return content, False
    
    modified = False
    
    # Look for struct definitions and add size assertions
    struct_pattern = r'typedef\s+struct\s+(\w+)\s*\{([^}]+)\}\s*(\w+);'
    
    matches = list(re.finditer(struct_pattern, content))
    if matches:
        # Add assertions at the end of the file
        assertions = ['\n// Compile-time struct size checks']
        for match in matches:
            struct_name = match.group(3) or match.group(1)
            assertions.append(f'static_assert(sizeof({struct_name}) > 0, "{struct_name} must have non-zero size");')
            stats['static_assert_added'] += 1
        
        # Add before the final #endif if present
        if content.rstrip().endswith('#endif'):
            lines = content.rstrip().split('\n')
            insert_pos = len(lines) - 1
            for assertion in assertions:
                lines.insert(insert_pos, assertion)
                insert_pos += 1
            content = '\n'.join(lines) + '\n'
            modified = True
    
    return content, modified

def process_file(filepath):
    """Process a single C/H file"""
    try:
        with open(filepath, 'r') as f:
            content = f.read()
        
        original_content = content
        file_modified = False
        
        # Add compat header
        content, modified = add_compat_header(filepath)
        file_modified = file_modified or modified
        
        # Add nodiscard attributes
        content, modified = add_nodiscard_attribute(content)
        file_modified = file_modified or modified
        
        # Add fallthrough attributes
        content, modified = add_fallthrough_attribute(content)
        file_modified = file_modified or modified
        
        # Add static assertions (headers only)
        content, modified = add_static_assertions(filepath, content)
        file_modified = file_modified or modified
        
        # Write back if modified
        if file_modified and content != original_content:
            with open(filepath, 'w') as f:
                f.write(content)
            stats['files_modified'] += 1
            return True
        
        return False
    
    except Exception as e:
        print(f"Error processing {filepath}: {e}")
        return False

def main():
    """Main processing function"""
    print("=" * 60)
    print("Phase 1.2-1.4: Adding C23 Features")
    print("=" * 60)
    print()
    
    # Find all C and H files
    c_dir = Path('C')
    files = list(c_dir.rglob('*.c')) + list(c_dir.rglob('*.h'))
    
    print(f"Processing {len(files)} files...")
    print()
    
    for filepath in files:
        if process_file(str(filepath)):
            print(f"  ✓ Modified: {filepath}")
    
    print()
    print("=" * 60)
    print("Phase 1.2-1.4 Complete!")
    print("=" * 60)
    print()
    print("Statistics:")
    print(f"  Files modified: {stats['files_modified']}")
    print(f"  [[nodiscard]] added: {stats['nodiscard_added']}")
    print(f"  [[fallthrough]] added: {stats['fallthrough_added']}")
    print(f"  static_assert added: {stats['static_assert_added']}")
    print()

if __name__ == '__main__':
    main()
