
#!/usr/bin/env python3
"""
Curriculum Manager for AI Assembly Engineers Training
Implements progressive learning system from basic assembly to advanced BDI operations
"""

import json
import numpy as np
from typing import List, Dict, Tuple, Optional, Any
from dataclasses import dataclass, asdict
from enum import Enum
import time
import logging
from pathlib import Path

# Import from task_reward module
import sys
sys.path.append('..')
from task_reward.task_generator import Task, TaskType, DifficultyLevel, TaskGenerator, TaskResult

class LearningPhase(Enum):
    FOUNDATION = "foundation"
    BUILDING = "building"
    INTEGRATION = "integration"
    MASTERY = "mastery"
    SPECIALIZATION = "specialization"

class AdaptationStrategy(Enum):
    CONSERVATIVE = "conservative"
    MODERATE = "moderate"
    AGGRESSIVE = "aggressive"

@dataclass
class LearnerProfile:
    learner_id: str
    current_phase: LearningPhase
    current_difficulty: DifficultyLevel
    strengths: List[TaskType]
    weaknesses: List[TaskType]
    learning_rate: float
    adaptation_strategy: AdaptationStrategy
    total_tasks_completed: int
    success_rate: float
    average_score: float
    time_efficiency: float
    last_updated: float

@dataclass
class CurriculumNode:
    node_id: str
    task_type: TaskType
    difficulty_level: DifficultyLevel
    prerequisites: List[str]
    learning_objectives: List[str]
    estimated_duration: int  # minutes
    success_threshold: float
    retry_limit: int
    adaptive_hints: List[str]

@dataclass
class LearningPath:
    path_id: str
    learner_id: str
    nodes: List[CurriculumNode]
    current_node_index: int
    completion_percentage: float
    estimated_completion_time: int
    adaptive_adjustments: List[Dict[str, Any]]

class CurriculumManager:
    """Manages progressive learning curriculum for AI assembly training"""
    
    def __init__(self, config_path: Optional[str] = None):
        self.task_generator = TaskGenerator()
        self.curriculum_graph = self._build_curriculum_graph()
        self.learner_profiles = {}
        self.learning_paths = {}
        self.adaptation_history = {}
        
        # Load configuration
        self.config = self._load_config(config_path)
        
        # Setup logging
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
    
    def _load_config(self, config_path: Optional[str]) -> Dict[str, Any]:
        """Load curriculum configuration"""
        default_config = {
            "adaptation_sensitivity": 0.7,
            "success_threshold": 0.75,
            "retry_limit": 3,
            "phase_advancement_threshold": 0.8,
            "difficulty_adjustment_rate": 0.1,
            "learning_rate_bounds": [0.1, 2.0],
            "time_efficiency_weight": 0.3,
            "performance_weight": 0.7
        }
        
        if config_path and Path(config_path).exists():
            with open(config_path, 'r') as f:
                user_config = json.load(f)
                default_config.update(user_config)
        
        return default_config
    
    def _build_curriculum_graph(self) -> Dict[str, CurriculumNode]:
        """Build the curriculum dependency graph"""
        nodes = {}
        
        # Foundation Phase - Basic x86 Instructions
        nodes["foundation_mov"] = CurriculumNode(
            node_id="foundation_mov",
            task_type=TaskType.INSTRUCTION_GENERATION,
            difficulty_level=DifficultyLevel.BEGINNER,
            prerequisites=[],
            learning_objectives=[
                "Understand MOV instruction syntax",
                "Master register-to-register transfers",
                "Learn immediate value loading"
            ],
            estimated_duration=30,
            success_threshold=0.8,
            retry_limit=3,
            adaptive_hints=[
                "MOV instruction copies data from source to destination",
                "Syntax: mov destination, source",
                "Common patterns: mov eax, ebx (register to register)"
            ]
        )
        
        nodes["foundation_arithmetic"] = CurriculumNode(
            node_id="foundation_arithmetic",
            task_type=TaskType.INSTRUCTION_GENERATION,
            difficulty_level=DifficultyLevel.BEGINNER,
            prerequisites=["foundation_mov"],
            learning_objectives=[
                "Master ADD and SUB instructions",
                "Understand flag effects",
                "Learn immediate arithmetic"
            ],
            estimated_duration=45,
            success_threshold=0.8,
            retry_limit=3,
            adaptive_hints=[
                "ADD instruction: add destination, source",
                "SUB instruction: sub destination, source",
                "Flags are affected by arithmetic operations"
            ]
        )
        
        nodes["foundation_logical"] = CurriculumNode(
            node_id="foundation_logical",
            task_type=TaskType.INSTRUCTION_GENERATION,
            difficulty_level=DifficultyLevel.BEGINNER,
            prerequisites=["foundation_arithmetic"],
            learning_objectives=[
                "Master AND, OR, XOR instructions",
                "Understand bit manipulation",
                "Learn shift operations"
            ],
            estimated_duration=45,
            success_threshold=0.8,
            retry_limit=3,
            adaptive_hints=[
                "Logical operations work bit by bit",
                "AND is used for masking bits",
                "XOR can be used for toggling bits"
            ]
        )
        
        # Building Phase - Memory and Control Flow
        nodes["building_memory"] = CurriculumNode(
            node_id="building_memory",
            task_type=TaskType.MEMORY_MANAGEMENT,
            difficulty_level=DifficultyLevel.NOVICE,
            prerequisites=["foundation_logical"],
            learning_objectives=[
                "Master stack operations (PUSH/POP)",
                "Understand memory addressing",
                "Learn effective address calculation"
            ],
            estimated_duration=60,
            success_threshold=0.75,
            retry_limit=4,
            adaptive_hints=[
                "Stack grows downward in memory",
                "PUSH decrements ESP, POP increments ESP",
                "LEA calculates addresses without memory access"
            ]
        )
        
        nodes["building_control"] = CurriculumNode(
            node_id="building_control",
            task_type=TaskType.CONTROL_FLOW,
            difficulty_level=DifficultyLevel.NOVICE,
            prerequisites=["building_memory"],
            learning_objectives=[
                "Master conditional jumps",
                "Understand loop structures",
                "Learn function calls"
            ],
            estimated_duration=75,
            success_threshold=0.75,
            retry_limit=4,
            adaptive_hints=[
                "CMP instruction sets flags for conditional jumps",
                "LOOP instruction uses ECX as counter",
                "CALL pushes return address on stack"
            ]
        )
        
        # Integration Phase - Complex Operations
        nodes["integration_functions"] = CurriculumNode(
            node_id="integration_functions",
            task_type=TaskType.FUNCTION_IMPLEMENTATION,
            difficulty_level=DifficultyLevel.INTERMEDIATE,
            prerequisites=["building_control"],
            learning_objectives=[
                "Implement complete functions",
                "Master calling conventions",
                "Handle parameter passing"
            ],
            estimated_duration=90,
            success_threshold=0.7,
            retry_limit=5,
            adaptive_hints=[
                "Function prologue: push ebp; mov ebp, esp",
                "Local variables at negative offsets from EBP",
                "Parameters at positive offsets from EBP"
            ]
        )
        
        nodes["integration_optimization"] = CurriculumNode(
            node_id="integration_optimization",
            task_type=TaskType.CODE_OPTIMIZATION,
            difficulty_level=DifficultyLevel.INTERMEDIATE,
            prerequisites=["integration_functions"],
            learning_objectives=[
                "Optimize instruction sequences",
                "Reduce memory access",
                "Improve register usage"
            ],
            estimated_duration=120,
            success_threshold=0.7,
            retry_limit=5,
            adaptive_hints=[
                "Combine multiple operations when possible",
                "Reuse registers to reduce memory access",
                "Consider instruction pairing and pipeline effects"
            ]
        )
        
        # Mastery Phase - Advanced Concepts
        nodes["mastery_advanced_mem"] = CurriculumNode(
            node_id="mastery_advanced_mem",
            task_type=TaskType.MEMORY_MANAGEMENT,
            difficulty_level=DifficultyLevel.ADVANCED,
            prerequisites=["integration_optimization"],
            learning_objectives=[
                "Implement dynamic memory allocation",
                "Master complex addressing modes",
                "Optimize memory access patterns"
            ],
            estimated_duration=150,
            success_threshold=0.65,
            retry_limit=6,
            adaptive_hints=[
                "Consider cache line alignment",
                "Minimize memory fragmentation",
                "Use appropriate allocation strategies"
            ]
        )
        
        nodes["mastery_performance"] = CurriculumNode(
            node_id="mastery_performance",
            task_type=TaskType.PERFORMANCE_TUNING,
            difficulty_level=DifficultyLevel.ADVANCED,
            prerequisites=["mastery_advanced_mem"],
            learning_objectives=[
                "Profile and optimize code",
                "Understand CPU pipeline effects",
                "Master SIMD instructions"
            ],
            estimated_duration=180,
            success_threshold=0.65,
            retry_limit=6,
            adaptive_hints=[
                "Profile before optimizing",
                "Consider branch prediction effects",
                "Use SIMD for parallel operations"
            ]
        )
        
        # Specialization Phase - BDI Operations
        nodes["specialization_bdi_basic"] = CurriculumNode(
            node_id="specialization_bdi_basic",
            task_type=TaskType.BDI_OPERATIONS,
            difficulty_level=DifficultyLevel.EXPERT,
            prerequisites=["mastery_performance"],
            learning_objectives=[
                "Understand BDI graph structures",
                "Implement basic BDI operations",
                "Master node connection algorithms"
            ],
            estimated_duration=240,
            success_threshold=0.6,
            retry_limit=8,
            adaptive_hints=[
                "BDI graphs represent computational decomposition",
                "Nodes contain binary operations",
                "Connections define data flow"
            ]
        )
        
        nodes["specialization_bdi_advanced"] = CurriculumNode(
            node_id="specialization_bdi_advanced",
            task_type=TaskType.BDI_OPERATIONS,
            difficulty_level=DifficultyLevel.MASTER,
            prerequisites=["specialization_bdi_basic"],
            learning_objectives=[
                "Implement complex BDI algorithms",
                "Optimize BDI graph operations",
                "Master binary decomposition/composition"
            ],
            estimated_duration=300,
            success_threshold=0.6,
            retry_limit=10,
            adaptive_hints=[
                "Optimize graph traversal algorithms",
                "Consider memory locality in graph layout",
                "Implement efficient decomposition strategies"
            ]
        )
        
        nodes["specialization_system_integration"] = CurriculumNode(
            node_id="specialization_system_integration",
            task_type=TaskType.SYSTEM_INTEGRATION,
            difficulty_level=DifficultyLevel.MASTER,
            prerequisites=["specialization_bdi_advanced"],
            learning_objectives=[
                "Integrate with BDI kernel",
                "Implement system-level optimizations",
                "Master real-time constraints"
            ],
            estimated_duration=360,
            success_threshold=0.55,
            retry_limit=12,
            adaptive_hints=[
                "Consider system call overhead",
                "Implement proper error handling",
                "Optimize for real-time performance"
            ]
        )
        
        return nodes
    
    def create_learner_profile(self, learner_id: str, 
                             initial_assessment: Optional[Dict[str, float]] = None) -> LearnerProfile:
        """Create a new learner profile with optional initial assessment"""
        
        # Determine initial phase and difficulty based on assessment
        if initial_assessment:
            avg_score = np.mean(list(initial_assessment.values()))
            if avg_score >= 0.8:
                current_phase = LearningPhase.BUILDING
                current_difficulty = DifficultyLevel.NOVICE
            elif avg_score >= 0.6:
                current_phase = LearningPhase.FOUNDATION
                current_difficulty = DifficultyLevel.BEGINNER
            else:
                current_phase = LearningPhase.FOUNDATION
                current_difficulty = DifficultyLevel.BEGINNER
        else:
            current_phase = LearningPhase.FOUNDATION
            current_difficulty = DifficultyLevel.BEGINNER
        
        profile = LearnerProfile(
            learner_id=learner_id,
            current_phase=current_phase,
            current_difficulty=current_difficulty,
            strengths=[],
            weaknesses=[],
            learning_rate=1.0,
            adaptation_strategy=AdaptationStrategy.MODERATE,
            total_tasks_completed=0,
            success_rate=0.0,
            average_score=0.0,
            time_efficiency=1.0,
            last_updated=time.time()
        )
        
        self.learner_profiles[learner_id] = profile
        return profile
    
    def generate_learning_path(self, learner_id: str) -> LearningPath:
        """Generate personalized learning path for learner"""
        
        if learner_id not in self.learner_profiles:
            raise ValueError(f"Learner profile not found: {learner_id}")
        
        profile = self.learner_profiles[learner_id]
        
        # Determine starting node based on current phase
        phase_start_nodes = {
            LearningPhase.FOUNDATION: "foundation_mov",
            LearningPhase.BUILDING: "building_memory",
            LearningPhase.INTEGRATION: "integration_functions",
            LearningPhase.MASTERY: "mastery_advanced_mem",
            LearningPhase.SPECIALIZATION: "specialization_bdi_basic"
        }
        
        start_node = phase_start_nodes[profile.current_phase]
        
        # Build path using topological sort from start node
        path_nodes = self._build_path_from_node(start_node)
        
        # Adapt path based on learner strengths/weaknesses
        adapted_nodes = self._adapt_path_for_learner(path_nodes, profile)
        
        # Calculate estimated completion time
        total_duration = sum(node.estimated_duration for node in adapted_nodes)
        adjusted_duration = int(total_duration / profile.learning_rate)
        
        path = LearningPath(
            path_id=f"{learner_id}_path_{int(time.time())}",
            learner_id=learner_id,
            nodes=adapted_nodes,
            current_node_index=0,
            completion_percentage=0.0,
            estimated_completion_time=adjusted_duration,
            adaptive_adjustments=[]
        )
        
        self.learning_paths[path.path_id] = path
        return path
    
    def get_next_task(self, learner_id: str) -> Optional[Task]:
        """Get next task for learner based on their learning path"""
        
        # Find active learning path
        active_path = None
        for path in self.learning_paths.values():
            if path.learner_id == learner_id and path.completion_percentage < 1.0:
                active_path = path
                break
        
        if not active_path:
            # Generate new learning path
            active_path = self.generate_learning_path(learner_id)
        
        # Get current node
        if active_path.current_node_index >= len(active_path.nodes):
            return None  # Path completed
        
        current_node = active_path.nodes[active_path.current_node_index]
        
        # Generate task for current node
        task = self.task_generator.generate_task(
            current_node.task_type,
            current_node.difficulty_level,
            context={
                "learning_objectives": current_node.learning_objectives,
                "adaptive_hints": current_node.adaptive_hints,
                "node_id": current_node.node_id
            }
        )
        
        return task
    
    def process_task_result(self, learner_id: str, task: Task, result: TaskResult) -> Dict[str, Any]:
        """Process task result and update learner progress"""
        
        if learner_id not in self.learner_profiles:
            raise ValueError(f"Learner profile not found: {learner_id}")
        
        profile = self.learner_profiles[learner_id]
        
        # Update profile statistics
        profile.total_tasks_completed += 1
        profile.success_rate = (
            (profile.success_rate * (profile.total_tasks_completed - 1) + 
             (1.0 if result.total_score >= 0.7 else 0.0)) / profile.total_tasks_completed
        )
        profile.average_score = (
            (profile.average_score * (profile.total_tasks_completed - 1) + 
             result.total_score) / profile.total_tasks_completed
        )
        
        # Update time efficiency
        expected_time = self._estimate_task_time(task, profile)
        time_ratio = result.execution_time / expected_time if expected_time > 0 else 1.0
        profile.time_efficiency = (profile.time_efficiency * 0.9 + (1.0 / time_ratio) * 0.1)
        
        # Update strengths and weaknesses
        self._update_task_type_performance(profile, task.task_type, result.total_score)
        
        # Determine if advancement is needed
        advancement_decision = self._evaluate_advancement(profile, task, result)
        
        # Update learning path progress
        path_update = self._update_learning_path_progress(learner_id, task, result)
        
        # Adapt curriculum if needed
        adaptation_decision = self._evaluate_curriculum_adaptation(profile, result)
        
        profile.last_updated = time.time()
        
        return {
            "profile_updated": True,
            "advancement_decision": advancement_decision,
            "path_update": path_update,
            "adaptation_decision": adaptation_decision,
            "next_recommendations": self._generate_next_recommendations(profile)
        }
    
    def _build_path_from_node(self, start_node_id: str) -> List[CurriculumNode]:
        """Build learning path from starting node using topological sort"""
        visited = set()
        path = []
        
        def dfs(node_id: str):
            if node_id in visited or node_id not in self.curriculum_graph:
                return
            
            visited.add(node_id)
            node = self.curriculum_graph[node_id]
            
            # Add prerequisites first
            for prereq in node.prerequisites:
                if prereq not in visited:
                    dfs(prereq)
            
            path.append(node)
            
            # Find nodes that depend on this one
            for other_id, other_node in self.curriculum_graph.items():
                if node_id in other_node.prerequisites and other_id not in visited:
                    dfs(other_id)
        
        dfs(start_node_id)
        return path
    
    def _adapt_path_for_learner(self, nodes: List[CurriculumNode], 
                               profile: LearnerProfile) -> List[CurriculumNode]:
        """Adapt learning path based on learner profile"""
        adapted_nodes = []
        
        for node in nodes:
            adapted_node = CurriculumNode(
                node_id=node.node_id,
                task_type=node.task_type,
                difficulty_level=node.difficulty_level,
                prerequisites=node.prerequisites,
                learning_objectives=node.learning_objectives,
                estimated_duration=int(node.estimated_duration / profile.learning_rate),
                success_threshold=node.success_threshold,
                retry_limit=node.retry_limit,
                adaptive_hints=node.adaptive_hints
            )
            
            # Adjust difficulty for weak areas
            if node.task_type in profile.weaknesses:
                if adapted_node.difficulty_level.value > 1:
                    adapted_node.difficulty_level = DifficultyLevel(adapted_node.difficulty_level.value - 1)
                adapted_node.retry_limit += 2
                adapted_node.success_threshold -= 0.05
            
            # Increase challenge for strong areas
            elif node.task_type in profile.strengths:
                if adapted_node.difficulty_level.value < 6:
                    adapted_node.difficulty_level = DifficultyLevel(adapted_node.difficulty_level.value + 1)
                adapted_node.estimated_duration = int(adapted_node.estimated_duration * 0.8)
            
            adapted_nodes.append(adapted_node)
        
        return adapted_nodes
    
    def _estimate_task_time(self, task: Task, profile: LearnerProfile) -> float:
        """Estimate expected task completion time for learner"""
        base_time = task.time_limit * 0.6  # Expect 60% of time limit
        
        # Adjust for learner efficiency
        adjusted_time = base_time / profile.time_efficiency
        
        # Adjust for task type strength/weakness
        if task.task_type in profile.strengths:
            adjusted_time *= 0.8
        elif task.task_type in profile.weaknesses:
            adjusted_time *= 1.3
        
        return adjusted_time
    
    def _update_task_type_performance(self, profile: LearnerProfile, 
                                    task_type: TaskType, score: float):
        """Update learner's performance tracking for task types"""
        
        # Initialize tracking if not exists
        if not hasattr(profile, 'task_type_scores'):
            profile.task_type_scores = {}
        
        if task_type not in profile.task_type_scores:
            profile.task_type_scores[task_type] = []
        
        profile.task_type_scores[task_type].append(score)
        
        # Keep only recent scores (last 10)
        if len(profile.task_type_scores[task_type]) > 10:
            profile.task_type_scores[task_type] = profile.task_type_scores[task_type][-10:]
        
        # Update strengths and weaknesses
        avg_score = np.mean(profile.task_type_scores[task_type])
        
        if avg_score >= 0.8 and task_type not in profile.strengths:
            profile.strengths.append(task_type)
            if task_type in profile.weaknesses:
                profile.weaknesses.remove(task_type)
        elif avg_score <= 0.6 and task_type not in profile.weaknesses:
            profile.weaknesses.append(task_type)
            if task_type in profile.strengths:
                profile.strengths.remove(task_type)
    
    def _evaluate_advancement(self, profile: LearnerProfile, task: Task, 
                            result: TaskResult) -> Dict[str, Any]:
        """Evaluate if learner should advance to next phase/difficulty"""
        
        advancement = {
            "phase_advancement": False,
            "difficulty_advancement": False,
            "recommendations": []
        }
        
        # Check phase advancement
        if (profile.success_rate >= self.config["phase_advancement_threshold"] and
            profile.total_tasks_completed >= 10):
            
            current_phase_value = list(LearningPhase).index(profile.current_phase)
            if current_phase_value < len(LearningPhase) - 1:
                new_phase = list(LearningPhase)[current_phase_value + 1]
                profile.current_phase = new_phase
                advancement["phase_advancement"] = True
                advancement["recommendations"].append(f"Advanced to {new_phase.value} phase")
        
        # Check difficulty advancement
        if (result.total_score >= 0.85 and 
            profile.average_score >= 0.8):
            
            if profile.current_difficulty.value < 6:
                new_difficulty = DifficultyLevel(profile.current_difficulty.value + 1)
                profile.current_difficulty = new_difficulty
                advancement["difficulty_advancement"] = True
                advancement["recommendations"].append(f"Advanced to {new_difficulty.name} difficulty")
        
        return advancement
    
    def _update_learning_path_progress(self, learner_id: str, task: Task, 
                                     result: TaskResult) -> Dict[str, Any]:
        """Update learning path progress"""
        
        # Find active path
        active_path = None
        for path in self.learning_paths.values():
            if path.learner_id == learner_id and path.completion_percentage < 1.0:
                active_path = path
                break
        
        if not active_path:
            return {"error": "No active learning path found"}
        
        # Check if current node is completed
        current_node = active_path.nodes[active_path.current_node_index]
        node_completed = result.total_score >= current_node.success_threshold
        
        update_info = {
            "node_completed": node_completed,
            "current_node": current_node.node_id,
            "score": result.total_score,
            "threshold": current_node.success_threshold
        }
        
        if node_completed:
            # Advance to next node
            active_path.current_node_index += 1
            active_path.completion_percentage = (
                active_path.current_node_index / len(active_path.nodes)
            )
            update_info["advanced_to_next"] = True
            
            if active_path.current_node_index >= len(active_path.nodes):
                update_info["path_completed"] = True
        
        return update_info
    
    def _evaluate_curriculum_adaptation(self, profile: LearnerProfile, 
                                      result: TaskResult) -> Dict[str, Any]:
        """Evaluate if curriculum adaptation is needed"""
        
        adaptation = {
            "adaptation_needed": False,
            "adaptation_type": None,
            "adjustments": []
        }
        
        # Check if learner is struggling
        if (profile.success_rate < 0.5 and profile.total_tasks_completed >= 5):
            adaptation["adaptation_needed"] = True
            adaptation["adaptation_type"] = "difficulty_reduction"
            adaptation["adjustments"].append("Reduce difficulty level")
            adaptation["adjustments"].append("Provide additional hints")
            adaptation["adjustments"].append("Increase retry limit")
        
        # Check if learner is excelling
        elif (profile.success_rate > 0.9 and profile.average_score > 0.85):
            adaptation["adaptation_needed"] = True
            adaptation["adaptation_type"] = "challenge_increase"
            adaptation["adjustments"].append("Increase difficulty level")
            adaptation["adjustments"].append("Add bonus challenges")
            adaptation["adjustments"].append("Reduce time limits")
        
        # Check for specific task type issues
        if hasattr(profile, 'task_type_scores'):
            for task_type, scores in profile.task_type_scores.items():
                if len(scores) >= 3 and np.mean(scores[-3:]) < 0.4:
                    adaptation["adaptation_needed"] = True
                    adaptation["adjustments"].append(f"Focus on {task_type.value} fundamentals")
        
        return adaptation
    
    def _generate_next_recommendations(self, profile: LearnerProfile) -> List[str]:
        """Generate recommendations for next learning steps"""
        recommendations = []
        
        # Phase-specific recommendations
        if profile.current_phase == LearningPhase.FOUNDATION:
            recommendations.append("Focus on mastering basic instruction syntax")
            recommendations.append("Practice register usage patterns")
        elif profile.current_phase == LearningPhase.BUILDING:
            recommendations.append("Work on memory management concepts")
            recommendations.append("Practice control flow structures")
        elif profile.current_phase == LearningPhase.INTEGRATION:
            recommendations.append("Focus on function implementation")
            recommendations.append("Practice code optimization techniques")
        elif profile.current_phase == LearningPhase.MASTERY:
            recommendations.append("Master advanced memory management")
            recommendations.append("Focus on performance optimization")
        else:  # SPECIALIZATION
            recommendations.append("Master BDI-specific operations")
            recommendations.append("Focus on system integration")
        
        # Weakness-specific recommendations
        for weakness in profile.weaknesses:
            recommendations.append(f"Additional practice needed in {weakness.value}")
        
        # Performance-based recommendations
        if profile.time_efficiency < 0.8:
            recommendations.append("Focus on improving coding speed")
        if profile.average_score < 0.7:
            recommendations.append("Review fundamental concepts")
        
        return recommendations
    
    def get_learner_analytics(self, learner_id: str) -> Dict[str, Any]:
        """Get comprehensive analytics for learner"""
        
        if learner_id not in self.learner_profiles:
            raise ValueError(f"Learner profile not found: {learner_id}")
        
        profile = self.learner_profiles[learner_id]
        
        # Find active path
        active_path = None
        for path in self.learning_paths.values():
            if path.learner_id == learner_id:
                active_path = path
                break
        
        analytics = {
            "profile": asdict(profile),
            "learning_path": asdict(active_path) if active_path else None,
            "progress_metrics": {
                "total_tasks": profile.total_tasks_completed,
                "success_rate": profile.success_rate,
                "average_score": profile.average_score,
                "time_efficiency": profile.time_efficiency,
                "current_phase": profile.current_phase.value,
                "current_difficulty": profile.current_difficulty.name
            },
            "strengths": [s.value for s in profile.strengths],
            "weaknesses": [w.value for w in profile.weaknesses],
            "recommendations": self._generate_next_recommendations(profile)
        }
        
        # Add task type performance if available
        if hasattr(profile, 'task_type_scores'):
            task_performance = {}
            for task_type, scores in profile.task_type_scores.items():
                task_performance[task_type.value] = {
                    "average_score": np.mean(scores),
                    "recent_trend": np.mean(scores[-3:]) if len(scores) >= 3 else np.mean(scores),
                    "total_attempts": len(scores)
                }
            analytics["task_type_performance"] = task_performance
        
        return analytics
    
    def save_curriculum_state(self, filepath: str):
        """Save current curriculum state to file"""
        state = {
            "learner_profiles": {
                learner_id: asdict(profile) 
                for learner_id, profile in self.learner_profiles.items()
            },
            "learning_paths": {
                path_id: asdict(path)
                for path_id, path in self.learning_paths.items()
            },
            "adaptation_history": self.adaptation_history,
            "timestamp": time.time()
        }
        
        with open(filepath, 'w') as f:
            json.dump(state, f, indent=2, default=str)
        
        self.logger.info(f"Curriculum state saved to {filepath}")
    
    def load_curriculum_state(self, filepath: str):
        """Load curriculum state from file"""
        with open(filepath, 'r') as f:
            state = json.load(f)
        
        # Reconstruct learner profiles
        for learner_id, profile_data in state["learner_profiles"].items():
            profile = LearnerProfile(**profile_data)
            # Convert enum strings back to enums
            profile.current_phase = LearningPhase(profile_data["current_phase"])
            profile.current_difficulty = DifficultyLevel(profile_data["current_difficulty"])
            profile.adaptation_strategy = AdaptationStrategy(profile_data["adaptation_strategy"])
            profile.strengths = [TaskType(t) for t in profile_data["strengths"]]
            profile.weaknesses = [TaskType(t) for t in profile_data["weaknesses"]]
            
            self.learner_profiles[learner_id] = profile
        
        # Reconstruct learning paths
        for path_id, path_data in state["learning_paths"].items():
            # Reconstruct nodes
            nodes = []
            for node_data in path_data["nodes"]:
                node = CurriculumNode(**node_data)
                node.task_type = TaskType(node_data["task_type"])
                node.difficulty_level = DifficultyLevel(node_data["difficulty_level"])
                nodes.append(node)
            
            path = LearningPath(
                path_id=path_data["path_id"],
                learner_id=path_data["learner_id"],
                nodes=nodes,
                current_node_index=path_data["current_node_index"],
                completion_percentage=path_data["completion_percentage"],
                estimated_completion_time=path_data["estimated_completion_time"],
                adaptive_adjustments=path_data["adaptive_adjustments"]
            )
            
            self.learning_paths[path_id] = path
        
        self.adaptation_history = state.get("adaptation_history", {})
        
        self.logger.info(f"Curriculum state loaded from {filepath}")

def main():
    """Demonstrate curriculum manager functionality"""
    
    # Initialize curriculum manager
    curriculum = CurriculumManager()
    
    # Create sample learner
    learner_id = "test_learner_001"
    profile = curriculum.create_learner_profile(learner_id)
    
    print(f"Created learner profile: {learner_id}")
    print(f"Initial phase: {profile.current_phase.value}")
    print(f"Initial difficulty: {profile.current_difficulty.name}")
    
    # Generate learning path
    path = curriculum.generate_learning_path(learner_id)
    print(f"\nGenerated learning path with {len(path.nodes)} nodes")
    print(f"Estimated completion time: {path.estimated_completion_time} minutes")
    
    # Simulate learning progression
    for i in range(5):
        task = curriculum.get_next_task(learner_id)
        if not task:
            break
        
        print(f"\nTask {i+1}: {task.task_type.value} - {task.difficulty_level.name}")
        print(f"Description: {task.description}")
        
        # Simulate task completion
        simulated_result = TaskResult(
            task_id=task.task_id,
            generated_code="# Simulated assembly code",
            execution_time=random.uniform(60, 300),
            performance_metrics={"cycles": 10.0, "memory": 64.0},
            correctness_score=random.uniform(0.6, 0.95),
            efficiency_score=random.uniform(0.5, 0.9),
            safety_score=random.uniform(0.7, 0.95),
            total_score=random.uniform(0.6, 0.9),
            feedback=["Good work", "Consider optimization"],
            completed_timestamp=time.time()
        )
        
        # Process result
        result_analysis = curriculum.process_task_result(learner_id, task, simulated_result)
        print(f"Task completed with score: {simulated_result.total_score:.2f}")
        
        if result_analysis["advancement_decision"]["phase_advancement"]:
            print("🎉 Advanced to next phase!")
        if result_analysis["advancement_decision"]["difficulty_advancement"]:
            print("📈 Advanced to higher difficulty!")
    
    # Get final analytics
    analytics = curriculum.get_learner_analytics(learner_id)
    print(f"\nFinal Analytics:")
    print(f"Total tasks completed: {analytics['progress_metrics']['total_tasks']}")
    print(f"Success rate: {analytics['progress_metrics']['success_rate']:.2f}")
    print(f"Average score: {analytics['progress_metrics']['average_score']:.2f}")
    print(f"Current phase: {analytics['progress_metrics']['current_phase']}")
    print(f"Strengths: {analytics['strengths']}")
    print(f"Weaknesses: {analytics['weaknesses']}")
    
    # Save state
    curriculum.save_curriculum_state("curriculum_state.json")
    print("\nCurriculum state saved successfully!")

if __name__ == "__main__":
    main()
