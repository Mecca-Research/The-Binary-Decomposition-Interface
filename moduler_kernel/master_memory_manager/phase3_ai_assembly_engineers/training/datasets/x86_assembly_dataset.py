
#!/usr/bin/env python3
"""
X86 Assembly Dataset Generator for AI Training
Generates comprehensive datasets for x86 assembly, memory management, and BDI operations
"""

import os
import json
import random
import numpy as np
from typing import List, Dict, Tuple, Any
from dataclasses import dataclass
from enum import Enum

class InstructionType(Enum):
    DATA_MOVEMENT = "data_movement"
    ARITHMETIC = "arithmetic"
    LOGICAL = "logical"
    CONTROL_FLOW = "control_flow"
    MEMORY_MANAGEMENT = "memory_management"
    SYSTEM_CALLS = "system_calls"
    BDI_OPERATIONS = "bdi_operations"

@dataclass
class AssemblyInstruction:
    mnemonic: str
    operands: List[str]
    instruction_type: InstructionType
    complexity: int  # 1-10 scale
    description: str
    example_usage: str

@dataclass
class TrainingExample:
    input_context: str
    target_assembly: str
    instruction_type: InstructionType
    complexity_level: int
    performance_metrics: Dict[str, float]
    verification_data: Dict[str, Any]

class X86AssemblyDatasetGenerator:
    """Generates comprehensive x86 assembly datasets for AI training"""
    
    def __init__(self):
        self.instructions = self._initialize_instruction_set()
        self.registers_32bit = ['eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'esp', 'ebp']
        self.registers_64bit = ['rax', 'rbx', 'rcx', 'rdx', 'rsi', 'rdi', 'rsp', 'rbp', 'r8', 'r9', 'r10', 'r11', 'r12', 'r13', 'r14', 'r15']
        self.memory_sizes = ['byte', 'word', 'dword', 'qword']
        
    def _initialize_instruction_set(self) -> Dict[InstructionType, List[AssemblyInstruction]]:
        """Initialize comprehensive x86 instruction set"""
        instructions = {
            InstructionType.DATA_MOVEMENT: [
                AssemblyInstruction("mov", ["reg", "reg/mem/imm"], InstructionType.DATA_MOVEMENT, 1, 
                                  "Move data between registers/memory", "mov eax, ebx"),
                AssemblyInstruction("movzx", ["reg32", "reg8/reg16"], InstructionType.DATA_MOVEMENT, 2,
                                  "Move with zero extension", "movzx eax, bl"),
                AssemblyInstruction("movsx", ["reg32", "reg8/reg16"], InstructionType.DATA_MOVEMENT, 2,
                                  "Move with sign extension", "movsx eax, bl"),
                AssemblyInstruction("lea", ["reg", "mem"], InstructionType.DATA_MOVEMENT, 3,
                                  "Load effective address", "lea eax, [ebx + ecx*2 + 8]"),
                AssemblyInstruction("xchg", ["reg/mem", "reg/mem"], InstructionType.DATA_MOVEMENT, 2,
                                  "Exchange operands", "xchg eax, ebx"),
            ],
            InstructionType.ARITHMETIC: [
                AssemblyInstruction("add", ["reg/mem", "reg/mem/imm"], InstructionType.ARITHMETIC, 1,
                                  "Addition", "add eax, ebx"),
                AssemblyInstruction("sub", ["reg/mem", "reg/mem/imm"], InstructionType.ARITHMETIC, 1,
                                  "Subtraction", "sub eax, 10"),
                AssemblyInstruction("mul", ["reg/mem"], InstructionType.ARITHMETIC, 3,
                                  "Unsigned multiplication", "mul ebx"),
                AssemblyInstruction("imul", ["reg", "reg/mem", "imm"], InstructionType.ARITHMETIC, 3,
                                  "Signed multiplication", "imul eax, ebx, 5"),
                AssemblyInstruction("div", ["reg/mem"], InstructionType.ARITHMETIC, 4,
                                  "Unsigned division", "div ebx"),
                AssemblyInstruction("idiv", ["reg/mem"], InstructionType.ARITHMETIC, 4,
                                  "Signed division", "idiv ebx"),
            ],
            InstructionType.LOGICAL: [
                AssemblyInstruction("and", ["reg/mem", "reg/mem/imm"], InstructionType.LOGICAL, 1,
                                  "Bitwise AND", "and eax, 0xFF"),
                AssemblyInstruction("or", ["reg/mem", "reg/mem/imm"], InstructionType.LOGICAL, 1,
                                  "Bitwise OR", "or eax, ebx"),
                AssemblyInstruction("xor", ["reg/mem", "reg/mem/imm"], InstructionType.LOGICAL, 1,
                                  "Bitwise XOR", "xor eax, eax"),
                AssemblyInstruction("not", ["reg/mem"], InstructionType.LOGICAL, 1,
                                  "Bitwise NOT", "not eax"),
                AssemblyInstruction("shl", ["reg/mem", "cl/imm"], InstructionType.LOGICAL, 2,
                                  "Shift left", "shl eax, 2"),
                AssemblyInstruction("shr", ["reg/mem", "cl/imm"], InstructionType.LOGICAL, 2,
                                  "Shift right", "shr eax, cl"),
            ],
            InstructionType.CONTROL_FLOW: [
                AssemblyInstruction("jmp", ["label/reg/mem"], InstructionType.CONTROL_FLOW, 2,
                                  "Unconditional jump", "jmp loop_start"),
                AssemblyInstruction("je", ["label"], InstructionType.CONTROL_FLOW, 2,
                                  "Jump if equal", "je equal_case"),
                AssemblyInstruction("jne", ["label"], InstructionType.CONTROL_FLOW, 2,
                                  "Jump if not equal", "jne not_equal"),
                AssemblyInstruction("call", ["label/reg/mem"], InstructionType.CONTROL_FLOW, 3,
                                  "Call procedure", "call my_function"),
                AssemblyInstruction("ret", [], InstructionType.CONTROL_FLOW, 2,
                                  "Return from procedure", "ret"),
                AssemblyInstruction("loop", ["label"], InstructionType.CONTROL_FLOW, 3,
                                  "Loop with ECX counter", "loop loop_start"),
            ],
            InstructionType.MEMORY_MANAGEMENT: [
                AssemblyInstruction("push", ["reg/mem/imm"], InstructionType.MEMORY_MANAGEMENT, 2,
                                  "Push onto stack", "push eax"),
                AssemblyInstruction("pop", ["reg/mem"], InstructionType.MEMORY_MANAGEMENT, 2,
                                  "Pop from stack", "pop eax"),
                AssemblyInstruction("pushad", [], InstructionType.MEMORY_MANAGEMENT, 3,
                                  "Push all general registers", "pushad"),
                AssemblyInstruction("popad", [], InstructionType.MEMORY_MANAGEMENT, 3,
                                  "Pop all general registers", "popad"),
            ],
            InstructionType.SYSTEM_CALLS: [
                AssemblyInstruction("int", ["imm"], InstructionType.SYSTEM_CALLS, 4,
                                  "Software interrupt", "int 0x80"),
                AssemblyInstruction("syscall", [], InstructionType.SYSTEM_CALLS, 4,
                                  "System call (64-bit)", "syscall"),
                AssemblyInstruction("sysenter", [], InstructionType.SYSTEM_CALLS, 4,
                                  "Fast system call entry", "sysenter"),
            ],
            InstructionType.BDI_OPERATIONS: [
                AssemblyInstruction("bdi_graph_create", ["reg", "size"], InstructionType.BDI_OPERATIONS, 8,
                                  "Create BDI graph structure", "bdi_graph_create eax, 1024"),
                AssemblyInstruction("bdi_node_connect", ["graph", "node1", "node2"], InstructionType.BDI_OPERATIONS, 7,
                                  "Connect BDI graph nodes", "bdi_node_connect eax, ebx, ecx"),
                AssemblyInstruction("bdi_decompose", ["input", "output"], InstructionType.BDI_OPERATIONS, 9,
                                  "Binary decomposition operation", "bdi_decompose eax, ebx"),
                AssemblyInstruction("bdi_compose", ["fragments", "result"], InstructionType.BDI_OPERATIONS, 9,
                                  "Binary composition operation", "bdi_compose eax, ebx"),
            ]
        }
        return instructions
    
    def generate_basic_examples(self, count: int = 1000) -> List[TrainingExample]:
        """Generate basic assembly instruction examples"""
        examples = []
        
        for _ in range(count):
            instruction_type = random.choice(list(InstructionType))
            if instruction_type == InstructionType.BDI_OPERATIONS:
                continue  # Skip BDI operations for basic examples
                
            instruction = random.choice(self.instructions[instruction_type])
            
            # Generate context and target assembly
            context = self._generate_context(instruction, complexity=1)
            target_assembly = self._generate_target_assembly(instruction, context)
            
            example = TrainingExample(
                input_context=context,
                target_assembly=target_assembly,
                instruction_type=instruction_type,
                complexity_level=1,
                performance_metrics=self._calculate_performance_metrics(target_assembly),
                verification_data=self._generate_verification_data(target_assembly)
            )
            examples.append(example)
        
        return examples
    
    def generate_intermediate_examples(self, count: int = 800) -> List[TrainingExample]:
        """Generate intermediate complexity examples with multiple instructions"""
        examples = []
        
        for _ in range(count):
            # Generate sequences of 2-5 instructions
            sequence_length = random.randint(2, 5)
            instruction_sequence = []
            
            for _ in range(sequence_length):
                instruction_type = random.choice([
                    InstructionType.DATA_MOVEMENT,
                    InstructionType.ARITHMETIC,
                    InstructionType.LOGICAL,
                    InstructionType.MEMORY_MANAGEMENT
                ])
                instruction = random.choice(self.instructions[instruction_type])
                instruction_sequence.append(instruction)
            
            context = self._generate_sequence_context(instruction_sequence)
            target_assembly = self._generate_sequence_assembly(instruction_sequence, context)
            
            example = TrainingExample(
                input_context=context,
                target_assembly=target_assembly,
                instruction_type=InstructionType.DATA_MOVEMENT,  # Mixed type
                complexity_level=3,
                performance_metrics=self._calculate_performance_metrics(target_assembly),
                verification_data=self._generate_verification_data(target_assembly)
            )
            examples.append(example)
        
        return examples
    
    def generate_advanced_examples(self, count: int = 500) -> List[TrainingExample]:
        """Generate advanced examples with control flow and procedures"""
        examples = []
        
        for _ in range(count):
            # Generate complex procedures with loops, conditionals, and function calls
            procedure_type = random.choice(['loop', 'conditional', 'function', 'memory_management'])
            
            if procedure_type == 'loop':
                context, target = self._generate_loop_example()
            elif procedure_type == 'conditional':
                context, target = self._generate_conditional_example()
            elif procedure_type == 'function':
                context, target = self._generate_function_example()
            else:
                context, target = self._generate_memory_management_example()
            
            example = TrainingExample(
                input_context=context,
                target_assembly=target,
                instruction_type=InstructionType.CONTROL_FLOW,
                complexity_level=5,
                performance_metrics=self._calculate_performance_metrics(target),
                verification_data=self._generate_verification_data(target)
            )
            examples.append(example)
        
        return examples
    
    def generate_bdi_examples(self, count: int = 300) -> List[TrainingExample]:
        """Generate BDI-specific operation examples"""
        examples = []
        
        for _ in range(count):
            bdi_instruction = random.choice(self.instructions[InstructionType.BDI_OPERATIONS])
            
            context = self._generate_bdi_context(bdi_instruction)
            target_assembly = self._generate_bdi_assembly(bdi_instruction, context)
            
            example = TrainingExample(
                input_context=context,
                target_assembly=target_assembly,
                instruction_type=InstructionType.BDI_OPERATIONS,
                complexity_level=8,
                performance_metrics=self._calculate_performance_metrics(target_assembly),
                verification_data=self._generate_verification_data(target_assembly)
            )
            examples.append(example)
        
        return examples
    
    def _generate_context(self, instruction: AssemblyInstruction, complexity: int) -> str:
        """Generate context description for instruction"""
        contexts = {
            InstructionType.DATA_MOVEMENT: [
                "Move the value from register A to register B",
                "Load the memory address into a register",
                "Copy data between memory locations",
                "Initialize register with immediate value"
            ],
            InstructionType.ARITHMETIC: [
                "Add two numbers and store result",
                "Subtract value from register",
                "Multiply two registers",
                "Divide register by immediate value"
            ],
            InstructionType.LOGICAL: [
                "Perform bitwise AND operation",
                "Clear specific bits in register",
                "Set bits using OR operation",
                "Toggle bits with XOR"
            ],
            InstructionType.CONTROL_FLOW: [
                "Jump to label if condition is met",
                "Call a function with parameters",
                "Return from function call",
                "Loop until counter reaches zero"
            ],
            InstructionType.MEMORY_MANAGEMENT: [
                "Save register value on stack",
                "Restore register from stack",
                "Preserve all registers before function call",
                "Clean up stack after function call"
            ]
        }
        
        return random.choice(contexts.get(instruction.instruction_type, ["Perform operation"]))
    
    def _generate_target_assembly(self, instruction: AssemblyInstruction, context: str) -> str:
        """Generate target assembly code for instruction"""
        if instruction.mnemonic == "mov":
            reg1 = random.choice(self.registers_32bit)
            reg2 = random.choice(self.registers_32bit)
            return f"mov {reg1}, {reg2}"
        elif instruction.mnemonic == "add":
            reg = random.choice(self.registers_32bit)
            value = random.randint(1, 100)
            return f"add {reg}, {value}"
        elif instruction.mnemonic == "push":
            reg = random.choice(self.registers_32bit)
            return f"push {reg}"
        elif instruction.mnemonic == "pop":
            reg = random.choice(self.registers_32bit)
            return f"pop {reg}"
        else:
            # Generate generic instruction
            return f"{instruction.mnemonic} {', '.join(instruction.operands[:2])}"
    
    def _generate_sequence_context(self, instructions: List[AssemblyInstruction]) -> str:
        """Generate context for instruction sequence"""
        return f"Implement a sequence of {len(instructions)} operations for data processing"
    
    def _generate_sequence_assembly(self, instructions: List[AssemblyInstruction], context: str) -> str:
        """Generate assembly sequence"""
        assembly_lines = []
        current_reg = "eax"
        
        for instruction in instructions:
            if instruction.instruction_type == InstructionType.DATA_MOVEMENT:
                next_reg = random.choice(self.registers_32bit)
                assembly_lines.append(f"mov {current_reg}, {next_reg}")
            elif instruction.instruction_type == InstructionType.ARITHMETIC:
                value = random.randint(1, 50)
                assembly_lines.append(f"add {current_reg}, {value}")
            elif instruction.instruction_type == InstructionType.LOGICAL:
                assembly_lines.append(f"and {current_reg}, 0xFF")
            elif instruction.instruction_type == InstructionType.MEMORY_MANAGEMENT:
                assembly_lines.append(f"push {current_reg}")
                assembly_lines.append(f"pop {current_reg}")
        
        return "\n".join(assembly_lines)
    
    def _generate_loop_example(self) -> Tuple[str, str]:
        """Generate loop example"""
        context = "Implement a loop that processes an array of integers"
        assembly = """
mov ecx, 10          ; Loop counter
mov esi, array_base  ; Array pointer
mov eax, 0          ; Accumulator

loop_start:
    add eax, [esi]   ; Add array element
    add esi, 4       ; Move to next element
    loop loop_start  ; Decrement ECX and loop

mov [result], eax    ; Store result
"""
        return context, assembly.strip()
    
    def _generate_conditional_example(self) -> Tuple[str, str]:
        """Generate conditional example"""
        context = "Compare two values and branch based on result"
        assembly = """
mov eax, [value1]
cmp eax, [value2]
je equal_case
jg greater_case

less_case:
    mov ebx, -1
    jmp end_compare

greater_case:
    mov ebx, 1
    jmp end_compare

equal_case:
    mov ebx, 0

end_compare:
    mov [result], ebx
"""
        return context, assembly.strip()
    
    def _generate_function_example(self) -> Tuple[str, str]:
        """Generate function call example"""
        context = "Implement function call with parameter passing and return value"
        assembly = """
; Function prologue
push ebp
mov ebp, esp
sub esp, 16          ; Allocate local variables

; Function body
mov eax, [ebp+8]     ; Get first parameter
add eax, [ebp+12]    ; Add second parameter
mov [ebp-4], eax     ; Store local variable

; Function epilogue
mov eax, [ebp-4]     ; Return value in EAX
mov esp, ebp
pop ebp
ret
"""
        return context, assembly.strip()
    
    def _generate_memory_management_example(self) -> Tuple[str, str]:
        """Generate memory management example"""
        context = "Implement dynamic memory allocation and deallocation"
        assembly = """
; Allocate memory block
push 1024            ; Size in bytes
call malloc
add esp, 4           ; Clean up stack
mov [mem_ptr], eax   ; Store pointer

; Use memory block
mov esi, [mem_ptr]
mov ecx, 256         ; Number of dwords
mov eax, 0           ; Fill value

fill_loop:
    mov [esi], eax
    add esi, 4
    loop fill_loop

; Free memory block
push [mem_ptr]
call free
add esp, 4
"""
        return context, assembly.strip()
    
    def _generate_bdi_context(self, instruction: AssemblyInstruction) -> str:
        """Generate BDI-specific context"""
        contexts = {
            "bdi_graph_create": "Create a new BDI graph structure for binary decomposition",
            "bdi_node_connect": "Connect two nodes in the BDI graph with specified relationship",
            "bdi_decompose": "Perform binary decomposition on input data structure",
            "bdi_compose": "Compose binary fragments back into original structure"
        }
        return contexts.get(instruction.mnemonic, "Perform BDI operation")
    
    def _generate_bdi_assembly(self, instruction: AssemblyInstruction, context: str) -> str:
        """Generate BDI-specific assembly"""
        if instruction.mnemonic == "bdi_graph_create":
            return """
push 1024            ; Graph size
call bdi_graph_create
add esp, 4
mov [graph_handle], eax
"""
        elif instruction.mnemonic == "bdi_node_connect":
            return """
push [node2_id]      ; Second node
push [node1_id]      ; First node  
push [graph_handle]  ; Graph handle
call bdi_node_connect
add esp, 12
"""
        elif instruction.mnemonic == "bdi_decompose":
            return """
push [output_buffer] ; Output buffer
push [input_data]    ; Input data
call bdi_decompose
add esp, 8
mov [decomp_result], eax
"""
        else:
            return f"{instruction.mnemonic} eax, ebx"
    
    def _calculate_performance_metrics(self, assembly: str) -> Dict[str, float]:
        """Calculate performance metrics for assembly code"""
        lines = assembly.strip().split('\n')
        instruction_count = len([line for line in lines if line.strip() and not line.strip().startswith(';')])
        
        # Estimate cycles (simplified)
        estimated_cycles = instruction_count * 1.5  # Average cycles per instruction
        
        # Estimate memory usage
        estimated_memory = instruction_count * 4  # Average bytes per instruction
        
        return {
            "instruction_count": float(instruction_count),
            "estimated_cycles": estimated_cycles,
            "estimated_memory_bytes": estimated_memory,
            "complexity_score": min(instruction_count / 10.0, 10.0)
        }
    
    def _generate_verification_data(self, assembly: str) -> Dict[str, Any]:
        """Generate verification data for assembly code"""
        lines = assembly.strip().split('\n')
        
        # Extract instructions and operands
        instructions = []
        for line in lines:
            line = line.strip()
            if line and not line.startswith(';'):
                parts = line.split()
                if parts:
                    instructions.append({
                        "mnemonic": parts[0],
                        "operands": parts[1:] if len(parts) > 1 else []
                    })
        
        return {
            "instruction_sequence": instructions,
            "register_usage": self._analyze_register_usage(assembly),
            "memory_accesses": self._analyze_memory_accesses(assembly),
            "control_flow": self._analyze_control_flow(assembly)
        }
    
    def _analyze_register_usage(self, assembly: str) -> List[str]:
        """Analyze register usage in assembly code"""
        registers_used = set()
        for reg in self.registers_32bit + self.registers_64bit:
            if reg in assembly:
                registers_used.add(reg)
        return list(registers_used)
    
    def _analyze_memory_accesses(self, assembly: str) -> int:
        """Count memory access operations"""
        memory_ops = assembly.count('[') + assembly.count('push') + assembly.count('pop')
        return memory_ops
    
    def _analyze_control_flow(self, assembly: str) -> Dict[str, int]:
        """Analyze control flow patterns"""
        jumps = assembly.count('jmp') + assembly.count('je') + assembly.count('jne') + assembly.count('jg') + assembly.count('jl')
        calls = assembly.count('call')
        loops = assembly.count('loop')
        
        return {
            "jumps": jumps,
            "calls": calls,
            "loops": loops
        }
    
    def save_dataset(self, examples: List[TrainingExample], filename: str):
        """Save dataset to JSON file"""
        dataset = []
        for example in examples:
            dataset.append({
                "input_context": example.input_context,
                "target_assembly": example.target_assembly,
                "instruction_type": example.instruction_type.value,
                "complexity_level": example.complexity_level,
                "performance_metrics": example.performance_metrics,
                "verification_data": example.verification_data
            })
        
        with open(filename, 'w') as f:
            json.dump(dataset, f, indent=2)
        
        print(f"Dataset saved to {filename} with {len(examples)} examples")

def main():
    """Generate comprehensive x86 assembly datasets"""
    generator = X86AssemblyDatasetGenerator()
    
    # Generate different complexity levels
    print("Generating basic examples...")
    basic_examples = generator.generate_basic_examples(1000)
    
    print("Generating intermediate examples...")
    intermediate_examples = generator.generate_intermediate_examples(800)
    
    print("Generating advanced examples...")
    advanced_examples = generator.generate_advanced_examples(500)
    
    print("Generating BDI examples...")
    bdi_examples = generator.generate_bdi_examples(300)
    
    # Save datasets
    os.makedirs("datasets", exist_ok=True)
    
    generator.save_dataset(basic_examples, "datasets/x86_basic_dataset.json")
    generator.save_dataset(intermediate_examples, "datasets/x86_intermediate_dataset.json")
    generator.save_dataset(advanced_examples, "datasets/x86_advanced_dataset.json")
    generator.save_dataset(bdi_examples, "datasets/bdi_operations_dataset.json")
    
    # Create combined dataset
    all_examples = basic_examples + intermediate_examples + advanced_examples + bdi_examples
    generator.save_dataset(all_examples, "datasets/x86_complete_dataset.json")
    
    print(f"\nDataset generation complete!")
    print(f"Total examples: {len(all_examples)}")
    print(f"Basic: {len(basic_examples)}")
    print(f"Intermediate: {len(intermediate_examples)}")
    print(f"Advanced: {len(advanced_examples)}")
    print(f"BDI Operations: {len(bdi_examples)}")

if __name__ == "__main__":
    main()
