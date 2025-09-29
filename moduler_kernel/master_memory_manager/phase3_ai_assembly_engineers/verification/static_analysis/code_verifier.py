
#!/usr/bin/env python3
"""
Code Verifier - Static analysis and formal verification of generated assembly
Part of Phase 3: AI Assembly Engineers for BDI
"""

import re
import ast
import json
import time
import logging
from typing import Dict, List, Optional, Any, Tuple, Set
from dataclasses import dataclass, asdict
from enum import Enum
import subprocess
import tempfile
import os

class VerificationLevel(Enum):
    BASIC = "basic"
    STANDARD = "standard"
    STRICT = "strict"
    FORMAL = "formal"

class IssueType(Enum):
    SYNTAX_ERROR = "syntax_error"
    SEMANTIC_ERROR = "semantic_error"
    SAFETY_VIOLATION = "safety_violation"
    PERFORMANCE_WARNING = "performance_warning"
    STYLE_ISSUE = "style_issue"
    SECURITY_RISK = "security_risk"

class IssueSeverity(Enum):
    INFO = "info"
    WARNING = "warning"
    ERROR = "error"
    CRITICAL = "critical"

@dataclass
class VerificationIssue:
    issue_type: IssueType
    severity: IssueSeverity
    line_number: int
    column: int
    message: str
    suggestion: str
    rule_id: str
    confidence: float

@dataclass
class VerificationResult:
    passed: bool
    verification_level: VerificationLevel
    total_issues: int
    critical_issues: int
    error_issues: int
    warning_issues: int
    info_issues: int
    issues: List[VerificationIssue]
    metrics: Dict[str, Any]
    verification_time: float
    timestamp: float

@dataclass
class CodeMetrics:
    total_lines: int
    code_lines: int
    comment_lines: int
    blank_lines: int
    instruction_count: int
    complexity_score: float
    maintainability_index: float
    safety_score: float
    performance_score: float

class CodeVerifier:
    """Static analysis and formal verification system for assembly code"""
    
    def __init__(self, config_path: Optional[str] = None):
        self.config = self._load_config(config_path)
        self.verification_rules = self._initialize_verification_rules()
        self.instruction_patterns = self._initialize_instruction_patterns()
        
        # Setup logging
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        self.logger.info("Code Verifier initialized")
    
    def _load_config(self, config_path: Optional[str]) -> Dict[str, Any]:
        """Load verifier configuration"""
        default_config = {
            "enable_syntax_checking": True,
            "enable_semantic_analysis": True,
            "enable_safety_analysis": True,
            "enable_performance_analysis": True,
            "enable_security_analysis": True,
            "enable_formal_verification": False,
            "max_complexity_score": 10.0,
            "min_maintainability_index": 60.0,
            "min_safety_score": 0.8,
            "min_performance_score": 0.7,
            "strict_mode": False,
            "custom_rules_enabled": True
        }
        
        if config_path and os.path.exists(config_path):
            with open(config_path, 'r') as f:
                user_config = json.load(f)
                default_config.update(user_config)
        
        return default_config
    
    def _initialize_verification_rules(self) -> Dict[str, Dict[str, Any]]:
        """Initialize comprehensive verification rules"""
        return {
            # Syntax Rules
            "syntax_001": {
                "name": "Valid Instruction Format",
                "description": "Instructions must follow valid x86 assembly format",
                "pattern": r"^\s*([a-zA-Z][a-zA-Z0-9]*)\s*([^;]*?)(?:;.*)?$",
                "severity": IssueSeverity.ERROR,
                "type": IssueType.SYNTAX_ERROR
            },
            "syntax_002": {
                "name": "Label Format",
                "description": "Labels must end with colon",
                "pattern": r"^\s*([a-zA-Z_][a-zA-Z0-9_]*):.*$",
                "severity": IssueSeverity.ERROR,
                "type": IssueType.SYNTAX_ERROR
            },
            
            # Safety Rules
            "safety_001": {
                "name": "Stack Overflow Protection",
                "description": "Detect potential stack overflow conditions",
                "check_function": "check_stack_overflow",
                "severity": IssueSeverity.CRITICAL,
                "type": IssueType.SAFETY_VIOLATION
            },
            "safety_002": {
                "name": "Buffer Bounds Checking",
                "description": "Memory access must be bounds-checked",
                "check_function": "check_buffer_bounds",
                "severity": IssueSeverity.ERROR,
                "type": IssueType.SAFETY_VIOLATION
            },
            "safety_003": {
                "name": "Register Preservation",
                "description": "Critical registers must be preserved",
                "check_function": "check_register_preservation",
                "severity": IssueSeverity.WARNING,
                "type": IssueType.SAFETY_VIOLATION
            },
            
            # Security Rules
            "security_001": {
                "name": "Privilege Escalation Check",
                "description": "Detect potential privilege escalation attempts",
                "check_function": "check_privilege_escalation",
                "severity": IssueSeverity.CRITICAL,
                "type": IssueType.SECURITY_RISK
            },
            "security_002": {
                "name": "Code Injection Prevention",
                "description": "Prevent code injection vulnerabilities",
                "check_function": "check_code_injection",
                "severity": IssueSeverity.CRITICAL,
                "type": IssueType.SECURITY_RISK
            },
            
            # Performance Rules
            "performance_001": {
                "name": "Inefficient Instruction Usage",
                "description": "Detect inefficient instruction sequences",
                "check_function": "check_instruction_efficiency",
                "severity": IssueSeverity.WARNING,
                "type": IssueType.PERFORMANCE_WARNING
            },
            "performance_002": {
                "name": "Memory Access Optimization",
                "description": "Optimize memory access patterns",
                "check_function": "check_memory_access",
                "severity": IssueSeverity.INFO,
                "type": IssueType.PERFORMANCE_WARNING
            },
            
            # Style Rules
            "style_001": {
                "name": "Code Documentation",
                "description": "Code should be properly documented",
                "check_function": "check_documentation",
                "severity": IssueSeverity.INFO,
                "type": IssueType.STYLE_ISSUE
            },
            "style_002": {
                "name": "Consistent Indentation",
                "description": "Code should use consistent indentation",
                "check_function": "check_indentation",
                "severity": IssueSeverity.INFO,
                "type": IssueType.STYLE_ISSUE
            }
        }
    
    def _initialize_instruction_patterns(self) -> Dict[str, Dict[str, Any]]:
        """Initialize x86 instruction patterns for validation"""
        return {
            # Data Movement Instructions
            "mov": {
                "operands": 2,
                "pattern": r"mov\s+(\w+),\s*(.+)",
                "description": "Move data between operands",
                "safety_level": "safe"
            },
            "push": {
                "operands": 1,
                "pattern": r"push\s+(.+)",
                "description": "Push operand onto stack",
                "safety_level": "safe",
                "stack_effect": -1
            },
            "pop": {
                "operands": 1,
                "pattern": r"pop\s+(.+)",
                "description": "Pop operand from stack",
                "safety_level": "safe",
                "stack_effect": 1
            },
            
            # Arithmetic Instructions
            "add": {
                "operands": 2,
                "pattern": r"add\s+(\w+),\s*(.+)",
                "description": "Add operands",
                "safety_level": "safe"
            },
            "sub": {
                "operands": 2,
                "pattern": r"sub\s+(\w+),\s*(.+)",
                "description": "Subtract operands",
                "safety_level": "safe"
            },
            
            # Control Flow Instructions
            "jmp": {
                "operands": 1,
                "pattern": r"jmp\s+(.+)",
                "description": "Unconditional jump",
                "safety_level": "caution",
                "control_flow": True
            },
            "call": {
                "operands": 1,
                "pattern": r"call\s+(.+)",
                "description": "Call procedure",
                "safety_level": "caution",
                "control_flow": True,
                "stack_effect": -1
            },
            "ret": {
                "operands": 0,
                "pattern": r"ret",
                "description": "Return from procedure",
                "safety_level": "caution",
                "control_flow": True,
                "stack_effect": 1
            },
            
            # System Instructions
            "int": {
                "operands": 1,
                "pattern": r"int\s+(.+)",
                "description": "Software interrupt",
                "safety_level": "dangerous",
                "privileged": True
            },
            "iret": {
                "operands": 0,
                "pattern": r"iret",
                "description": "Return from interrupt",
                "safety_level": "dangerous",
                "privileged": True
            }
        }
    
    def verify_code(self, code: str, verification_level: VerificationLevel = VerificationLevel.STANDARD) -> VerificationResult:
        """Perform comprehensive code verification"""
        start_time = time.time()
        issues = []
        
        # Parse code into lines
        lines = code.split('\n')
        
        # Calculate metrics
        metrics = self._calculate_metrics(code)
        
        # Syntax verification
        if self.config["enable_syntax_checking"]:
            syntax_issues = self._verify_syntax(lines)
            issues.extend(syntax_issues)
        
        # Semantic analysis
        if self.config["enable_semantic_analysis"]:
            semantic_issues = self._verify_semantics(lines)
            issues.extend(semantic_issues)
        
        # Safety analysis
        if self.config["enable_safety_analysis"]:
            safety_issues = self._verify_safety(lines)
            issues.extend(safety_issues)
        
        # Performance analysis
        if self.config["enable_performance_analysis"]:
            performance_issues = self._verify_performance(lines)
            issues.extend(performance_issues)
        
        # Security analysis
        if self.config["enable_security_analysis"]:
            security_issues = self._verify_security(lines)
            issues.extend(security_issues)
        
        # Formal verification (if enabled and requested)
        if (self.config["enable_formal_verification"] and 
            verification_level == VerificationLevel.FORMAL):
            formal_issues = self._perform_formal_verification(code)
            issues.extend(formal_issues)
        
        # Count issues by severity
        critical_count = sum(1 for issue in issues if issue.severity == IssueSeverity.CRITICAL)
        error_count = sum(1 for issue in issues if issue.severity == IssueSeverity.ERROR)
        warning_count = sum(1 for issue in issues if issue.severity == IssueSeverity.WARNING)
        info_count = sum(1 for issue in issues if issue.severity == IssueSeverity.INFO)
        
        # Determine if verification passed
        passed = self._determine_verification_result(issues, verification_level, metrics)
        
        verification_time = time.time() - start_time
        
        return VerificationResult(
            passed=passed,
            verification_level=verification_level,
            total_issues=len(issues),
            critical_issues=critical_count,
            error_issues=error_count,
            warning_issues=warning_count,
            info_issues=info_count,
            issues=issues,
            metrics=asdict(metrics),
            verification_time=verification_time,
            timestamp=time.time()
        )
    
    def _calculate_metrics(self, code: str) -> CodeMetrics:
        """Calculate comprehensive code metrics"""
        lines = code.split('\n')
        
        total_lines = len(lines)
        code_lines = 0
        comment_lines = 0
        blank_lines = 0
        instruction_count = 0
        
        for line in lines:
            stripped = line.strip()
            if not stripped:
                blank_lines += 1
            elif stripped.startswith(';'):
                comment_lines += 1
            else:
                code_lines += 1
                # Count instructions (not labels or directives)
                if not stripped.endswith(':') and not stripped.startswith('.'):
                    instruction_count += 1
        
        # Calculate complexity score (simplified)
        complexity_score = self._calculate_complexity(lines)
        
        # Calculate maintainability index
        maintainability_index = self._calculate_maintainability(
            total_lines, code_lines, comment_lines, complexity_score
        )
        
        # Calculate safety score
        safety_score = self._calculate_safety_score(lines)
        
        # Calculate performance score
        performance_score = self._calculate_performance_score(lines)
        
        return CodeMetrics(
            total_lines=total_lines,
            code_lines=code_lines,
            comment_lines=comment_lines,
            blank_lines=blank_lines,
            instruction_count=instruction_count,
            complexity_score=complexity_score,
            maintainability_index=maintainability_index,
            safety_score=safety_score,
            performance_score=performance_score
        )
    
    def _verify_syntax(self, lines: List[str]) -> List[VerificationIssue]:
        """Verify syntax correctness"""
        issues = []
        
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip()
            if not stripped or stripped.startswith(';'):
                continue
            
            # Check instruction format
            if ':' not in stripped:  # Not a label
                # Check if it's a valid instruction
                parts = stripped.split()
                if parts:
                    instruction = parts[0].lower()
                    
                    # Check if instruction is known
                    if instruction not in self.instruction_patterns:
                        # Check if it's a directive or pseudo-instruction
                        if not instruction.startswith('.') and not instruction.startswith('section'):
                            issues.append(VerificationIssue(
                                issue_type=IssueType.SYNTAX_ERROR,
                                severity=IssueSeverity.ERROR,
                                line_number=line_num,
                                column=0,
                                message=f"Unknown instruction: {instruction}",
                                suggestion=f"Check if '{instruction}' is a valid x86 instruction",
                                rule_id="syntax_001",
                                confidence=0.9
                            ))
                    else:
                        # Validate operand count
                        pattern_info = self.instruction_patterns[instruction]
                        expected_operands = pattern_info["operands"]
                        
                        if len(parts) - 1 != expected_operands and expected_operands > 0:
                            issues.append(VerificationIssue(
                                issue_type=IssueType.SYNTAX_ERROR,
                                severity=IssueSeverity.ERROR,
                                line_number=line_num,
                                column=0,
                                message=f"Instruction '{instruction}' expects {expected_operands} operands, got {len(parts) - 1}",
                                suggestion=f"Provide exactly {expected_operands} operands for '{instruction}'",
                                rule_id="syntax_002",
                                confidence=0.95
                            ))
        
        return issues
    
    def _verify_semantics(self, lines: List[str]) -> List[VerificationIssue]:
        """Verify semantic correctness"""
        issues = []
        
        # Track register usage
        register_usage = {}
        stack_depth = 0
        
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip()
            if not stripped or stripped.startswith(';') or stripped.endswith(':'):
                continue
            
            parts = stripped.split()
            if not parts:
                continue
            
            instruction = parts[0].lower()
            
            # Check stack balance
            if instruction in self.instruction_patterns:
                pattern_info = self.instruction_patterns[instruction]
                if "stack_effect" in pattern_info:
                    stack_depth += pattern_info["stack_effect"]
            
            # Check register usage patterns
            if instruction == "mov" and len(parts) >= 3:
                dest = parts[1].rstrip(',')
                src = parts[2]
                
                # Track register assignments
                if dest in ['eax', 'ebx', 'ecx', 'edx', 'esi', 'edi']:
                    register_usage[dest] = line_num
                
                # Check for potential issues
                if dest == src:
                    issues.append(VerificationIssue(
                        issue_type=IssueType.SEMANTIC_ERROR,
                        severity=IssueSeverity.WARNING,
                        line_number=line_num,
                        column=0,
                        message=f"Redundant move operation: {dest} to itself",
                        suggestion="Remove redundant move operation",
                        rule_id="semantic_001",
                        confidence=0.8
                    ))
        
        # Check final stack balance
        if stack_depth != 0:
            issues.append(VerificationIssue(
                issue_type=IssueType.SEMANTIC_ERROR,
                severity=IssueSeverity.ERROR,
                line_number=len(lines),
                column=0,
                message=f"Stack imbalance detected: {stack_depth} operations",
                suggestion="Ensure equal number of push/pop operations",
                rule_id="semantic_002",
                confidence=0.9
            ))
        
        return issues
    
    def _verify_safety(self, lines: List[str]) -> List[VerificationIssue]:
        """Verify safety requirements"""
        issues = []
        
        # Check for safety violations
        issues.extend(self.check_stack_overflow(lines))
        issues.extend(self.check_buffer_bounds(lines))
        issues.extend(self.check_register_preservation(lines))
        
        return issues
    
    def _verify_performance(self, lines: List[str]) -> List[VerificationIssue]:
        """Verify performance characteristics"""
        issues = []
        
        # Check for performance issues
        issues.extend(self.check_instruction_efficiency(lines))
        issues.extend(self.check_memory_access(lines))
        
        return issues
    
    def _verify_security(self, lines: List[str]) -> List[VerificationIssue]:
        """Verify security requirements"""
        issues = []
        
        # Check for security vulnerabilities
        issues.extend(self.check_privilege_escalation(lines))
        issues.extend(self.check_code_injection(lines))
        
        return issues
    
    def _perform_formal_verification(self, code: str) -> List[VerificationIssue]:
        """Perform formal verification using external tools"""
        issues = []
        
        # This would integrate with formal verification tools like CBMC, SMACK, etc.
        # For now, we'll implement a simplified version
        
        try:
            # Create temporary file
            with tempfile.NamedTemporaryFile(mode='w', suffix='.asm', delete=False) as f:
                f.write(code)
                temp_file = f.name
            
            # Run simplified formal verification
            # In a real implementation, this would use tools like:
            # - CBMC for bounded model checking
            # - SMACK for LLVM-based verification
            # - Dafny for specification-based verification
            
            self.logger.info("Formal verification would be performed here")
            
            # Cleanup
            os.unlink(temp_file)
            
        except Exception as e:
            issues.append(VerificationIssue(
                issue_type=IssueType.SYNTAX_ERROR,
                severity=IssueSeverity.WARNING,
                line_number=0,
                column=0,
                message=f"Formal verification failed: {str(e)}",
                suggestion="Check code for formal verification compatibility",
                rule_id="formal_001",
                confidence=0.5
            ))
        
        return issues
    
    # Safety check implementations
    def check_stack_overflow(self, lines: List[str]) -> List[VerificationIssue]:
        """Check for potential stack overflow conditions"""
        issues = []
        stack_operations = 0
        
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip().lower()
            if 'push' in stripped:
                stack_operations += 1
            elif 'pop' in stripped:
                stack_operations -= 1
            
            # Check for excessive stack usage
            if stack_operations > 100:  # Arbitrary threshold
                issues.append(VerificationIssue(
                    issue_type=IssueType.SAFETY_VIOLATION,
                    severity=IssueSeverity.WARNING,
                    line_number=line_num,
                    column=0,
                    message="Potential stack overflow: excessive stack usage",
                    suggestion="Consider reducing stack usage or implementing stack checks",
                    rule_id="safety_001",
                    confidence=0.7
                ))
        
        return issues
    
    def check_buffer_bounds(self, lines: List[str]) -> List[VerificationIssue]:
        """Check for buffer bounds violations"""
        issues = []
        
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip().lower()
            
            # Look for memory access patterns
            if re.search(r'\[.*\+.*\]', stripped):
                # Found indexed memory access
                issues.append(VerificationIssue(
                    issue_type=IssueType.SAFETY_VIOLATION,
                    severity=IssueSeverity.WARNING,
                    line_number=line_num,
                    column=0,
                    message="Indexed memory access detected - ensure bounds checking",
                    suggestion="Add bounds checking before memory access",
                    rule_id="safety_002",
                    confidence=0.6
                ))
        
        return issues
    
    def check_register_preservation(self, lines: List[str]) -> List[VerificationIssue]:
        """Check for proper register preservation"""
        issues = []
        
        # Track register saves and restores
        saved_registers = set()
        used_registers = set()
        
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip().lower()
            
            if 'pushad' in stripped:
                saved_registers.update(['eax', 'ebx', 'ecx', 'edx', 'esi', 'edi'])
            elif 'popad' in stripped:
                saved_registers.clear()
            elif 'push' in stripped:
                # Extract register name
                match = re.search(r'push\s+(\w+)', stripped)
                if match:
                    saved_registers.add(match.group(1))
            elif 'pop' in stripped:
                # Extract register name
                match = re.search(r'pop\s+(\w+)', stripped)
                if match:
                    saved_registers.discard(match.group(1))
            
            # Track register usage
            for reg in ['eax', 'ebx', 'ecx', 'edx', 'esi', 'edi']:
                if reg in stripped:
                    used_registers.add(reg)
        
        # Check if used registers were properly saved
        unsaved_registers = used_registers - saved_registers
        if unsaved_registers:
            issues.append(VerificationIssue(
                issue_type=IssueType.SAFETY_VIOLATION,
                severity=IssueSeverity.WARNING,
                line_number=1,
                column=0,
                message=f"Registers used without preservation: {', '.join(unsaved_registers)}",
                suggestion="Save and restore registers that are modified",
                rule_id="safety_003",
                confidence=0.8
            ))
        
        return issues
    
    def check_privilege_escalation(self, lines: List[str]) -> List[VerificationIssue]:
        """Check for privilege escalation attempts"""
        issues = []
        
        dangerous_instructions = ['int', 'iret', 'lgdt', 'lidt', 'lldt', 'ltr']
        
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip().lower()
            
            for instr in dangerous_instructions:
                if instr in stripped:
                    issues.append(VerificationIssue(
                        issue_type=IssueType.SECURITY_RISK,
                        severity=IssueSeverity.CRITICAL,
                        line_number=line_num,
                        column=0,
                        message=f"Privileged instruction detected: {instr}",
                        suggestion="Ensure proper privilege level for this instruction",
                        rule_id="security_001",
                        confidence=0.9
                    ))
        
        return issues
    
    def check_code_injection(self, lines: List[str]) -> List[VerificationIssue]:
        """Check for code injection vulnerabilities"""
        issues = []
        
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip().lower()
            
            # Look for dynamic code generation patterns
            if 'jmp' in stripped and ('eax' in stripped or 'ebx' in stripped):
                issues.append(VerificationIssue(
                    issue_type=IssueType.SECURITY_RISK,
                    severity=IssueSeverity.ERROR,
                    line_number=line_num,
                    column=0,
                    message="Potential code injection: indirect jump with register",
                    suggestion="Validate jump targets or use direct jumps",
                    rule_id="security_002",
                    confidence=0.7
                ))
        
        return issues
    
    def check_instruction_efficiency(self, lines: List[str]) -> List[VerificationIssue]:
        """Check for instruction efficiency"""
        issues = []
        
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip().lower()
            
            # Check for inefficient patterns
            if 'mov eax, 0' in stripped:
                issues.append(VerificationIssue(
                    issue_type=IssueType.PERFORMANCE_WARNING,
                    severity=IssueSeverity.INFO,
                    line_number=line_num,
                    column=0,
                    message="Consider using 'xor eax, eax' instead of 'mov eax, 0'",
                    suggestion="Use 'xor eax, eax' for better performance",
                    rule_id="performance_001",
                    confidence=0.8
                ))
        
        return issues
    
    def check_memory_access(self, lines: List[str]) -> List[VerificationIssue]:
        """Check memory access patterns"""
        issues = []
        
        memory_accesses = 0
        for line_num, line in enumerate(lines, 1):
            stripped = line.strip().lower()
            
            if '[' in stripped and ']' in stripped:
                memory_accesses += 1
        
        # Check for excessive memory access
        if memory_accesses > len(lines) * 0.5:  # More than 50% memory access
            issues.append(VerificationIssue(
                issue_type=IssueType.PERFORMANCE_WARNING,
                severity=IssueSeverity.INFO,
                line_number=1,
                column=0,
                message="High memory access ratio detected",
                suggestion="Consider caching values in registers",
                rule_id="performance_002",
                confidence=0.6
            ))
        
        return issues
    
    def check_documentation(self, lines: List[str]) -> List[VerificationIssue]:
        """Check code documentation"""
        issues = []
        
        comment_lines = sum(1 for line in lines if line.strip().startswith(';'))
        code_lines = sum(1 for line in lines if line.strip() and not line.strip().startswith(';'))
        
        if code_lines > 0:
            comment_ratio = comment_lines / code_lines
            if comment_ratio < 0.1:  # Less than 10% comments
                issues.append(VerificationIssue(
                    issue_type=IssueType.STYLE_ISSUE,
                    severity=IssueSeverity.INFO,
                    line_number=1,
                    column=0,
                    message="Low comment ratio - consider adding more documentation",
                    suggestion="Add comments to explain complex operations",
                    rule_id="style_001",
                    confidence=0.7
                ))
        
        return issues
    
    def check_indentation(self, lines: List[str]) -> List[VerificationIssue]:
        """Check indentation consistency"""
        issues = []
        
        indentation_types = set()
        for line in lines:
            if line.startswith(' '):
                indentation_types.add('spaces')
            elif line.startswith('\t'):
                indentation_types.add('tabs')
        
        if len(indentation_types) > 1:
            issues.append(VerificationIssue(
                issue_type=IssueType.STYLE_ISSUE,
                severity=IssueSeverity.INFO,
                line_number=1,
                column=0,
                message="Inconsistent indentation - mixing tabs and spaces",
                suggestion="Use consistent indentation (either tabs or spaces)",
                rule_id="style_002",
                confidence=0.9
            ))
        
        return issues
    
    def _calculate_complexity(self, lines: List[str]) -> float:
        """Calculate cyclomatic complexity"""
        complexity = 1  # Base complexity
        
        for line in lines:
            stripped = line.strip().lower()
            
            # Count decision points
            if any(instr in stripped for instr in ['jmp', 'je', 'jne', 'jg', 'jl', 'call']):
                complexity += 1
            elif 'loop' in stripped:
                complexity += 1
        
        return float(complexity)
    
    def _calculate_maintainability(self, total_lines: int, code_lines: int, 
                                 comment_lines: int, complexity: float) -> float:
        """Calculate maintainability index"""
        if code_lines == 0:
            return 100.0
        
        # Simplified maintainability calculation
        comment_ratio = comment_lines / max(1, total_lines)
        complexity_factor = max(1, complexity)
        
        maintainability = 100 - (complexity_factor * 5) + (comment_ratio * 20)
        return max(0.0, min(100.0, maintainability))
    
    def _calculate_safety_score(self, lines: List[str]) -> float:
        """Calculate safety score"""
        score = 1.0
        total_instructions = 0
        unsafe_instructions = 0
        
        for line in lines:
            stripped = line.strip().lower()
            if not stripped or stripped.startswith(';'):
                continue
            
            parts = stripped.split()
            if parts:
                instruction = parts[0]
                total_instructions += 1
                
                # Check if instruction is potentially unsafe
                if instruction in ['int', 'iret', 'jmp']:
                    unsafe_instructions += 1
        
        if total_instructions > 0:
            unsafe_ratio = unsafe_instructions / total_instructions
            score = max(0.0, 1.0 - unsafe_ratio)
        
        return score
    
    def _calculate_performance_score(self, lines: List[str]) -> float:
        """Calculate performance score"""
        score = 1.0
        total_instructions = 0
        efficient_instructions = 0
        
        efficient_ops = ['mov', 'add', 'sub', 'and', 'or', 'xor', 'shl', 'shr']
        
        for line in lines:
            stripped = line.strip().lower()
            if not stripped or stripped.startswith(';'):
                continue
            
            parts = stripped.split()
            if parts:
                instruction = parts[0]
                total_instructions += 1
                
                if instruction in efficient_ops:
                    efficient_instructions += 1
        
        if total_instructions > 0:
            efficiency_ratio = efficient_instructions / total_instructions
            score = efficiency_ratio
        
        return score
    
    def _determine_verification_result(self, issues: List[VerificationIssue], 
                                     level: VerificationLevel, 
                                     metrics: CodeMetrics) -> bool:
        """Determine overall verification result"""
        
        # Check for critical issues
        critical_issues = [i for i in issues if i.severity == IssueSeverity.CRITICAL]
        if critical_issues:
            return False
        
        # Check for error issues based on verification level
        error_issues = [i for i in issues if i.severity == IssueSeverity.ERROR]
        
        if level == VerificationLevel.STRICT and error_issues:
            return False
        elif level == VerificationLevel.STANDARD and len(error_issues) > 3:
            return False
        elif level == VerificationLevel.BASIC and len(error_issues) > 10:
            return False
        
        # Check metrics thresholds
        if (metrics.safety_score < self.config["min_safety_score"] or
            metrics.performance_score < self.config["min_performance_score"]):
            return False
        
        return True

def main():
    """Demonstrate code verifier functionality"""
    
    # Initialize verifier
    verifier = CodeVerifier()
    
    # Sample assembly code
    sample_code = """
; Sample x86 assembly code
section .text
global _start

_start:
    ; Initialize registers
    mov eax, 1          ; System call number
    mov ebx, 0          ; Exit status
    
    ; Potential issues for demonstration
    mov eax, eax        ; Redundant operation
    push eax            ; Stack operation
    ; Missing pop - stack imbalance
    
    ; System call
    int 0x80            ; Privileged instruction
    
    ; Inefficient operation
    mov ecx, 0          ; Could use xor
    
    ret
    """
    
    # Verify code at different levels
    for level in [VerificationLevel.BASIC, VerificationLevel.STANDARD, VerificationLevel.STRICT]:
        print(f"\n=== Verification Level: {level.value.upper()} ===")
        
        result = verifier.verify_code(sample_code, level)
        
        print(f"Verification Result: {'PASSED' if result.passed else 'FAILED'}")
        print(f"Total Issues: {result.total_issues}")
        print(f"Critical: {result.critical_issues}, Errors: {result.error_issues}, Warnings: {result.warning_issues}")
        print(f"Verification Time: {result.verification_time:.3f}s")
        
        # Show metrics
        print(f"\nCode Metrics:")
        print(f"  Lines of Code: {result.metrics['code_lines']}")
        print(f"  Complexity Score: {result.metrics['complexity_score']:.1f}")
        print(f"  Safety Score: {result.metrics['safety_score']:.2f}")
        print(f"  Performance Score: {result.metrics['performance_score']:.2f}")
        
        # Show first few issues
        if result.issues:
            print(f"\nTop Issues:")
            for issue in result.issues[:3]:
                print(f"  Line {issue.line_number}: {issue.message}")
                print(f"    Suggestion: {issue.suggestion}")

if __name__ == "__main__":
    main()
