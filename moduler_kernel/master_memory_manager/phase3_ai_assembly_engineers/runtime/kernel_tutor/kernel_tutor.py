
#!/usr/bin/env python3
"""
Kernel Tutor - Interactive teaching system that guides AI through kernel development
Part of Phase 3: AI Assembly Engineers for BDI
"""

import json
import time
import threading
import queue
import logging
from typing import Dict, List, Optional, Any, Tuple
from dataclasses import dataclass, asdict
from enum import Enum
import subprocess
import tempfile
import os
import re

class TutorMode(Enum):
    GUIDED = "guided"
    INTERACTIVE = "interactive"
    AUTONOMOUS = "autonomous"
    ASSESSMENT = "assessment"

class LessonType(Enum):
    KERNEL_BASICS = "kernel_basics"
    MEMORY_MANAGEMENT = "memory_management"
    PROCESS_SCHEDULING = "process_scheduling"
    INTERRUPT_HANDLING = "interrupt_handling"
    SYSTEM_CALLS = "system_calls"
    DEVICE_DRIVERS = "device_drivers"
    BDI_INTEGRATION = "bdi_integration"
    PERFORMANCE_OPTIMIZATION = "performance_optimization"

class FeedbackType(Enum):
    HINT = "hint"
    CORRECTION = "correction"
    ENCOURAGEMENT = "encouragement"
    WARNING = "warning"
    ERROR = "error"
    SUCCESS = "success"

@dataclass
class TutorSession:
    session_id: str
    learner_id: str
    mode: TutorMode
    current_lesson: Optional[LessonType]
    start_timestamp: float
    last_activity: float
    total_interactions: int
    successful_tasks: int
    failed_tasks: int
    current_context: Dict[str, Any]

@dataclass
class LessonStep:
    step_id: str
    title: str
    description: str
    learning_objectives: List[str]
    prerequisites: List[str]
    instructions: str
    expected_code: str
    validation_criteria: Dict[str, Any]
    hints: List[str]
    difficulty_level: int

@dataclass
class TutorFeedback:
    feedback_type: FeedbackType
    message: str
    code_suggestions: Optional[str]
    next_steps: List[str]
    confidence: float
    timestamp: float

@dataclass
class CodeAnalysis:
    syntax_valid: bool
    semantic_valid: bool
    performance_score: float
    safety_score: float
    style_score: float
    issues: List[Dict[str, Any]]
    suggestions: List[str]

class KernelTutor:
    """Interactive teaching system for kernel development"""
    
    def __init__(self, config_path: Optional[str] = None):
        self.config = self._load_config(config_path)
        self.active_sessions = {}
        self.lesson_library = self._initialize_lesson_library()
        self.feedback_engine = FeedbackEngine(self.config)
        self.code_analyzer = CodeAnalyzer(self.config)
        
        # Setup logging
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        # Initialize interaction queue
        self.interaction_queue = queue.Queue()
        self.processing_thread = threading.Thread(target=self._process_interactions, daemon=True)
        self.processing_thread.start()
        
        self.logger.info("Kernel Tutor initialized")
    
    def _load_config(self, config_path: Optional[str]) -> Dict[str, Any]:
        """Load tutor configuration"""
        default_config = {
            "max_concurrent_sessions": 10,
            "session_timeout_minutes": 60,
            "enable_code_analysis": True,
            "enable_real_time_feedback": True,
            "difficulty_adaptation": True,
            "personalization_enabled": True,
            "safety_checks_enabled": True,
            "performance_monitoring": True
        }
        
        if config_path and os.path.exists(config_path):
            with open(config_path, 'r') as f:
                user_config = json.load(f)
                default_config.update(user_config)
        
        return default_config
    
    def _initialize_lesson_library(self) -> Dict[LessonType, List[LessonStep]]:
        """Initialize comprehensive lesson library"""
        lessons = {}
        
        # Kernel Basics Lessons
        lessons[LessonType.KERNEL_BASICS] = [
            LessonStep(
                step_id="kb_001",
                title="Understanding Kernel Space vs User Space",
                description="Learn the fundamental distinction between kernel and user space",
                learning_objectives=[
                    "Understand memory protection mechanisms",
                    "Learn privilege levels and ring architecture",
                    "Master system call interface"
                ],
                prerequisites=[],
                instructions="""
Write assembly code that demonstrates the difference between kernel and user space:
1. Create a simple system call wrapper
2. Show privilege level transitions
3. Demonstrate memory protection boundaries
                """,
                expected_code="""
; System call wrapper example
section .text
global _start

_start:
    ; User space code
    mov eax, 1          ; sys_exit system call
    mov ebx, 0          ; exit status
    int 0x80            ; invoke system call (transition to kernel space)
    
    ; This code runs in user space (ring 3)
    ; The int 0x80 instruction causes a privilege level change to ring 0
                """,
                validation_criteria={
                    "has_system_call": True,
                    "demonstrates_privilege_transition": True,
                    "proper_register_usage": True
                },
                hints=[
                    "Use int 0x80 for system calls on x86",
                    "EAX register contains system call number",
                    "System calls transition from ring 3 to ring 0"
                ],
                difficulty_level=2
            ),
            LessonStep(
                step_id="kb_002",
                title="Kernel Entry Points and Interrupt Handling",
                description="Learn how the kernel handles interrupts and system calls",
                learning_objectives=[
                    "Understand interrupt descriptor table (IDT)",
                    "Learn interrupt service routine structure",
                    "Master context switching basics"
                ],
                prerequisites=["kb_001"],
                instructions="""
Implement a basic interrupt service routine:
1. Set up proper stack frame
2. Save and restore registers
3. Handle the interrupt and return properly
                """,
                expected_code="""
; Basic interrupt service routine
section .text

interrupt_handler:
    ; Save all registers (context save)
    pushad
    push ds
    push es
    push fs
    push gs
    
    ; Set up kernel data segments
    mov ax, 0x10        ; Kernel data segment
    mov ds, ax
    mov es, ax
    
    ; Handle the interrupt (placeholder)
    call handle_interrupt
    
    ; Restore context
    pop gs
    pop fs
    pop es
    pop ds
    popad
    
    ; Return from interrupt
    iret
                """,
                validation_criteria={
                    "saves_context": True,
                    "restores_context": True,
                    "uses_iret": True,
                    "proper_segment_setup": True
                },
                hints=[
                    "Use pushad/popad for register context",
                    "Don't forget segment registers",
                    "iret is used to return from interrupts"
                ],
                difficulty_level=3
            )
        ]
        
        # Memory Management Lessons
        lessons[LessonType.MEMORY_MANAGEMENT] = [
            LessonStep(
                step_id="mm_001",
                title="Page Table Management",
                description="Learn how to set up and manage page tables",
                learning_objectives=[
                    "Understand virtual memory concepts",
                    "Learn page table structure",
                    "Master address translation"
                ],
                prerequisites=["kb_001", "kb_002"],
                instructions="""
Implement basic page table setup:
1. Create page directory entries
2. Set up page table entries
3. Enable paging mechanism
                """,
                expected_code="""
; Page table setup
section .data
align 4096
page_directory:
    times 1024 dd 0     ; 1024 page directory entries

page_table:
    times 1024 dd 0     ; 1024 page table entries

section .text
setup_paging:
    ; Set up first page table entry
    mov eax, page_table
    or eax, 0x003       ; Present + Writable
    mov [page_directory], eax
    
    ; Map first 4MB of memory
    mov ebx, 0x003      ; Present + Writable
    mov ecx, 1024       ; 1024 pages
    mov edi, page_table
    
map_pages:
    mov [edi], ebx
    add ebx, 0x1000     ; Next page (4KB)
    add edi, 4          ; Next entry
    loop map_pages
    
    ; Load page directory
    mov eax, page_directory
    mov cr3, eax
    
    ; Enable paging
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax
    
    ret
                """,
                validation_criteria={
                    "creates_page_directory": True,
                    "sets_up_page_tables": True,
                    "enables_paging": True,
                    "proper_alignment": True
                },
                hints=[
                    "Page tables must be 4KB aligned",
                    "Use CR3 to load page directory",
                    "Set bit 31 of CR0 to enable paging"
                ],
                difficulty_level=4
            )
        ]
        
        # BDI Integration Lessons
        lessons[LessonType.BDI_INTEGRATION] = [
            LessonStep(
                step_id="bdi_001",
                title="BDI Graph Integration with Kernel",
                description="Learn how to integrate BDI graph operations with kernel services",
                learning_objectives=[
                    "Understand BDI graph structure in kernel context",
                    "Learn kernel-level graph operations",
                    "Master BDI system call interface"
                ],
                prerequisites=["kb_001", "kb_002", "mm_001"],
                instructions="""
Implement BDI graph system call:
1. Create BDI graph system call handler
2. Implement graph creation in kernel space
3. Provide user space interface
                """,
                expected_code="""
; BDI graph system call implementation
section .text

; System call handler for BDI operations
sys_bdi_graph_create:
    ; Validate parameters
    cmp ebx, 0          ; Check graph size parameter
    jle .invalid_param
    
    ; Allocate kernel memory for graph
    push ebx            ; Save size
    call kmalloc        ; Allocate kernel memory
    test eax, eax
    jz .alloc_failed
    
    ; Initialize BDI graph structure
    mov edi, eax        ; Graph pointer
    pop ecx             ; Restore size
    mov [edi], ecx      ; Store size
    mov dword [edi+4], 0 ; Node count
    
    ; Return graph handle to user space
    ret

.invalid_param:
    mov eax, -1         ; Error code
    ret

.alloc_failed:
    add esp, 4          ; Clean stack
    mov eax, -2         ; Out of memory
    ret
                """,
                validation_criteria={
                    "validates_parameters": True,
                    "allocates_kernel_memory": True,
                    "initializes_graph_structure": True,
                    "handles_errors": True
                },
                hints=[
                    "Always validate system call parameters",
                    "Use kernel memory allocation functions",
                    "Return appropriate error codes"
                ],
                difficulty_level=6
            )
        ]
        
        return lessons
    
    def create_session(self, learner_id: str, mode: TutorMode = TutorMode.GUIDED) -> str:
        """Create a new tutoring session"""
        session_id = f"session_{learner_id}_{int(time.time())}"
        
        session = TutorSession(
            session_id=session_id,
            learner_id=learner_id,
            mode=mode,
            current_lesson=None,
            start_timestamp=time.time(),
            last_activity=time.time(),
            total_interactions=0,
            successful_tasks=0,
            failed_tasks=0,
            current_context={}
        )
        
        self.active_sessions[session_id] = session
        
        self.logger.info(f"Created tutoring session {session_id} for learner {learner_id}")
        
        return session_id
    
    def start_lesson(self, session_id: str, lesson_type: LessonType) -> Dict[str, Any]:
        """Start a specific lesson in a session"""
        if session_id not in self.active_sessions:
            return {"error": "Session not found"}
        
        session = self.active_sessions[session_id]
        session.current_lesson = lesson_type
        session.last_activity = time.time()
        
        # Get first lesson step
        lesson_steps = self.lesson_library.get(lesson_type, [])
        if not lesson_steps:
            return {"error": "Lesson not available"}
        
        first_step = lesson_steps[0]
        session.current_context = {
            "current_step": 0,
            "total_steps": len(lesson_steps),
            "step_attempts": 0,
            "step_start_time": time.time()
        }
        
        return {
            "session_id": session_id,
            "lesson_type": lesson_type.value,
            "step": asdict(first_step),
            "progress": {
                "current_step": 1,
                "total_steps": len(lesson_steps),
                "completion_percentage": 0
            }
        }
    
    def submit_code(self, session_id: str, code: str) -> Dict[str, Any]:
        """Submit code for evaluation and feedback"""
        if session_id not in self.active_sessions:
            return {"error": "Session not found"}
        
        session = self.active_sessions[session_id]
        session.last_activity = time.time()
        session.total_interactions += 1
        
        if not session.current_lesson:
            return {"error": "No active lesson"}
        
        # Get current lesson step
        lesson_steps = self.lesson_library[session.current_lesson]
        current_step_index = session.current_context.get("current_step", 0)
        
        if current_step_index >= len(lesson_steps):
            return {"error": "Lesson completed"}
        
        current_step = lesson_steps[current_step_index]
        session.current_context["step_attempts"] += 1
        
        # Analyze submitted code
        analysis = self.code_analyzer.analyze_code(code, current_step)
        
        # Generate feedback
        feedback = self.feedback_engine.generate_feedback(
            code, current_step, analysis, session
        )
        
        # Check if step is completed
        step_completed = self._evaluate_step_completion(code, current_step, analysis)
        
        response = {
            "session_id": session_id,
            "analysis": asdict(analysis),
            "feedback": asdict(feedback),
            "step_completed": step_completed,
            "attempts": session.current_context["step_attempts"]
        }
        
        if step_completed:
            session.successful_tasks += 1
            session.current_context["current_step"] += 1
            session.current_context["step_attempts"] = 0
            session.current_context["step_start_time"] = time.time()
            
            # Check if lesson is completed
            if session.current_context["current_step"] >= len(lesson_steps):
                response["lesson_completed"] = True
                response["lesson_summary"] = self._generate_lesson_summary(session)
            else:
                # Provide next step
                next_step = lesson_steps[session.current_context["current_step"]]
                response["next_step"] = asdict(next_step)
                response["progress"] = {
                    "current_step": session.current_context["current_step"] + 1,
                    "total_steps": len(lesson_steps),
                    "completion_percentage": (session.current_context["current_step"] / len(lesson_steps)) * 100
                }
        else:
            session.failed_tasks += 1
        
        return response
    
    def get_hint(self, session_id: str) -> Dict[str, Any]:
        """Get a hint for the current step"""
        if session_id not in self.active_sessions:
            return {"error": "Session not found"}
        
        session = self.active_sessions[session_id]
        
        if not session.current_lesson:
            return {"error": "No active lesson"}
        
        lesson_steps = self.lesson_library[session.current_lesson]
        current_step_index = session.current_context.get("current_step", 0)
        
        if current_step_index >= len(lesson_steps):
            return {"error": "Lesson completed"}
        
        current_step = lesson_steps[current_step_index]
        attempts = session.current_context.get("step_attempts", 0)
        
        # Provide progressive hints based on attempts
        hint_index = min(attempts, len(current_step.hints) - 1)
        hint = current_step.hints[hint_index] if current_step.hints else "No hints available"
        
        return {
            "session_id": session_id,
            "hint": hint,
            "hint_number": hint_index + 1,
            "total_hints": len(current_step.hints),
            "attempts": attempts
        }
    
    def get_session_status(self, session_id: str) -> Dict[str, Any]:
        """Get current session status"""
        if session_id not in self.active_sessions:
            return {"error": "Session not found"}
        
        session = self.active_sessions[session_id]
        
        status = {
            "session_id": session_id,
            "learner_id": session.learner_id,
            "mode": session.mode.value,
            "current_lesson": session.current_lesson.value if session.current_lesson else None,
            "start_timestamp": session.start_timestamp,
            "last_activity": session.last_activity,
            "total_interactions": session.total_interactions,
            "successful_tasks": session.successful_tasks,
            "failed_tasks": session.failed_tasks,
            "success_rate": session.successful_tasks / max(1, session.total_interactions),
            "session_duration": time.time() - session.start_timestamp
        }
        
        if session.current_lesson:
            lesson_steps = self.lesson_library[session.current_lesson]
            current_step = session.current_context.get("current_step", 0)
            status["progress"] = {
                "current_step": current_step + 1,
                "total_steps": len(lesson_steps),
                "completion_percentage": (current_step / len(lesson_steps)) * 100,
                "step_attempts": session.current_context.get("step_attempts", 0)
            }
        
        return status
    
    def end_session(self, session_id: str) -> Dict[str, Any]:
        """End a tutoring session"""
        if session_id not in self.active_sessions:
            return {"error": "Session not found"}
        
        session = self.active_sessions[session_id]
        
        # Generate session summary
        summary = {
            "session_id": session_id,
            "learner_id": session.learner_id,
            "duration": time.time() - session.start_timestamp,
            "total_interactions": session.total_interactions,
            "successful_tasks": session.successful_tasks,
            "failed_tasks": session.failed_tasks,
            "success_rate": session.successful_tasks / max(1, session.total_interactions),
            "lessons_attempted": [session.current_lesson.value] if session.current_lesson else [],
            "end_timestamp": time.time()
        }
        
        # Remove session
        del self.active_sessions[session_id]
        
        self.logger.info(f"Ended tutoring session {session_id}")
        
        return summary
    
    def _evaluate_step_completion(self, code: str, step: LessonStep, analysis: CodeAnalysis) -> bool:
        """Evaluate if a step is completed successfully"""
        if not analysis.syntax_valid:
            return False
        
        # Check validation criteria
        criteria = step.validation_criteria
        
        for criterion, required in criteria.items():
            if criterion == "has_system_call":
                if required and "int 0x80" not in code and "syscall" not in code:
                    return False
            elif criterion == "demonstrates_privilege_transition":
                if required and "int" not in code:
                    return False
            elif criterion == "saves_context":
                if required and ("pushad" not in code and "push" not in code):
                    return False
            elif criterion == "uses_iret":
                if required and "iret" not in code:
                    return False
            elif criterion == "enables_paging":
                if required and "cr0" not in code:
                    return False
        
        # Check minimum quality thresholds
        if analysis.performance_score < 0.6 or analysis.safety_score < 0.7:
            return False
        
        return True
    
    def _generate_lesson_summary(self, session: TutorSession) -> Dict[str, Any]:
        """Generate summary for completed lesson"""
        return {
            "lesson_type": session.current_lesson.value,
            "completion_time": time.time() - session.current_context.get("step_start_time", 0),
            "total_attempts": session.current_context.get("step_attempts", 0),
            "performance_rating": "excellent" if session.successful_tasks > session.failed_tasks else "good"
        }
    
    def _process_interactions(self):
        """Background thread to process interactions"""
        while True:
            try:
                # Process any queued interactions
                interaction = self.interaction_queue.get(timeout=1.0)
                # Process interaction here
                self.interaction_queue.task_done()
            except queue.Empty:
                # Clean up expired sessions
                self._cleanup_expired_sessions()
    
    def _cleanup_expired_sessions(self):
        """Clean up expired sessions"""
        current_time = time.time()
        timeout = self.config["session_timeout_minutes"] * 60
        
        expired_sessions = []
        for session_id, session in self.active_sessions.items():
            if current_time - session.last_activity > timeout:
                expired_sessions.append(session_id)
        
        for session_id in expired_sessions:
            self.logger.info(f"Cleaning up expired session {session_id}")
            del self.active_sessions[session_id]

class FeedbackEngine:
    """Generates intelligent feedback for learners"""
    
    def __init__(self, config: Dict[str, Any]):
        self.config = config
    
    def generate_feedback(self, code: str, step: LessonStep, analysis: CodeAnalysis, 
                         session: TutorSession) -> TutorFeedback:
        """Generate contextual feedback"""
        
        if not analysis.syntax_valid:
            return TutorFeedback(
                feedback_type=FeedbackType.ERROR,
                message="Your code has syntax errors. Please check the assembly syntax.",
                code_suggestions=self._suggest_syntax_fixes(code, analysis),
                next_steps=["Fix syntax errors", "Review assembly language basics"],
                confidence=0.9,
                timestamp=time.time()
            )
        
        if analysis.safety_score < 0.5:
            return TutorFeedback(
                feedback_type=FeedbackType.WARNING,
                message="Your code has potential safety issues. Kernel code must be extremely careful.",
                code_suggestions=self._suggest_safety_improvements(code, analysis),
                next_steps=["Review safety guidelines", "Add bounds checking"],
                confidence=0.8,
                timestamp=time.time()
            )
        
        if analysis.performance_score < 0.6:
            return TutorFeedback(
                feedback_type=FeedbackType.HINT,
                message="Your code works but could be optimized for better performance.",
                code_suggestions=self._suggest_performance_improvements(code, analysis),
                next_steps=["Consider instruction efficiency", "Optimize memory access"],
                confidence=0.7,
                timestamp=time.time()
            )
        
        # Positive feedback
        return TutorFeedback(
            feedback_type=FeedbackType.SUCCESS,
            message="Excellent work! Your code demonstrates good understanding of the concepts.",
            code_suggestions=None,
            next_steps=["Continue to next step", "Try advanced variations"],
            confidence=0.9,
            timestamp=time.time()
        )
    
    def _suggest_syntax_fixes(self, code: str, analysis: CodeAnalysis) -> str:
        """Suggest syntax fixes"""
        suggestions = []
        
        for issue in analysis.issues:
            if issue["type"] == "syntax":
                suggestions.append(f"Line {issue['line']}: {issue['suggestion']}")
        
        return "\n".join(suggestions) if suggestions else "Check assembly syntax reference"
    
    def _suggest_safety_improvements(self, code: str, analysis: CodeAnalysis) -> str:
        """Suggest safety improvements"""
        suggestions = [
            "Add bounds checking for memory access",
            "Validate input parameters",
            "Use proper error handling"
        ]
        return "\n".join(suggestions)
    
    def _suggest_performance_improvements(self, code: str, analysis: CodeAnalysis) -> str:
        """Suggest performance improvements"""
        suggestions = [
            "Consider using more efficient instructions",
            "Optimize register usage",
            "Reduce memory access operations"
        ]
        return "\n".join(suggestions)

class CodeAnalyzer:
    """Analyzes submitted assembly code"""
    
    def __init__(self, config: Dict[str, Any]):
        self.config = config
    
    def analyze_code(self, code: str, step: LessonStep) -> CodeAnalysis:
        """Perform comprehensive code analysis"""
        
        # Basic syntax validation
        syntax_valid = self._validate_syntax(code)
        
        # Semantic analysis
        semantic_valid = self._validate_semantics(code, step)
        
        # Performance analysis
        performance_score = self._analyze_performance(code)
        
        # Safety analysis
        safety_score = self._analyze_safety(code)
        
        # Style analysis
        style_score = self._analyze_style(code)
        
        # Collect issues
        issues = self._collect_issues(code)
        
        # Generate suggestions
        suggestions = self._generate_suggestions(code, step)
        
        return CodeAnalysis(
            syntax_valid=syntax_valid,
            semantic_valid=semantic_valid,
            performance_score=performance_score,
            safety_score=safety_score,
            style_score=style_score,
            issues=issues,
            suggestions=suggestions
        )
    
    def _validate_syntax(self, code: str) -> bool:
        """Validate assembly syntax"""
        # Simplified syntax validation
        lines = code.strip().split('\n')
        
        for line in lines:
            line = line.strip()
            if not line or line.startswith(';'):
                continue
            
            # Check for basic instruction format
            if ':' in line:  # Label
                continue
            
            parts = line.split()
            if not parts:
                continue
            
            instruction = parts[0].lower()
            
            # Check if it's a known instruction (simplified)
            known_instructions = [
                'mov', 'add', 'sub', 'mul', 'div', 'and', 'or', 'xor',
                'push', 'pop', 'call', 'ret', 'jmp', 'je', 'jne', 'jg', 'jl',
                'int', 'iret', 'pushad', 'popad', 'cmp', 'test', 'loop'
            ]
            
            if instruction not in known_instructions and not instruction.startswith('j'):
                return False
        
        return True
    
    def _validate_semantics(self, code: str, step: LessonStep) -> bool:
        """Validate semantic correctness"""
        # Check if code addresses the learning objectives
        objectives_met = 0
        total_objectives = len(step.learning_objectives)
        
        for objective in step.learning_objectives:
            if "system call" in objective.lower() and ("int" in code or "syscall" in code):
                objectives_met += 1
            elif "interrupt" in objective.lower() and ("int" in code or "iret" in code):
                objectives_met += 1
            elif "memory" in objective.lower() and ("mov" in code or "push" in code):
                objectives_met += 1
        
        return objectives_met >= (total_objectives * 0.6)  # 60% of objectives met
    
    def _analyze_performance(self, code: str) -> float:
        """Analyze code performance"""
        lines = [line.strip() for line in code.split('\n') if line.strip() and not line.strip().startswith(';')]
        
        if not lines:
            return 0.0
        
        # Simple performance heuristics
        score = 1.0
        
        # Penalize excessive instructions
        if len(lines) > 50:
            score -= 0.2
        
        # Reward efficient instruction usage
        efficient_instructions = ['mov', 'add', 'sub', 'and', 'or', 'xor']
        efficient_count = sum(1 for line in lines if any(instr in line.lower() for instr in efficient_instructions))
        score += (efficient_count / len(lines)) * 0.3
        
        return max(0.0, min(1.0, score))
    
    def _analyze_safety(self, code: str) -> float:
        """Analyze code safety"""
        score = 1.0
        
        # Check for potentially unsafe operations
        unsafe_patterns = [
            r'mov\s+\w+,\s*0x[0-9a-fA-F]+',  # Direct memory access
            r'jmp\s+\w+',  # Unconditional jumps
        ]
        
        for pattern in unsafe_patterns:
            if re.search(pattern, code, re.IGNORECASE):
                score -= 0.1
        
        # Reward safety practices
        if 'pushad' in code.lower() and 'popad' in code.lower():
            score += 0.2  # Context preservation
        
        return max(0.0, min(1.0, score))
    
    def _analyze_style(self, code: str) -> float:
        """Analyze code style"""
        lines = code.split('\n')
        score = 1.0
        
        # Check for comments
        comment_lines = sum(1 for line in lines if line.strip().startswith(';'))
        if comment_lines > 0:
            score += 0.2
        
        # Check for consistent indentation
        indented_lines = sum(1 for line in lines if line.startswith('    ') or line.startswith('\t'))
        if indented_lines > len(lines) * 0.3:
            score += 0.1
        
        return max(0.0, min(1.0, score))
    
    def _collect_issues(self, code: str) -> List[Dict[str, Any]]:
        """Collect code issues"""
        issues = []
        
        lines = code.split('\n')
        for i, line in enumerate(lines, 1):
            line = line.strip()
            if not line or line.startswith(';'):
                continue
            
            # Check for common issues
            if line.lower().startswith('mov') and ',' not in line:
                issues.append({
                    "type": "syntax",
                    "line": i,
                    "message": "MOV instruction requires two operands",
                    "suggestion": "Use format: mov destination, source"
                })
        
        return issues
    
    def _generate_suggestions(self, code: str, step: LessonStep) -> List[str]:
        """Generate improvement suggestions"""
        suggestions = []
        
        if "pushad" not in code.lower() and "interrupt" in step.title.lower():
            suggestions.append("Consider using pushad/popad to save/restore registers in interrupt handlers")
        
        if len(code.split('\n')) < 5:
            suggestions.append("Try to add more detailed implementation")
        
        if ';' not in code:
            suggestions.append("Add comments to explain your code")
        
        return suggestions

def main():
    """Demonstrate kernel tutor functionality"""
    
    # Initialize tutor
    tutor = KernelTutor()
    
    # Create session
    session_id = tutor.create_session("test_learner", TutorMode.GUIDED)
    print(f"Created session: {session_id}")
    
    # Start kernel basics lesson
    lesson_start = tutor.start_lesson(session_id, LessonType.KERNEL_BASICS)
    print(f"Started lesson: {lesson_start['lesson_type']}")
    print(f"First step: {lesson_start['step']['title']}")
    
    # Submit sample code
    sample_code = """
; System call example
section .text
global _start

_start:
    mov eax, 1          ; sys_exit
    mov ebx, 0          ; exit status
    int 0x80            ; system call
    """
    
    result = tutor.submit_code(session_id, sample_code)
    print(f"Code submission result: {result['feedback']['message']}")
    print(f"Step completed: {result['step_completed']}")
    
    # Get hint if needed
    if not result['step_completed']:
        hint = tutor.get_hint(session_id)
        print(f"Hint: {hint['hint']}")
    
    # Get session status
    status = tutor.get_session_status(session_id)
    print(f"Session status: {status['success_rate']:.2%} success rate")
    
    # End session
    summary = tutor.end_session(session_id)
    print(f"Session ended. Duration: {summary['duration']:.1f} seconds")

if __name__ == "__main__":
    main()
