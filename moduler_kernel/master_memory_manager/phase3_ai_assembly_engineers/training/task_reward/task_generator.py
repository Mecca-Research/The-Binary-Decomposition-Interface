
#!/usr/bin/env python3
"""
Task Generator for AI Assembly Engineers Training
Generates progressive tasks with adaptive difficulty and comprehensive reward systems
"""

import random
import json
import numpy as np
from typing import List, Dict, Tuple, Any, Optional
from dataclasses import dataclass, asdict
from enum import Enum
import hashlib
import time

class TaskType(Enum):
    INSTRUCTION_GENERATION = "instruction_generation"
    CODE_OPTIMIZATION = "code_optimization"
    MEMORY_MANAGEMENT = "memory_management"
    CONTROL_FLOW = "control_flow"
    FUNCTION_IMPLEMENTATION = "function_implementation"
    BDI_OPERATIONS = "bdi_operations"
    SYSTEM_INTEGRATION = "system_integration"
    PERFORMANCE_TUNING = "performance_tuning"

class DifficultyLevel(Enum):
    BEGINNER = 1
    NOVICE = 2
    INTERMEDIATE = 3
    ADVANCED = 4
    EXPERT = 5
    MASTER = 6

@dataclass
class TaskConstraints:
    max_instructions: int
    max_registers: int
    max_memory_accesses: int
    allowed_instruction_types: List[str]
    performance_target: Dict[str, float]
    safety_requirements: List[str]

@dataclass
class Task:
    task_id: str
    task_type: TaskType
    difficulty_level: DifficultyLevel
    description: str
    requirements: List[str]
    constraints: TaskConstraints
    input_data: Dict[str, Any]
    expected_output: Dict[str, Any]
    evaluation_criteria: Dict[str, float]
    hints: List[str]
    time_limit: int  # seconds
    created_timestamp: float

@dataclass
class TaskResult:
    task_id: str
    generated_code: str
    execution_time: float
    performance_metrics: Dict[str, float]
    correctness_score: float
    efficiency_score: float
    safety_score: float
    total_score: float
    feedback: List[str]
    completed_timestamp: float

class TaskGenerator:
    """Generates progressive tasks for AI assembly training"""
    
    def __init__(self):
        self.task_templates = self._initialize_task_templates()
        self.difficulty_progression = self._initialize_difficulty_progression()
        self.performance_baselines = self._initialize_performance_baselines()
        
    def _initialize_task_templates(self) -> Dict[TaskType, List[Dict]]:
        """Initialize task templates for different types"""
        return {
            TaskType.INSTRUCTION_GENERATION: [
                {
                    "description": "Generate x86 assembly to move data between registers",
                    "requirements": ["Use MOV instruction", "Preserve register state"],
                    "input_template": {"source_reg": "eax", "dest_reg": "ebx"},
                    "difficulty_factors": ["register_count", "addressing_modes"]
                },
                {
                    "description": "Generate arithmetic operations with immediate values",
                    "requirements": ["Use ADD/SUB instructions", "Handle overflow"],
                    "input_template": {"operation": "add", "operand1": "eax", "operand2": 42},
                    "difficulty_factors": ["operation_complexity", "operand_types"]
                },
                {
                    "description": "Generate logical operations for bit manipulation",
                    "requirements": ["Use AND/OR/XOR instructions", "Preserve flags"],
                    "input_template": {"operation": "and", "mask": "0xFF"},
                    "difficulty_factors": ["bit_patterns", "flag_handling"]
                }
            ],
            TaskType.CODE_OPTIMIZATION: [
                {
                    "description": "Optimize given assembly code for performance",
                    "requirements": ["Reduce instruction count", "Maintain functionality"],
                    "input_template": {"original_code": "", "optimization_target": "speed"},
                    "difficulty_factors": ["code_complexity", "optimization_constraints"]
                },
                {
                    "description": "Optimize memory access patterns",
                    "requirements": ["Minimize cache misses", "Reduce memory bandwidth"],
                    "input_template": {"memory_pattern": "sequential", "data_size": 1024},
                    "difficulty_factors": ["access_patterns", "cache_hierarchy"]
                }
            ],
            TaskType.MEMORY_MANAGEMENT: [
                {
                    "description": "Implement stack frame management",
                    "requirements": ["Proper prologue/epilogue", "Local variable access"],
                    "input_template": {"local_vars": 3, "parameters": 2},
                    "difficulty_factors": ["frame_complexity", "calling_convention"]
                },
                {
                    "description": "Implement dynamic memory allocation",
                    "requirements": ["Safe allocation/deallocation", "Error handling"],
                    "input_template": {"block_size": 1024, "alignment": 16},
                    "difficulty_factors": ["allocation_strategy", "error_handling"]
                }
            ],
            TaskType.CONTROL_FLOW: [
                {
                    "description": "Implement conditional branching",
                    "requirements": ["Correct condition evaluation", "Proper jump targets"],
                    "input_template": {"condition": "greater_than", "threshold": 100},
                    "difficulty_factors": ["condition_complexity", "branch_prediction"]
                },
                {
                    "description": "Implement loop structures",
                    "requirements": ["Proper loop initialization", "Correct termination"],
                    "input_template": {"loop_type": "for", "iterations": 10},
                    "difficulty_factors": ["loop_complexity", "optimization_level"]
                }
            ],
            TaskType.FUNCTION_IMPLEMENTATION: [
                {
                    "description": "Implement function with parameter passing",
                    "requirements": ["Follow calling convention", "Return value handling"],
                    "input_template": {"params": ["int", "int"], "return_type": "int"},
                    "difficulty_factors": ["parameter_complexity", "calling_convention"]
                }
            ],
            TaskType.BDI_OPERATIONS: [
                {
                    "description": "Implement BDI graph creation and manipulation",
                    "requirements": ["Create graph structure", "Connect nodes efficiently"],
                    "input_template": {"node_count": 10, "connection_density": 0.3},
                    "difficulty_factors": ["graph_complexity", "memory_efficiency"]
                },
                {
                    "description": "Implement binary decomposition algorithm",
                    "requirements": ["Decompose binary data", "Maintain structure integrity"],
                    "input_template": {"data_size": 1024, "decomposition_depth": 3},
                    "difficulty_factors": ["algorithm_complexity", "memory_usage"]
                }
            ]
        }
    
    def _initialize_difficulty_progression(self) -> Dict[DifficultyLevel, Dict]:
        """Initialize difficulty progression parameters"""
        return {
            DifficultyLevel.BEGINNER: {
                "max_instructions": 5,
                "max_registers": 3,
                "max_memory_accesses": 2,
                "complexity_multiplier": 1.0,
                "time_limit": 300  # 5 minutes
            },
            DifficultyLevel.NOVICE: {
                "max_instructions": 10,
                "max_registers": 5,
                "max_memory_accesses": 5,
                "complexity_multiplier": 1.5,
                "time_limit": 600  # 10 minutes
            },
            DifficultyLevel.INTERMEDIATE: {
                "max_instructions": 20,
                "max_registers": 8,
                "max_memory_accesses": 10,
                "complexity_multiplier": 2.0,
                "time_limit": 900  # 15 minutes
            },
            DifficultyLevel.ADVANCED: {
                "max_instructions": 50,
                "max_registers": 12,
                "max_memory_accesses": 20,
                "complexity_multiplier": 3.0,
                "time_limit": 1800  # 30 minutes
            },
            DifficultyLevel.EXPERT: {
                "max_instructions": 100,
                "max_registers": 16,
                "max_memory_accesses": 40,
                "complexity_multiplier": 4.0,
                "time_limit": 3600  # 1 hour
            },
            DifficultyLevel.MASTER: {
                "max_instructions": 200,
                "max_registers": 20,
                "max_memory_accesses": 80,
                "complexity_multiplier": 5.0,
                "time_limit": 7200  # 2 hours
            }
        }
    
    def _initialize_performance_baselines(self) -> Dict[TaskType, Dict]:
        """Initialize performance baselines for different task types"""
        return {
            TaskType.INSTRUCTION_GENERATION: {
                "target_cycles": 1.0,
                "target_memory": 4.0,
                "target_accuracy": 0.95
            },
            TaskType.CODE_OPTIMIZATION: {
                "target_speedup": 1.5,
                "target_size_reduction": 0.8,
                "target_accuracy": 0.98
            },
            TaskType.MEMORY_MANAGEMENT: {
                "target_efficiency": 0.9,
                "target_fragmentation": 0.1,
                "target_accuracy": 0.95
            },
            TaskType.CONTROL_FLOW: {
                "target_branch_prediction": 0.85,
                "target_cycles": 2.0,
                "target_accuracy": 0.95
            },
            TaskType.BDI_OPERATIONS: {
                "target_throughput": 1000.0,
                "target_memory_efficiency": 0.8,
                "target_accuracy": 0.9
            }
        }
    
    def generate_task(self, task_type: TaskType, difficulty: DifficultyLevel, 
                     context: Optional[Dict] = None) -> Task:
        """Generate a specific task with given parameters"""
        
        # Select template
        templates = self.task_templates[task_type]
        template = random.choice(templates)
        
        # Get difficulty parameters
        diff_params = self.difficulty_progression[difficulty]
        
        # Generate unique task ID
        task_id = self._generate_task_id(task_type, difficulty)
        
        # Create constraints
        constraints = TaskConstraints(
            max_instructions=diff_params["max_instructions"],
            max_registers=diff_params["max_registers"],
            max_memory_accesses=diff_params["max_memory_accesses"],
            allowed_instruction_types=self._get_allowed_instructions(task_type, difficulty),
            performance_target=self._get_performance_target(task_type, difficulty),
            safety_requirements=self._get_safety_requirements(task_type, difficulty)
        )
        
        # Generate input data
        input_data = self._generate_input_data(template, difficulty, context)
        
        # Generate expected output
        expected_output = self._generate_expected_output(task_type, input_data, difficulty)
        
        # Create evaluation criteria
        evaluation_criteria = self._create_evaluation_criteria(task_type, difficulty)
        
        # Generate hints
        hints = self._generate_hints(task_type, difficulty, template)
        
        task = Task(
            task_id=task_id,
            task_type=task_type,
            difficulty_level=difficulty,
            description=template["description"],
            requirements=template["requirements"],
            constraints=constraints,
            input_data=input_data,
            expected_output=expected_output,
            evaluation_criteria=evaluation_criteria,
            hints=hints,
            time_limit=diff_params["time_limit"],
            created_timestamp=time.time()
        )
        
        return task
    
    def generate_progressive_curriculum(self, num_tasks_per_level: int = 10) -> List[Task]:
        """Generate a progressive curriculum of tasks"""
        curriculum = []
        
        # Define progression path
        task_progression = [
            (TaskType.INSTRUCTION_GENERATION, DifficultyLevel.BEGINNER),
            (TaskType.INSTRUCTION_GENERATION, DifficultyLevel.NOVICE),
            (TaskType.MEMORY_MANAGEMENT, DifficultyLevel.BEGINNER),
            (TaskType.CONTROL_FLOW, DifficultyLevel.BEGINNER),
            (TaskType.INSTRUCTION_GENERATION, DifficultyLevel.INTERMEDIATE),
            (TaskType.CODE_OPTIMIZATION, DifficultyLevel.BEGINNER),
            (TaskType.MEMORY_MANAGEMENT, DifficultyLevel.NOVICE),
            (TaskType.CONTROL_FLOW, DifficultyLevel.NOVICE),
            (TaskType.FUNCTION_IMPLEMENTATION, DifficultyLevel.BEGINNER),
            (TaskType.CODE_OPTIMIZATION, DifficultyLevel.NOVICE),
            (TaskType.MEMORY_MANAGEMENT, DifficultyLevel.INTERMEDIATE),
            (TaskType.CONTROL_FLOW, DifficultyLevel.INTERMEDIATE),
            (TaskType.FUNCTION_IMPLEMENTATION, DifficultyLevel.NOVICE),
            (TaskType.BDI_OPERATIONS, DifficultyLevel.BEGINNER),
            (TaskType.CODE_OPTIMIZATION, DifficultyLevel.INTERMEDIATE),
            (TaskType.FUNCTION_IMPLEMENTATION, DifficultyLevel.INTERMEDIATE),
            (TaskType.BDI_OPERATIONS, DifficultyLevel.NOVICE),
            (TaskType.SYSTEM_INTEGRATION, DifficultyLevel.BEGINNER),
            (TaskType.BDI_OPERATIONS, DifficultyLevel.INTERMEDIATE),
            (TaskType.PERFORMANCE_TUNING, DifficultyLevel.BEGINNER),
        ]
        
        for task_type, difficulty in task_progression:
            for _ in range(num_tasks_per_level):
                task = self.generate_task(task_type, difficulty)
                curriculum.append(task)
        
        return curriculum
    
    def _generate_task_id(self, task_type: TaskType, difficulty: DifficultyLevel) -> str:
        """Generate unique task ID"""
        timestamp = str(int(time.time() * 1000))
        content = f"{task_type.value}_{difficulty.value}_{timestamp}"
        return hashlib.md5(content.encode()).hexdigest()[:12]
    
    def _get_allowed_instructions(self, task_type: TaskType, difficulty: DifficultyLevel) -> List[str]:
        """Get allowed instruction types for task"""
        base_instructions = ["mov", "add", "sub", "cmp", "jmp", "call", "ret"]
        
        if difficulty.value >= 2:
            base_instructions.extend(["mul", "div", "and", "or", "xor", "shl", "shr"])
        
        if difficulty.value >= 3:
            base_instructions.extend(["push", "pop", "lea", "loop"])
        
        if difficulty.value >= 4:
            base_instructions.extend(["imul", "idiv", "movzx", "movsx"])
        
        if task_type == TaskType.BDI_OPERATIONS:
            base_instructions.extend(["bdi_graph_create", "bdi_node_connect", "bdi_decompose"])
        
        return base_instructions
    
    def _get_performance_target(self, task_type: TaskType, difficulty: DifficultyLevel) -> Dict[str, float]:
        """Get performance targets for task"""
        baseline = self.performance_baselines[task_type]
        difficulty_multiplier = self.difficulty_progression[difficulty]["complexity_multiplier"]
        
        return {
            key: value * difficulty_multiplier for key, value in baseline.items()
        }
    
    def _get_safety_requirements(self, task_type: TaskType, difficulty: DifficultyLevel) -> List[str]:
        """Get safety requirements for task"""
        base_requirements = [
            "No buffer overflows",
            "Proper register preservation",
            "Valid memory access only"
        ]
        
        if difficulty.value >= 3:
            base_requirements.extend([
                "Stack alignment maintenance",
                "Proper error handling"
            ])
        
        if task_type == TaskType.MEMORY_MANAGEMENT:
            base_requirements.extend([
                "No memory leaks",
                "Proper deallocation"
            ])
        
        return base_requirements
    
    def _generate_input_data(self, template: Dict, difficulty: DifficultyLevel, 
                           context: Optional[Dict]) -> Dict[str, Any]:
        """Generate input data for task"""
        input_data = template["input_template"].copy()
        
        # Add difficulty-specific modifications
        if difficulty.value >= 3:
            input_data["optimization_hints"] = ["Use efficient addressing", "Minimize memory access"]
        
        if difficulty.value >= 4:
            input_data["performance_constraints"] = {"max_cycles": 100, "max_memory": 1024}
        
        # Add context-specific data
        if context:
            input_data.update(context)
        
        return input_data
    
    def _generate_expected_output(self, task_type: TaskType, input_data: Dict, 
                                difficulty: DifficultyLevel) -> Dict[str, Any]:
        """Generate expected output for task"""
        expected_output = {
            "assembly_code": "",  # Will be filled by reference implementation
            "performance_metrics": self._get_performance_target(task_type, difficulty),
            "correctness_criteria": ["Functional correctness", "Syntax validity"],
            "optimization_criteria": ["Instruction efficiency", "Register usage"]
        }
        
        return expected_output
    
    def _create_evaluation_criteria(self, task_type: TaskType, difficulty: DifficultyLevel) -> Dict[str, float]:
        """Create evaluation criteria weights"""
        base_criteria = {
            "correctness": 0.4,
            "efficiency": 0.3,
            "safety": 0.2,
            "style": 0.1
        }
        
        # Adjust weights based on task type
        if task_type == TaskType.CODE_OPTIMIZATION:
            base_criteria["efficiency"] = 0.5
            base_criteria["correctness"] = 0.3
        elif task_type == TaskType.MEMORY_MANAGEMENT:
            base_criteria["safety"] = 0.4
            base_criteria["correctness"] = 0.3
        elif task_type == TaskType.BDI_OPERATIONS:
            base_criteria["correctness"] = 0.5
            base_criteria["efficiency"] = 0.3
        
        return base_criteria
    
    def _generate_hints(self, task_type: TaskType, difficulty: DifficultyLevel, 
                       template: Dict) -> List[str]:
        """Generate helpful hints for task"""
        hints = []
        
        if difficulty == DifficultyLevel.BEGINNER:
            hints.extend([
                "Start with basic instruction structure",
                "Check register usage carefully",
                "Verify operand types match instruction requirements"
            ])
        
        if task_type == TaskType.MEMORY_MANAGEMENT:
            hints.extend([
                "Remember to preserve stack alignment",
                "Use proper calling convention",
                "Check for memory access bounds"
            ])
        elif task_type == TaskType.CODE_OPTIMIZATION:
            hints.extend([
                "Look for redundant instructions",
                "Consider register reuse opportunities",
                "Optimize memory access patterns"
            ])
        elif task_type == TaskType.BDI_OPERATIONS:
            hints.extend([
                "Consider graph structure efficiency",
                "Optimize node connection algorithms",
                "Minimize memory fragmentation"
            ])
        
        return hints

class RewardCalculator:
    """Calculates rewards for completed tasks"""
    
    def __init__(self):
        self.base_rewards = {
            DifficultyLevel.BEGINNER: 100,
            DifficultyLevel.NOVICE: 200,
            DifficultyLevel.INTERMEDIATE: 400,
            DifficultyLevel.ADVANCED: 800,
            DifficultyLevel.EXPERT: 1600,
            DifficultyLevel.MASTER: 3200
        }
        
        self.bonus_multipliers = {
            "perfect_score": 2.0,
            "time_bonus": 1.5,
            "efficiency_bonus": 1.3,
            "innovation_bonus": 1.2
        }
    
    def calculate_reward(self, task: Task, result: TaskResult) -> Dict[str, float]:
        """Calculate comprehensive reward for task completion"""
        base_reward = self.base_rewards[task.difficulty_level]
        
        # Performance-based multiplier
        performance_multiplier = (
            result.correctness_score * 0.4 +
            result.efficiency_score * 0.3 +
            result.safety_score * 0.3
        )
        
        # Time bonus
        time_ratio = result.execution_time / task.time_limit
        time_bonus = max(0, (1.0 - time_ratio) * 0.5) if time_ratio < 0.8 else 0
        
        # Efficiency bonus
        efficiency_bonus = max(0, (result.efficiency_score - 0.8) * 0.5) if result.efficiency_score > 0.8 else 0
        
        # Perfect score bonus
        perfect_bonus = 0.5 if result.total_score >= 0.95 else 0
        
        # Calculate final reward
        total_multiplier = performance_multiplier + time_bonus + efficiency_bonus + perfect_bonus
        final_reward = base_reward * max(0.1, total_multiplier)  # Minimum 10% of base reward
        
        return {
            "base_reward": base_reward,
            "performance_multiplier": performance_multiplier,
            "time_bonus": time_bonus,
            "efficiency_bonus": efficiency_bonus,
            "perfect_bonus": perfect_bonus,
            "final_reward": final_reward,
            "total_multiplier": total_multiplier
        }

def main():
    """Generate sample tasks and demonstrate reward calculation"""
    generator = TaskGenerator()
    calculator = RewardCalculator()
    
    # Generate sample curriculum
    print("Generating progressive curriculum...")
    curriculum = generator.generate_progressive_curriculum(5)
    
    # Save curriculum to file
    curriculum_data = []
    for task in curriculum:
        curriculum_data.append(asdict(task))
    
    with open("progressive_curriculum.json", "w") as f:
        json.dump(curriculum_data, f, indent=2, default=str)
    
    print(f"Generated {len(curriculum)} tasks")
    
    # Demonstrate task types and difficulties
    task_counts = {}
    for task in curriculum:
        key = f"{task.task_type.value}_{task.difficulty_level.value}"
        task_counts[key] = task_counts.get(key, 0) + 1
    
    print("\nTask distribution:")
    for key, count in sorted(task_counts.items()):
        print(f"  {key}: {count}")
    
    # Generate sample task result and calculate reward
    sample_task = curriculum[0]
    sample_result = TaskResult(
        task_id=sample_task.task_id,
        generated_code="mov eax, ebx\nadd eax, 42",
        execution_time=120.0,
        performance_metrics={"cycles": 2.0, "memory": 8.0},
        correctness_score=0.95,
        efficiency_score=0.85,
        safety_score=0.90,
        total_score=0.90,
        feedback=["Good register usage", "Consider optimization"],
        completed_timestamp=time.time()
    )
    
    reward_breakdown = calculator.calculate_reward(sample_task, sample_result)
    
    print(f"\nSample reward calculation:")
    for key, value in reward_breakdown.items():
        print(f"  {key}: {value:.2f}")

if __name__ == "__main__":
    main()
