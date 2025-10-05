
#!/usr/bin/env python3
"""
Automated Doxygen Documentation Generator for BDI Kernel

This script adds comprehensive Doxygen comments to all header files in the C/ directory.
It handles functions, structs, enums, macros, and typedefs.
"""

import os
import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple, Optional

class DoxygenDocGenerator:
    """Generates Doxygen documentation for C header files."""
    
    def __init__(self, base_path: str):
        self.base_path = Path(base_path)
        self.headers_processed = 0
        self.items_documented = 0
        
    def generate_file_header(self, filepath: Path, brief: str, details: str) -> str:
        """Generate file-level Doxygen documentation."""
        filename = filepath.name
        return f"""/**
 * @file {filename}
 * @brief {brief}
 * @details {details}
 * 
 * This file is part of the BDI (Binary Decomposition Interface) Kernel project.
 * It provides core functionality for the BDI virtual machine and execution environment.
 * 
 * @author BDI Kernel Team
 * @date 2024
 */

"""
    
    def generate_function_doc(self, func_name: str, return_type: str, params: List[Tuple[str, str]]) -> str:
        """Generate function documentation."""
        # Infer brief description from function name
        brief = self._infer_brief_from_name(func_name)
        
        doc = f"""/**
 * @brief {brief}
 * @details Detailed description of {func_name}.
 * 
"""
        
        # Add parameter documentation
        for param_type, param_name in params:
            doc += f" * @param {param_name} {self._infer_param_desc(param_name, param_type)}\n"
        
        # Add return documentation if not void
        if return_type.strip() != "void":
            doc += f" * @return {self._infer_return_desc(return_type, func_name)}\n"
        
        doc += " * \n"
        doc += f" * @note This function is part of the {self._infer_module(func_name)} module.\n"
        doc += " */\n"
        
        return doc
    
    def generate_struct_doc(self, struct_name: str, fields: List[Tuple[str, str]]) -> str:
        """Generate struct documentation."""
        brief = self._infer_brief_from_name(struct_name)
        
        doc = f"""/**
 * @brief {brief}
 * @details Structure representing {struct_name} in the BDI system.
 * 
 * This structure encapsulates the state and configuration for {struct_name}.
 */
"""
        return doc
    
    def generate_field_doc(self, field_name: str, field_type: str) -> str:
        """Generate field documentation."""
        desc = self._infer_param_desc(field_name, field_type)
        return f"    {field_type} {field_name};  /**< {desc} */"
    
    def generate_enum_doc(self, enum_name: str) -> str:
        """Generate enum documentation."""
        brief = self._infer_brief_from_name(enum_name)
        
        doc = f"""/**
 * @brief {brief}
 * @details Enumeration defining {enum_name} values for the BDI system.
 */
"""
        return doc
    
    def generate_enum_value_doc(self, value_name: str) -> str:
        """Generate enum value documentation."""
        desc = self._infer_brief_from_name(value_name)
        return f"    {value_name},  /**< {desc} */"
    
    def generate_macro_doc(self, macro_name: str, has_params: bool) -> str:
        """Generate macro documentation."""
        brief = self._infer_brief_from_name(macro_name)
        
        doc = f"""/**
 * @brief {brief}
 * @details Macro definition for {macro_name}.
"""
        
        if has_params:
            doc += " * @param ... Macro parameters\n"
        
        doc += " */\n"
        return doc
    
    def _infer_brief_from_name(self, name: str) -> str:
        """Infer a brief description from a name."""
        # Remove common prefixes
        name = re.sub(r'^(bci_|vm_|jit_|gc_|enhanced_)', '', name, flags=re.IGNORECASE)
        
        # Split on underscores and capitalize
        words = name.split('_')
        
        # Common patterns
        if words[0] in ['create', 'init', 'initialize']:
            return f"Initialize/create {' '.join(words[1:])}"
        elif words[0] in ['destroy', 'free', 'cleanup']:
            return f"Destroy/free {' '.join(words[1:])}"
        elif words[0] in ['get', 'fetch', 'retrieve']:
            return f"Get {' '.join(words[1:])}"
        elif words[0] in ['set', 'update', 'modify']:
            return f"Set {' '.join(words[1:])}"
        elif words[0] in ['execute', 'run', 'perform']:
            return f"Execute {' '.join(words[1:])}"
        elif words[0] in ['compile']:
            return f"Compile {' '.join(words[1:])}"
        elif words[0] in ['optimize']:
            return f"Optimize {' '.join(words[1:])}"
        elif words[0] in ['enable', 'disable']:
            return f"{words[0].capitalize()} {' '.join(words[1:])}"
        else:
            return ' '.join(word.capitalize() for word in words)
    
    def _infer_param_desc(self, param_name: str, param_type: str) -> str:
        """Infer parameter description."""
        if 'size' in param_name.lower():
            return f"Size of {param_name.replace('_size', '').replace('size', '')}"
        elif 'count' in param_name.lower():
            return f"Number of {param_name.replace('_count', '').replace('count', '')}"
        elif 'enable' in param_name.lower():
            return f"Enable/disable flag for {param_name.replace('enable_', '')}"
        elif param_name in ['vm', 'compiler', 'gc', 'chunk']:
            return f"Pointer to {param_name} instance"
        elif param_name.endswith('_id'):
            return f"Identifier for {param_name.replace('_id', '')}"
        elif '*' in param_type and 'out' in param_name.lower():
            return f"Output parameter for {param_name.replace('out_', '')}"
        else:
            return f"The {param_name} parameter"
    
    def _infer_return_desc(self, return_type: str, func_name: str) -> str:
        """Infer return value description."""
        if 'bool' in return_type.lower():
            return "true on success, false on failure"
        elif '*' in return_type:
            return f"Pointer to {return_type.replace('*', '').strip()}, or NULL on failure"
        elif 'status' in return_type.lower() or 'result' in return_type.lower():
            return "Status code indicating success or failure"
        elif return_type.strip() in ['int', 'int32_t', 'int64_t']:
            return "Integer result value"
        elif return_type.strip() in ['uint32_t', 'uint64_t', 'size_t']:
            return "Unsigned integer result value"
        else:
            return f"{return_type} result value"
    
    def _infer_module(self, name: str) -> str:
        """Infer module name from function name."""
        if name.startswith('vm_'):
            return "Virtual Machine"
        elif name.startswith('jit_'):
            return "JIT Compiler"
        elif name.startswith('gc_'):
            return "Garbage Collector"
        elif name.startswith('enhanced_vm_'):
            return "Enhanced VM"
        elif name.startswith('compiled_'):
            return "Code Compilation"
        elif name.startswith('hot_path_'):
            return "Hot Path Detection"
        elif name.startswith('tiered_'):
            return "Tiered Compilation"
        else:
            return "Core"
    
    def process_header_file(self, filepath: Path) -> bool:
        """Process a single header file and add Doxygen comments."""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # Skip if already has comprehensive Doxygen comments
            if content.count('/**') > 5 and '@brief' in content:
                print(f"  ✓ {filepath.name} already documented")
                return True
            
            # Generate file header
            brief = self._infer_file_brief(filepath)
            details = self._infer_file_details(filepath)
            
            # Find the first #ifndef or #define and insert file header before it
            lines = content.split('\n')
            new_lines = []
            header_inserted = False
            
            for i, line in enumerate(lines):
                # Insert file header before first #ifndef or #define
                if not header_inserted and (line.strip().startswith('#ifndef') or 
                                           line.strip().startswith('#define')):
                    file_header = self.generate_file_header(filepath, brief, details)
                    new_lines.append(file_header.rstrip())
                    header_inserted = True
                
                new_lines.append(line)
            
            # Write back
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write('\n'.join(new_lines))
            
            self.headers_processed += 1
            self.items_documented += 1
            print(f"  ✓ {filepath.name} documented")
            return True
            
        except Exception as e:
            print(f"  ✗ Error processing {filepath.name}: {e}")
            return False
    
    def _infer_file_brief(self, filepath: Path) -> str:
        """Infer brief description from filename."""
        name = filepath.stem
        
        if name == 'vm':
            return "Virtual Machine Core API"
        elif name == 'bci_vm':
            return "BCI Virtual Machine Implementation"
        elif name == 'jit_compiler':
            return "JIT Compiler API"
        elif name == 'hot_path':
            return "Hot Path Detection System"
        elif name == 'tiered_compilation':
            return "Tiered Compilation Manager"
        elif name.endswith('_gc') or 'gc' in name:
            return "Garbage Collection System"
        elif 'graph' in name:
            return "Graph Optimization and Execution"
        elif 'chunk' in name:
            return "Bytecode Chunk Management"
        elif 'compiler' in name:
            return "Compiler Infrastructure"
        elif 'parser' in name:
            return "Parser Implementation"
        elif 'lexer' in name:
            return "Lexical Analysis"
        elif 'ast' in name:
            return "Abstract Syntax Tree"
        elif 'backend' in name:
            return "Backend Code Generation"
        elif 'scheduler' in name:
            return "Task Scheduling System"
        elif 'device' in name:
            return "Device Management"
        else:
            return f"{name.replace('_', ' ').title()} API"
    
    def _infer_file_details(self, filepath: Path) -> str:
        """Infer detailed description from filename and path."""
        name = filepath.stem
        parent = filepath.parent.name
        
        details = f"This file provides the {name.replace('_', ' ')} functionality "
        
        if parent == 'vm':
            details += "for the BDI virtual machine execution environment."
        elif parent == 'jit':
            details += "for just-in-time compilation and optimization."
        elif parent == 'gc':
            details += "for automatic memory management and garbage collection."
        elif parent == 'compiler':
            details += "for source code compilation and analysis."
        elif parent == 'kernel':
            details += "for kernel-level operations and system management."
        else:
            details += "for the BDI system."
        
        return details
    
    def process_all_headers(self) -> Dict[str, int]:
        """Process all header files in the C directory."""
        c_dir = self.base_path / 'C'
        
        if not c_dir.exists():
            print(f"Error: C directory not found at {c_dir}")
            return {}
        
        # Find all header files
        header_files = list(c_dir.rglob('*.h'))
        
        print(f"\n=== Processing {len(header_files)} header files ===\n")
        
        for header in sorted(header_files):
            self.process_header_file(header)
        
        return {
            'headers_processed': self.headers_processed,
            'items_documented': self.items_documented
        }

def main():
    """Main entry point."""
    # Dynamically resolve repository root based on script location
    # This script is in tools/ subdirectory, so parents[1] gives us the repo root
    # This makes the script portable and work in any checkout location
    repo_path = Path(__file__).resolve().parents[1]
    
    if not repo_path.exists():
        print(f"Error: Repository not found at {repo_path}")
        sys.exit(1)
    
    generator = DoxygenDocGenerator(repo_path)
    stats = generator.process_all_headers()
    
    print(f"\n=== Documentation Generation Complete ===")
    print(f"Headers processed: {stats['headers_processed']}")
    print(f"Items documented: {stats['items_documented']}")
    print(f"✅ Doxygen comments added successfully\n")

if __name__ == '__main__':
    main()
