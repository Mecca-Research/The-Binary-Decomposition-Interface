
#!/usr/bin/env python3
"""
Learning Analytics for AI Assembly Engineers Training
Comprehensive performance tracking, adaptation algorithms, and progress monitoring
"""

import numpy as np
import pandas as pd
import json
import time
import logging
from typing import List, Dict, Tuple, Optional, Any
from dataclasses import dataclass, asdict
from enum import Enum
from collections import defaultdict, deque
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path

# Import from other modules
import sys
sys.path.append('..')
from task_reward.task_generator import TaskType, DifficultyLevel, Task, TaskResult
from curriculum.curriculum_manager import LearnerProfile, LearningPhase

class MetricType(Enum):
    PERFORMANCE = "performance"
    EFFICIENCY = "efficiency"
    LEARNING_RATE = "learning_rate"
    ENGAGEMENT = "engagement"
    RETENTION = "retention"
    ADAPTATION = "adaptation"

class TrendDirection(Enum):
    IMPROVING = "improving"
    DECLINING = "declining"
    STABLE = "stable"
    VOLATILE = "volatile"

@dataclass
class PerformanceMetric:
    metric_name: str
    metric_type: MetricType
    value: float
    timestamp: float
    context: Dict[str, Any]
    confidence: float

@dataclass
class LearningTrend:
    metric_name: str
    direction: TrendDirection
    slope: float
    confidence: float
    duration_days: float
    significance: float

@dataclass
class AdaptationRecommendation:
    recommendation_id: str
    learner_id: str
    recommendation_type: str
    priority: int  # 1-5, 5 being highest
    description: str
    expected_impact: float
    implementation_effort: int  # 1-5, 5 being highest effort
    evidence: List[str]
    created_timestamp: float

@dataclass
class LearningInsight:
    insight_id: str
    learner_id: str
    insight_type: str
    title: str
    description: str
    supporting_data: Dict[str, Any]
    actionable: bool
    confidence_score: float
    created_timestamp: float

class LearningAnalytics:
    """Comprehensive learning analytics system for AI assembly training"""
    
    def __init__(self, config_path: Optional[str] = None):
        self.config = self._load_config(config_path)
        self.metrics_history = defaultdict(list)
        self.performance_cache = {}
        self.trend_cache = {}
        self.insights_cache = {}
        
        # Setup logging
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        # Initialize analytics components
        self.performance_analyzer = PerformanceAnalyzer(self.config)
        self.trend_analyzer = TrendAnalyzer(self.config)
        self.adaptation_engine = AdaptationEngine(self.config)
        self.insight_generator = InsightGenerator(self.config)
    
    def _load_config(self, config_path: Optional[str]) -> Dict[str, Any]:
        """Load analytics configuration"""
        default_config = {
            "metrics_retention_days": 90,
            "trend_analysis_window": 14,
            "performance_smoothing_factor": 0.3,
            "adaptation_sensitivity": 0.7,
            "insight_confidence_threshold": 0.6,
            "visualization_style": "seaborn",
            "cache_ttl_minutes": 30,
            "batch_processing_size": 100
        }
        
        if config_path and Path(config_path).exists():
            with open(config_path, 'r') as f:
                user_config = json.load(f)
                default_config.update(user_config)
        
        return default_config
    
    def record_task_completion(self, learner_id: str, task: Task, result: TaskResult, 
                             context: Optional[Dict[str, Any]] = None):
        """Record task completion and extract metrics"""
        
        timestamp = time.time()
        context = context or {}
        
        # Extract performance metrics
        metrics = self._extract_performance_metrics(task, result, context, timestamp)
        
        # Store metrics
        for metric in metrics:
            self.metrics_history[learner_id].append(metric)
        
        # Trigger real-time analysis
        self._trigger_real_time_analysis(learner_id, metrics)
        
        # Clean old metrics
        self._cleanup_old_metrics(learner_id)
    
    def _extract_performance_metrics(self, task: Task, result: TaskResult, 
                                   context: Dict[str, Any], timestamp: float) -> List[PerformanceMetric]:
        """Extract comprehensive performance metrics from task result"""
        
        metrics = []
        
        # Core performance metrics
        metrics.append(PerformanceMetric(
            metric_name="correctness_score",
            metric_type=MetricType.PERFORMANCE,
            value=result.correctness_score,
            timestamp=timestamp,
            context={**context, "task_type": task.task_type.value, "difficulty": task.difficulty_level.value},
            confidence=0.95
        ))
        
        metrics.append(PerformanceMetric(
            metric_name="efficiency_score",
            metric_type=MetricType.EFFICIENCY,
            value=result.efficiency_score,
            timestamp=timestamp,
            context={**context, "task_type": task.task_type.value},
            confidence=0.90
        ))
        
        metrics.append(PerformanceMetric(
            metric_name="safety_score",
            metric_type=MetricType.PERFORMANCE,
            value=result.safety_score,
            timestamp=timestamp,
            context={**context, "task_type": task.task_type.value},
            confidence=0.85
        ))
        
        # Time-based efficiency metrics
        time_efficiency = min(1.0, task.time_limit / max(result.execution_time, 1.0))
        metrics.append(PerformanceMetric(
            metric_name="time_efficiency",
            metric_type=MetricType.EFFICIENCY,
            value=time_efficiency,
            timestamp=timestamp,
            context={**context, "time_limit": task.time_limit, "actual_time": result.execution_time},
            confidence=0.95
        ))
        
        # Learning rate estimation
        learning_rate = self._estimate_learning_rate(task, result, context)
        metrics.append(PerformanceMetric(
            metric_name="learning_rate",
            metric_type=MetricType.LEARNING_RATE,
            value=learning_rate,
            timestamp=timestamp,
            context={**context, "estimation_method": "task_difficulty_progression"},
            confidence=0.70
        ))
        
        # Code quality metrics
        if "code_analysis" in result.performance_metrics:
            code_quality = result.performance_metrics["code_analysis"].get("quality_score", 0.5)
            metrics.append(PerformanceMetric(
                metric_name="code_quality",
                metric_type=MetricType.PERFORMANCE,
                value=code_quality,
                timestamp=timestamp,
                context={**context, "analysis_type": "static_analysis"},
                confidence=0.80
            ))
        
        # Engagement metrics
        engagement_score = self._calculate_engagement_score(task, result, context)
        metrics.append(PerformanceMetric(
            metric_name="engagement_score",
            metric_type=MetricType.ENGAGEMENT,
            value=engagement_score,
            timestamp=timestamp,
            context={**context, "calculation_method": "composite_engagement"},
            confidence=0.75
        ))
        
        return metrics
    
    def _estimate_learning_rate(self, task: Task, result: TaskResult, context: Dict[str, Any]) -> float:
        """Estimate learning rate based on task progression"""
        
        # Base learning rate from performance
        base_rate = (result.correctness_score + result.efficiency_score) / 2.0
        
        # Adjust for difficulty progression
        difficulty_factor = task.difficulty_level.value / 6.0  # Normalize to 0-1
        adjusted_rate = base_rate * (1.0 + difficulty_factor * 0.5)
        
        # Adjust for time efficiency
        time_factor = min(1.0, task.time_limit / max(result.execution_time, 1.0))
        final_rate = adjusted_rate * (0.7 + time_factor * 0.3)
        
        return min(1.0, final_rate)
    
    def _calculate_engagement_score(self, task: Task, result: TaskResult, context: Dict[str, Any]) -> float:
        """Calculate engagement score based on various factors"""
        
        # Time spent vs expected time
        expected_time = task.time_limit * 0.6  # Expect 60% of time limit
        time_ratio = result.execution_time / expected_time
        time_engagement = 1.0 - abs(1.0 - time_ratio) * 0.5  # Penalize both too fast and too slow
        
        # Effort indicated by code complexity
        code_lines = len(result.generated_code.split('\n'))
        effort_engagement = min(1.0, code_lines / 20.0)  # Normalize to reasonable code length
        
        # Performance consistency
        performance_variance = abs(result.correctness_score - result.efficiency_score)
        consistency_engagement = 1.0 - performance_variance * 0.5
        
        # Weighted combination
        engagement = (
            time_engagement * 0.4 +
            effort_engagement * 0.3 +
            consistency_engagement * 0.3
        )
        
        return max(0.0, min(1.0, engagement))
    
    def _trigger_real_time_analysis(self, learner_id: str, metrics: List[PerformanceMetric]):
        """Trigger real-time analysis for immediate feedback"""
        
        # Check for immediate concerns
        for metric in metrics:
            if metric.metric_name == "correctness_score" and metric.value < 0.3:
                self.logger.warning(f"Low correctness score for learner {learner_id}: {metric.value}")
            elif metric.metric_name == "safety_score" and metric.value < 0.5:
                self.logger.warning(f"Safety concerns for learner {learner_id}: {metric.value}")
        
        # Update performance cache
        self._update_performance_cache(learner_id, metrics)
    
    def _update_performance_cache(self, learner_id: str, metrics: List[PerformanceMetric]):
        """Update performance cache for quick access"""
        
        if learner_id not in self.performance_cache:
            self.performance_cache[learner_id] = {}
        
        for metric in metrics:
            if metric.metric_name not in self.performance_cache[learner_id]:
                self.performance_cache[learner_id][metric.metric_name] = deque(maxlen=50)
            
            self.performance_cache[learner_id][metric.metric_name].append({
                "value": metric.value,
                "timestamp": metric.timestamp,
                "confidence": metric.confidence
            })
    
    def _cleanup_old_metrics(self, learner_id: str):
        """Clean up old metrics beyond retention period"""
        
        cutoff_time = time.time() - (self.config["metrics_retention_days"] * 24 * 3600)
        
        if learner_id in self.metrics_history:
            self.metrics_history[learner_id] = [
                metric for metric in self.metrics_history[learner_id]
                if metric.timestamp > cutoff_time
            ]
    
    def analyze_learner_performance(self, learner_id: str, 
                                  time_window_days: Optional[int] = None) -> Dict[str, Any]:
        """Comprehensive performance analysis for learner"""
        
        time_window_days = time_window_days or self.config["trend_analysis_window"]
        cutoff_time = time.time() - (time_window_days * 24 * 3600)
        
        # Get recent metrics
        recent_metrics = [
            metric for metric in self.metrics_history.get(learner_id, [])
            if metric.timestamp > cutoff_time
        ]
        
        if not recent_metrics:
            return {"error": "No recent metrics found"}
        
        # Analyze performance by metric type
        performance_analysis = {}
        
        for metric_type in MetricType:
            type_metrics = [m for m in recent_metrics if m.metric_type == metric_type]
            if type_metrics:
                performance_analysis[metric_type.value] = self.performance_analyzer.analyze_metric_group(type_metrics)
        
        # Analyze trends
        trend_analysis = self.trend_analyzer.analyze_trends(learner_id, recent_metrics)
        
        # Generate insights
        insights = self.insight_generator.generate_insights(learner_id, recent_metrics, trend_analysis)
        
        # Generate adaptation recommendations
        recommendations = self.adaptation_engine.generate_recommendations(
            learner_id, performance_analysis, trend_analysis, insights
        )
        
        return {
            "learner_id": learner_id,
            "analysis_period_days": time_window_days,
            "total_metrics": len(recent_metrics),
            "performance_analysis": performance_analysis,
            "trend_analysis": trend_analysis,
            "insights": [asdict(insight) for insight in insights],
            "recommendations": [asdict(rec) for rec in recommendations],
            "generated_timestamp": time.time()
        }
    
    def generate_progress_report(self, learner_id: str) -> Dict[str, Any]:
        """Generate comprehensive progress report"""
        
        all_metrics = self.metrics_history.get(learner_id, [])
        if not all_metrics:
            return {"error": "No metrics found for learner"}
        
        # Calculate overall statistics
        correctness_scores = [m.value for m in all_metrics if m.metric_name == "correctness_score"]
        efficiency_scores = [m.value for m in all_metrics if m.metric_name == "efficiency_score"]
        safety_scores = [m.value for m in all_metrics if m.metric_name == "safety_score"]
        
        # Performance summary
        performance_summary = {
            "total_tasks_analyzed": len(correctness_scores),
            "average_correctness": np.mean(correctness_scores) if correctness_scores else 0,
            "average_efficiency": np.mean(efficiency_scores) if efficiency_scores else 0,
            "average_safety": np.mean(safety_scores) if safety_scores else 0,
            "performance_trend": self._calculate_overall_trend(correctness_scores),
            "consistency_score": 1.0 - np.std(correctness_scores) if len(correctness_scores) > 1 else 1.0
        }
        
        # Task type performance
        task_type_performance = self._analyze_task_type_performance(all_metrics)
        
        # Difficulty progression
        difficulty_progression = self._analyze_difficulty_progression(all_metrics)
        
        # Learning milestones
        milestones = self._identify_learning_milestones(all_metrics)
        
        # Strengths and areas for improvement
        strengths_weaknesses = self._identify_strengths_weaknesses(all_metrics)
        
        return {
            "learner_id": learner_id,
            "report_generated": time.time(),
            "performance_summary": performance_summary,
            "task_type_performance": task_type_performance,
            "difficulty_progression": difficulty_progression,
            "learning_milestones": milestones,
            "strengths_and_weaknesses": strengths_weaknesses,
            "recommendations": self._generate_progress_recommendations(performance_summary, task_type_performance)
        }
    
    def _calculate_overall_trend(self, scores: List[float]) -> str:
        """Calculate overall performance trend"""
        if len(scores) < 3:
            return "insufficient_data"
        
        # Use linear regression to determine trend
        x = np.arange(len(scores))
        slope, _ = np.polyfit(x, scores, 1)
        
        if slope > 0.01:
            return "improving"
        elif slope < -0.01:
            return "declining"
        else:
            return "stable"
    
    def _analyze_task_type_performance(self, metrics: List[PerformanceMetric]) -> Dict[str, Any]:
        """Analyze performance by task type"""
        
        task_type_metrics = defaultdict(list)
        
        for metric in metrics:
            if metric.metric_name == "correctness_score" and "task_type" in metric.context:
                task_type = metric.context["task_type"]
                task_type_metrics[task_type].append(metric.value)
        
        performance_by_type = {}
        for task_type, scores in task_type_metrics.items():
            performance_by_type[task_type] = {
                "average_score": np.mean(scores),
                "score_std": np.std(scores),
                "total_attempts": len(scores),
                "trend": self._calculate_overall_trend(scores)
            }
        
        return performance_by_type
    
    def _analyze_difficulty_progression(self, metrics: List[PerformanceMetric]) -> Dict[str, Any]:
        """Analyze progression through difficulty levels"""
        
        difficulty_metrics = defaultdict(list)
        
        for metric in metrics:
            if metric.metric_name == "correctness_score" and "difficulty" in metric.context:
                difficulty = metric.context["difficulty"]
                difficulty_metrics[difficulty].append({
                    "score": metric.value,
                    "timestamp": metric.timestamp
                })
        
        progression = {}
        for difficulty, data in difficulty_metrics.items():
            scores = [d["score"] for d in data]
            progression[f"difficulty_{difficulty}"] = {
                "average_score": np.mean(scores),
                "attempts": len(scores),
                "first_attempt": min(d["timestamp"] for d in data),
                "last_attempt": max(d["timestamp"] for d in data),
                "mastery_achieved": np.mean(scores[-5:]) > 0.8 if len(scores) >= 5 else False
            }
        
        return progression
    
    def _identify_learning_milestones(self, metrics: List[PerformanceMetric]) -> List[Dict[str, Any]]:
        """Identify significant learning milestones"""
        
        milestones = []
        
        # Find first high performance task
        correctness_metrics = [m for m in metrics if m.metric_name == "correctness_score"]
        for i, metric in enumerate(correctness_metrics):
            if metric.value >= 0.9:
                milestones.append({
                    "milestone": "first_excellent_performance",
                    "description": "First task with >90% correctness",
                    "timestamp": metric.timestamp,
                    "task_number": i + 1,
                    "score": metric.value
                })
                break
        
        # Find consistency milestone
        if len(correctness_metrics) >= 10:
            recent_scores = [m.value for m in correctness_metrics[-10:]]
            if all(score >= 0.75 for score in recent_scores):
                milestones.append({
                    "milestone": "consistent_performance",
                    "description": "10 consecutive tasks with >75% correctness",
                    "timestamp": correctness_metrics[-1].timestamp,
                    "average_score": np.mean(recent_scores)
                })
        
        # Find difficulty advancement milestones
        difficulty_changes = []
        current_difficulty = None
        for metric in metrics:
            if "difficulty" in metric.context:
                difficulty = metric.context["difficulty"]
                if current_difficulty is not None and difficulty > current_difficulty:
                    difficulty_changes.append({
                        "milestone": f"difficulty_advancement_to_{difficulty}",
                        "description": f"Advanced to difficulty level {difficulty}",
                        "timestamp": metric.timestamp,
                        "from_difficulty": current_difficulty,
                        "to_difficulty": difficulty
                    })
                current_difficulty = difficulty
        
        milestones.extend(difficulty_changes)
        
        return sorted(milestones, key=lambda x: x["timestamp"])
    
    def _identify_strengths_weaknesses(self, metrics: List[PerformanceMetric]) -> Dict[str, Any]:
        """Identify learner strengths and weaknesses"""
        
        # Analyze by task type
        task_type_performance = self._analyze_task_type_performance(metrics)
        
        strengths = []
        weaknesses = []
        
        for task_type, performance in task_type_performance.items():
            if performance["average_score"] >= 0.8:
                strengths.append({
                    "area": task_type,
                    "average_score": performance["average_score"],
                    "consistency": 1.0 - performance["score_std"]
                })
            elif performance["average_score"] <= 0.6:
                weaknesses.append({
                    "area": task_type,
                    "average_score": performance["average_score"],
                    "improvement_needed": 0.8 - performance["average_score"]
                })
        
        # Analyze by metric type
        metric_performance = defaultdict(list)
        for metric in metrics:
            metric_performance[metric.metric_name].append(metric.value)
        
        for metric_name, values in metric_performance.items():
            avg_value = np.mean(values)
            if avg_value >= 0.85:
                strengths.append({
                    "area": metric_name,
                    "average_score": avg_value,
                    "type": "metric_strength"
                })
            elif avg_value <= 0.6:
                weaknesses.append({
                    "area": metric_name,
                    "average_score": avg_value,
                    "type": "metric_weakness"
                })
        
        return {
            "strengths": sorted(strengths, key=lambda x: x["average_score"], reverse=True),
            "weaknesses": sorted(weaknesses, key=lambda x: x["average_score"])
        }
    
    def _generate_progress_recommendations(self, performance_summary: Dict[str, Any], 
                                         task_type_performance: Dict[str, Any]) -> List[str]:
        """Generate recommendations based on progress analysis"""
        
        recommendations = []
        
        # Overall performance recommendations
        if performance_summary["average_correctness"] < 0.7:
            recommendations.append("Focus on fundamental concepts to improve correctness")
        
        if performance_summary["average_efficiency"] < 0.6:
            recommendations.append("Practice code optimization techniques")
        
        if performance_summary["consistency_score"] < 0.7:
            recommendations.append("Work on maintaining consistent performance across tasks")
        
        # Task-specific recommendations
        for task_type, performance in task_type_performance.items():
            if performance["average_score"] < 0.6:
                recommendations.append(f"Additional practice needed in {task_type}")
            elif performance["score_std"] > 0.3:
                recommendations.append(f"Focus on consistency in {task_type} tasks")
        
        # Trend-based recommendations
        if performance_summary["performance_trend"] == "declining":
            recommendations.append("Review recent mistakes and focus on areas of difficulty")
        elif performance_summary["performance_trend"] == "stable":
            recommendations.append("Consider advancing to more challenging tasks")
        
        return recommendations
    
    def create_visualization_dashboard(self, learner_id: str, output_dir: str = "analytics_output"):
        """Create comprehensive visualization dashboard"""
        
        Path(output_dir).mkdir(exist_ok=True)
        
        metrics = self.metrics_history.get(learner_id, [])
        if not metrics:
            self.logger.warning(f"No metrics found for learner {learner_id}")
            return
        
        # Set style
        plt.style.use('seaborn-v0_8' if 'seaborn' in plt.style.available else 'default')
        
        # Create performance over time plot
        self._create_performance_timeline(metrics, f"{output_dir}/performance_timeline.png")
        
        # Create task type performance comparison
        self._create_task_type_comparison(metrics, f"{output_dir}/task_type_performance.png")
        
        # Create difficulty progression plot
        self._create_difficulty_progression(metrics, f"{output_dir}/difficulty_progression.png")
        
        # Create metric correlation heatmap
        self._create_correlation_heatmap(metrics, f"{output_dir}/metric_correlations.png")
        
        self.logger.info(f"Visualization dashboard created in {output_dir}")
    
    def _create_performance_timeline(self, metrics: List[PerformanceMetric], output_path: str):
        """Create performance timeline visualization"""
        
        fig, axes = plt.subplots(2, 2, figsize=(15, 10))
        fig.suptitle('Performance Timeline Analysis', fontsize=16)
        
        # Correctness over time
        correctness_metrics = [m for m in metrics if m.metric_name == "correctness_score"]
        if correctness_metrics:
            timestamps = [m.timestamp for m in correctness_metrics]
            scores = [m.value for m in correctness_metrics]
            axes[0, 0].plot(timestamps, scores, 'b-', alpha=0.7)
            axes[0, 0].set_title('Correctness Score Over Time')
            axes[0, 0].set_ylabel('Score')
        
        # Efficiency over time
        efficiency_metrics = [m for m in metrics if m.metric_name == "efficiency_score"]
        if efficiency_metrics:
            timestamps = [m.timestamp for m in efficiency_metrics]
            scores = [m.value for m in efficiency_metrics]
            axes[0, 1].plot(timestamps, scores, 'g-', alpha=0.7)
            axes[0, 1].set_title('Efficiency Score Over Time')
            axes[0, 1].set_ylabel('Score')
        
        # Safety over time
        safety_metrics = [m for m in metrics if m.metric_name == "safety_score"]
        if safety_metrics:
            timestamps = [m.timestamp for m in safety_metrics]
            scores = [m.value for m in safety_metrics]
            axes[1, 0].plot(timestamps, scores, 'r-', alpha=0.7)
            axes[1, 0].set_title('Safety Score Over Time')
            axes[1, 0].set_ylabel('Score')
        
        # Learning rate over time
        learning_metrics = [m for m in metrics if m.metric_name == "learning_rate"]
        if learning_metrics:
            timestamps = [m.timestamp for m in learning_metrics]
            scores = [m.value for m in learning_metrics]
            axes[1, 1].plot(timestamps, scores, 'm-', alpha=0.7)
            axes[1, 1].set_title('Learning Rate Over Time')
            axes[1, 1].set_ylabel('Rate')
        
        plt.tight_layout()
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        plt.close()
    
    def _create_task_type_comparison(self, metrics: List[PerformanceMetric], output_path: str):
        """Create task type performance comparison"""
        
        task_type_performance = self._analyze_task_type_performance(metrics)
        
        if not task_type_performance:
            return
        
        task_types = list(task_type_performance.keys())
        avg_scores = [task_type_performance[tt]["average_score"] for tt in task_types]
        std_scores = [task_type_performance[tt]["score_std"] for tt in task_types]
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
        
        # Average performance by task type
        bars = ax1.bar(task_types, avg_scores, alpha=0.7, color='skyblue')
        ax1.set_title('Average Performance by Task Type')
        ax1.set_ylabel('Average Score')
        ax1.set_xticklabels(task_types, rotation=45, ha='right')
        
        # Add value labels on bars
        for bar, score in zip(bars, avg_scores):
            ax1.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.01,
                    f'{score:.2f}', ha='center', va='bottom')
        
        # Consistency (inverse of std) by task type
        consistency = [1.0 - std for std in std_scores]
        bars2 = ax2.bar(task_types, consistency, alpha=0.7, color='lightcoral')
        ax2.set_title('Consistency by Task Type')
        ax2.set_ylabel('Consistency Score')
        ax2.set_xticklabels(task_types, rotation=45, ha='right')
        
        # Add value labels on bars
        for bar, cons in zip(bars2, consistency):
            ax2.text(bar.get_x() + bar.get_width()/2, bar.get_height() + 0.01,
                    f'{cons:.2f}', ha='center', va='bottom')
        
        plt.tight_layout()
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        plt.close()
    
    def _create_difficulty_progression(self, metrics: List[PerformanceMetric], output_path: str):
        """Create difficulty progression visualization"""
        
        difficulty_progression = self._analyze_difficulty_progression(metrics)
        
        if not difficulty_progression:
            return
        
        difficulties = sorted([int(k.split('_')[1]) for k in difficulty_progression.keys()])
        avg_scores = [difficulty_progression[f"difficulty_{d}"]["average_score"] for d in difficulties]
        attempts = [difficulty_progression[f"difficulty_{d}"]["attempts"] for d in difficulties]
        
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))
        
        # Performance by difficulty
        ax1.plot(difficulties, avg_scores, 'o-', linewidth=2, markersize=8)
        ax1.set_title('Performance vs Difficulty Level')
        ax1.set_xlabel('Difficulty Level')
        ax1.set_ylabel('Average Score')
        ax1.grid(True, alpha=0.3)
        
        # Attempts by difficulty
        ax2.bar(difficulties, attempts, alpha=0.7, color='orange')
        ax2.set_title('Task Attempts by Difficulty Level')
        ax2.set_xlabel('Difficulty Level')
        ax2.set_ylabel('Number of Attempts')
        
        plt.tight_layout()
        plt.savefig(output_path, dpi=300, bbox_inches='tight')
        plt.close()
    
    def _create_correlation_heatmap(self, metrics: List[PerformanceMetric], output_path: str):
        """Create metric correlation heatmap"""
        
        # Prepare data for correlation analysis
        metric_data = defaultdict(list)
        timestamps = []
        
        for metric in metrics:
            if metric.metric_name in ["correctness_score", "efficiency_score", "safety_score", 
                                    "time_efficiency", "learning_rate", "engagement_score"]:
                metric_data[metric.metric_name].append(metric.value)
                if metric.timestamp not in timestamps:
                    timestamps.append(metric.timestamp)
        
        # Create DataFrame
        df_data = {}
        min_length = min(len(values) for values in metric_data.values()) if metric_data else 0
        
        for metric_name, values in metric_data.items():
            df_data[metric_name] = values[:min_length]
        
        if df_data:
            df = pd.DataFrame(df_data)
            correlation_matrix = df.corr()
            
            plt.figure(figsize=(10, 8))
            sns.heatmap(correlation_matrix, annot=True, cmap='coolwarm', center=0,
                       square=True, fmt='.2f')
            plt.title('Metric Correlation Heatmap')
            plt.tight_layout()
            plt.savefig(output_path, dpi=300, bbox_inches='tight')
            plt.close()

class PerformanceAnalyzer:
    """Analyzes performance metrics"""
    
    def __init__(self, config: Dict[str, Any]):
        self.config = config
    
    def analyze_metric_group(self, metrics: List[PerformanceMetric]) -> Dict[str, Any]:
        """Analyze a group of metrics of the same type"""
        
        values = [m.value for m in metrics]
        timestamps = [m.timestamp for m in metrics]
        
        return {
            "count": len(values),
            "mean": np.mean(values),
            "std": np.std(values),
            "min": np.min(values),
            "max": np.max(values),
            "trend": self._calculate_trend(values),
            "recent_performance": np.mean(values[-5:]) if len(values) >= 5 else np.mean(values),
            "improvement_rate": self._calculate_improvement_rate(values, timestamps)
        }
    
    def _calculate_trend(self, values: List[float]) -> str:
        """Calculate trend direction"""
        if len(values) < 3:
            return "insufficient_data"
        
        x = np.arange(len(values))
        slope, _ = np.polyfit(x, values, 1)
        
        if slope > 0.01:
            return "improving"
        elif slope < -0.01:
            return "declining"
        else:
            return "stable"
    
    def _calculate_improvement_rate(self, values: List[float], timestamps: List[float]) -> float:
        """Calculate rate of improvement per day"""
        if len(values) < 2:
            return 0.0
        
        time_span_days = (timestamps[-1] - timestamps[0]) / (24 * 3600)
        if time_span_days == 0:
            return 0.0
        
        performance_change = values[-1] - values[0]
        return performance_change / time_span_days

class TrendAnalyzer:
    """Analyzes learning trends"""
    
    def __init__(self, config: Dict[str, Any]):
        self.config = config
    
    def analyze_trends(self, learner_id: str, metrics: List[PerformanceMetric]) -> List[LearningTrend]:
        """Analyze learning trends from metrics"""
        
        trends = []
        
        # Group metrics by name
        metric_groups = defaultdict(list)
        for metric in metrics:
            metric_groups[metric.metric_name].append(metric)
        
        # Analyze trend for each metric
        for metric_name, metric_list in metric_groups.items():
            if len(metric_list) >= 3:
                trend = self._analyze_single_metric_trend(metric_name, metric_list)
                if trend:
                    trends.append(trend)
        
        return trends
    
    def _analyze_single_metric_trend(self, metric_name: str, 
                                   metrics: List[PerformanceMetric]) -> Optional[LearningTrend]:
        """Analyze trend for a single metric"""
        
        values = [m.value for m in metrics]
        timestamps = [m.timestamp for m in metrics]
        
        # Calculate trend using linear regression
        x = np.arange(len(values))
        slope, intercept = np.polyfit(x, values, 1)
        
        # Calculate R-squared for confidence
        y_pred = slope * x + intercept
        ss_res = np.sum((values - y_pred) ** 2)
        ss_tot = np.sum((values - np.mean(values)) ** 2)
        r_squared = 1 - (ss_res / ss_tot) if ss_tot != 0 else 0
        
        # Determine trend direction
        if abs(slope) < 0.01:
            direction = TrendDirection.STABLE
        elif slope > 0.01:
            direction = TrendDirection.IMPROVING
        else:
            direction = TrendDirection.DECLINING
        
        # Check for volatility
        if np.std(values) > 0.3:
            direction = TrendDirection.VOLATILE
        
        # Calculate duration
        duration_days = (timestamps[-1] - timestamps[0]) / (24 * 3600)
        
        return LearningTrend(
            metric_name=metric_name,
            direction=direction,
            slope=slope,
            confidence=r_squared,
            duration_days=duration_days,
            significance=abs(slope) * r_squared
        )

class AdaptationEngine:
    """Generates adaptation recommendations"""
    
    def __init__(self, config: Dict[str, Any]):
        self.config = config
    
    def generate_recommendations(self, learner_id: str, performance_analysis: Dict[str, Any],
                               trend_analysis: List[LearningTrend], 
                               insights: List[LearningInsight]) -> List[AdaptationRecommendation]:
        """Generate adaptation recommendations"""
        
        recommendations = []
        
        # Performance-based recommendations
        for metric_type, analysis in performance_analysis.items():
            if analysis["mean"] < 0.6:
                recommendations.append(AdaptationRecommendation(
                    recommendation_id=f"perf_{metric_type}_{int(time.time())}",
                    learner_id=learner_id,
                    recommendation_type="performance_improvement",
                    priority=4,
                    description=f"Focus on improving {metric_type} (current average: {analysis['mean']:.2f})",
                    expected_impact=0.3,
                    implementation_effort=3,
                    evidence=[f"Low average {metric_type}: {analysis['mean']:.2f}"],
                    created_timestamp=time.time()
                ))
        
        # Trend-based recommendations
        for trend in trend_analysis:
            if trend.direction == TrendDirection.DECLINING and trend.significance > 0.1:
                recommendations.append(AdaptationRecommendation(
                    recommendation_id=f"trend_{trend.metric_name}_{int(time.time())}",
                    learner_id=learner_id,
                    recommendation_type="trend_correction",
                    priority=5,
                    description=f"Address declining trend in {trend.metric_name}",
                    expected_impact=0.4,
                    implementation_effort=4,
                    evidence=[f"Declining trend: slope={trend.slope:.3f}, confidence={trend.confidence:.2f}"],
                    created_timestamp=time.time()
                ))
        
        # Insight-based recommendations
        for insight in insights:
            if insight.actionable and insight.confidence_score > 0.7:
                recommendations.append(AdaptationRecommendation(
                    recommendation_id=f"insight_{insight.insight_id}",
                    learner_id=learner_id,
                    recommendation_type="insight_based",
                    priority=3,
                    description=insight.description,
                    expected_impact=0.25,
                    implementation_effort=2,
                    evidence=[f"Insight: {insight.title}"],
                    created_timestamp=time.time()
                ))
        
        return sorted(recommendations, key=lambda x: x.priority, reverse=True)

class InsightGenerator:
    """Generates learning insights"""
    
    def __init__(self, config: Dict[str, Any]):
        self.config = config
    
    def generate_insights(self, learner_id: str, metrics: List[PerformanceMetric],
                         trends: List[LearningTrend]) -> List[LearningInsight]:
        """Generate learning insights"""
        
        insights = []
        
        # Performance pattern insights
        insights.extend(self._generate_performance_insights(learner_id, metrics))
        
        # Trend insights
        insights.extend(self._generate_trend_insights(learner_id, trends))
        
        # Learning efficiency insights
        insights.extend(self._generate_efficiency_insights(learner_id, metrics))
        
        return insights
    
    def _generate_performance_insights(self, learner_id: str, 
                                     metrics: List[PerformanceMetric]) -> List[LearningInsight]:
        """Generate performance-based insights"""
        
        insights = []
        
        # Analyze correctness patterns
        correctness_metrics = [m for m in metrics if m.metric_name == "correctness_score"]
        if len(correctness_metrics) >= 10:
            recent_scores = [m.value for m in correctness_metrics[-10:]]
            early_scores = [m.value for m in correctness_metrics[:10]]
            
            improvement = np.mean(recent_scores) - np.mean(early_scores)
            if improvement > 0.2:
                insights.append(LearningInsight(
                    insight_id=f"perf_improvement_{int(time.time())}",
                    learner_id=learner_id,
                    insight_type="performance_improvement",
                    title="Significant Performance Improvement",
                    description=f"Performance has improved by {improvement:.1%} over recent tasks",
                    supporting_data={"improvement": improvement, "recent_avg": np.mean(recent_scores)},
                    actionable=True,
                    confidence_score=0.8,
                    created_timestamp=time.time()
                ))
        
        return insights
    
    def _generate_trend_insights(self, learner_id: str, 
                               trends: List[LearningTrend]) -> List[LearningInsight]:
        """Generate trend-based insights"""
        
        insights = []
        
        for trend in trends:
            if trend.significance > 0.15 and trend.confidence > 0.6:
                if trend.direction == TrendDirection.IMPROVING:
                    insights.append(LearningInsight(
                        insight_id=f"trend_positive_{trend.metric_name}_{int(time.time())}",
                        learner_id=learner_id,
                        insight_type="positive_trend",
                        title=f"Strong Improvement in {trend.metric_name}",
                        description=f"Consistent improvement trend detected in {trend.metric_name}",
                        supporting_data={"slope": trend.slope, "confidence": trend.confidence},
                        actionable=False,
                        confidence_score=trend.confidence,
                        created_timestamp=time.time()
                    ))
        
        return insights
    
    def _generate_efficiency_insights(self, learner_id: str, 
                                    metrics: List[PerformanceMetric]) -> List[LearningInsight]:
        """Generate efficiency-based insights"""
        
        insights = []
        
        # Analyze time efficiency patterns
        time_metrics = [m for m in metrics if m.metric_name == "time_efficiency"]
        if time_metrics:
            avg_efficiency = np.mean([m.value for m in time_metrics])
            if avg_efficiency < 0.5:
                insights.append(LearningInsight(
                    insight_id=f"time_efficiency_{int(time.time())}",
                    learner_id=learner_id,
                    insight_type="efficiency_concern",
                    title="Time Management Opportunity",
                    description="Tasks are taking longer than expected - consider time management strategies",
                    supporting_data={"avg_efficiency": avg_efficiency},
                    actionable=True,
                    confidence_score=0.7,
                    created_timestamp=time.time()
                ))
        
        return insights

def main():
    """Demonstrate learning analytics functionality"""
    
    # Initialize analytics system
    analytics = LearningAnalytics()
    
    # Simulate learner data
    learner_id = "test_learner_analytics"
    
    # Simulate task completions
    for i in range(50):
        # Create mock task
        task = Task(
            task_id=f"task_{i}",
            task_type=TaskType.INSTRUCTION_GENERATION,
            difficulty_level=DifficultyLevel(min(6, (i // 10) + 1)),
            description="Mock task",
            requirements=[],
            constraints=None,
            input_data={},
            expected_output={},
            evaluation_criteria={},
            hints=[],
            time_limit=300,
            created_timestamp=time.time() - (50-i) * 3600
        )
        
        # Create mock result with some progression
        base_score = 0.5 + (i * 0.008) + np.random.normal(0, 0.1)
        result = TaskResult(
            task_id=task.task_id,
            generated_code="mock code",
            execution_time=np.random.uniform(60, 240),
            performance_metrics={},
            correctness_score=max(0, min(1, base_score)),
            efficiency_score=max(0, min(1, base_score + np.random.normal(0, 0.05))),
            safety_score=max(0, min(1, base_score + np.random.normal(0, 0.03))),
            total_score=max(0, min(1, base_score)),
            feedback=[],
            completed_timestamp=time.time() - (50-i) * 3600
        )
        
        # Record completion
        analytics.record_task_completion(learner_id, task, result)
    
    # Analyze performance
    print("Analyzing learner performance...")
    analysis = analytics.analyze_learner_performance(learner_id)
    
    print(f"Performance Analysis Summary:")
    print(f"Total metrics: {analysis['total_metrics']}")
    print(f"Insights generated: {len(analysis['insights'])}")
    print(f"Recommendations: {len(analysis['recommendations'])}")
    
    # Generate progress report
    print("\nGenerating progress report...")
    report = analytics.generate_progress_report(learner_id)
    
    print(f"Progress Report Summary:")
    print(f"Total tasks: {report['performance_summary']['total_tasks_analyzed']}")
    print(f"Average correctness: {report['performance_summary']['average_correctness']:.2f}")
    print(f"Performance trend: {report['performance_summary']['performance_trend']}")
    print(f"Learning milestones: {len(report['learning_milestones'])}")
    
    # Create visualizations
    print("\nCreating visualization dashboard...")
    analytics.create_visualization_dashboard(learner_id)
    
    print("Learning analytics demonstration complete!")

if __name__ == "__main__":
    main()
