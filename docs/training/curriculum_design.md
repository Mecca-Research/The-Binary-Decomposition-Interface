
# BDI Curriculum Design

## Philosophy

The BDI curriculum is designed around the principle of **progressive mastery**: AI processes learn fundamental concepts before advancing to more complex topics. This approach ensures:

1. **Solid Foundation**: Basic skills are mastered before advanced topics
2. **Reduced Confusion**: Concepts build naturally on previous knowledge
3. **Measurable Progress**: Clear gates define advancement criteria
4. **Adaptive Learning**: System adjusts to individual learning patterns

## Phase Progression

### Phase 0: Foundations (Beginner)

**Goal**: Establish basic computational and logical reasoning skills

**Topics**:
- Arithmetic: Addition and subtraction (0-100)
- Logic: Basic AND, OR, NOT operations
- Language: Single character recognition
- Code: Simple variable assignments

**Advancement Criteria**:
- 90% accuracy on arithmetic operations
- 85% accuracy on basic logic
- 100 minimum training examples
- 3 consistent sessions

**Typical Duration**: 1-2 hours

### Phase 1: Elementary (Novice)

**Goal**: Expand numerical range and introduce compound operations

**Topics**:
- Arithmetic: Extended range (0-1000), multiplication
- Logic: XOR, NAND, NOR operations
- Language: Bigrams and common pairs
- Code: Simple expressions and operators

**Advancement Criteria**:
- 92% accuracy on extended arithmetic
- 88% accuracy on boolean logic
- 150 minimum training examples
- 3 consistent sessions

**Typical Duration**: 2-4 hours

### Phase 2: Intermediate (Competent)

**Goal**: Introduce modular thinking and uncertainty

**Topics**:
- Arithmetic: Modular arithmetic (mod 2-47)
- Logic: Three-valued logic with unknown
- Language: Trigrams and pattern recognition
- Code: Control flow (if statements)

**Advancement Criteria**:
- 94% accuracy on modular arithmetic
- 90% accuracy on three-valued logic
- 200 minimum training examples
- 4 consistent sessions

**Typical Duration**: 4-8 hours

### Phase 3: Advanced (Proficient)

**Goal**: Master number theory and formal logic

**Topics**:
- Math: GCD, LCM, primality testing
- Logic: Propositional logic, tautologies
- Language: Syntax fragments, grammatical patterns
- Code: Loops and iteration

**Advancement Criteria**:
- 95% accuracy on number theory
- 92% accuracy on propositional logic
- 250 minimum training examples
- 4 consistent sessions

**Typical Duration**: 8-16 hours

### Phase 4: Expert (Advanced)

**Goal**: Handle complex sequences and patterns

**Topics**:
- Math: Fibonacci, Catalan numbers, factorials
- Logic: Predicate logic patterns
- Language: Multi-word phrases, dependencies
- Code: AST patterns, function calls

**Advancement Criteria**:
- 96% accuracy on sequences
- 94% accuracy on predicate logic
- 300 minimum training examples
- 5 consistent sessions

**Typical Duration**: 16-32 hours

### Phase 5: Master (Expert)

**Goal**: Optimize and refine problem-solving approaches

**Topics**:
- Math: Advanced algorithms, optimization
- Logic: Complex compound expressions
- Language: Semantic patterns, meaning
- Code: Design patterns, idioms

**Advancement Criteria**:
- 97% accuracy on algorithms
- 95% accuracy on complex logic
- 350 minimum training examples
- 5 consistent sessions

**Typical Duration**: 32-64 hours

### Phase 6: Virtuoso (Master)

**Goal**: Detect errors and prove correctness

**Topics**:
- Math: Proof techniques, verification
- Logic: Proof patterns, formal verification
- Language: Style analysis, quality metrics
- Code: Bug detection, security analysis

**Advancement Criteria**:
- 98% accuracy on optimization
- 96% accuracy on proof patterns
- 400 minimum training examples
- 6 consistent sessions

**Typical Duration**: 64-128 hours

### Phase 7: Transcendent (Grandmaster)

**Goal**: Synthesize novel solutions and creative approaches

**Topics**:
- Math: Novel problem solving, research-level
- Logic: Creative reasoning, hypothesis generation
- Language: Generation, composition, creativity
- Code: Architecture design, system optimization

**Advancement Criteria**:
- 99% accuracy on all topics
- Creative problem solving demonstrated
- 500 minimum training examples
- 6 consistent sessions

**Typical Duration**: 128+ hours

## Accuracy Gates

### Gate Design Principles

1. **Progressive Difficulty**: Each phase requires higher accuracy
2. **Minimum Samples**: Ensures statistical significance
3. **Consistency**: Multiple sessions prevent lucky streaks
4. **Regression Prevention**: Recent performance must remain high
5. **Adaptive Thresholds**: Can adjust based on difficulty

### Gate Configuration

| Phase | Required Accuracy | Min Samples | Consistency Sessions |
|-------|------------------|-------------|---------------------|
| 0→1   | 90%              | 100         | 3                   |
| 1→2   | 92%              | 150         | 3                   |
| 2→3   | 94%              | 200         | 4                   |
| 3→4   | 95%              | 250         | 4                   |
| 4→5   | 96%              | 300         | 5                   |
| 5→6   | 97%              | 350         | 5                   |
| 6→7   | 98%              | 400         | 6                   |
| 7→∞   | 99%              | 500         | 6                   |

## Content Selection

### Difficulty Scaling

Within each phase, content difficulty gradually increases:

1. **Early Phase**: Simplest examples from phase topic set
2. **Mid Phase**: Mixed difficulty, emphasis on weak areas
3. **Late Phase**: Hardest examples, preparation for next phase

### Topic Balancing

The system ensures balanced coverage across all topics:

- **Math**: 30% of training time
- **Logic**: 25% of training time
- **Language**: 25% of training time
- **Code**: 20% of training time

### Adaptive Selection

Content selection adapts to individual performance:

1. **Strength Reinforcement**: Occasional review of mastered topics
2. **Weakness Focus**: Extra practice on struggling topics
3. **Spaced Repetition**: Revisit topics at increasing intervals
4. **Interleaving**: Mix topics to improve retention

## Progress Metrics

### Per-Phase Metrics

- Total attempts
- Correct/incorrect answers
- Accuracy percentage
- Time spent
- Number of sessions
- First/last attempt timestamps

### Per-Topic Metrics

- Attempts per topic
- Accuracy per topic
- Time per topic
- Strength/weakness identification

### Overall Metrics

- Total training time
- Overall accuracy
- Learning velocity (correct answers per hour)
- Mastery level (0-7)
- Phase progression history

## Learning Patterns

### Typical Learning Curves

1. **Fast Learners**: Advance every 2-4 hours
2. **Average Learners**: Advance every 4-8 hours
3. **Slow Learners**: Advance every 8-16 hours
4. **Struggling Learners**: May need curriculum adjustment

### Common Challenges

1. **Plateau Effect**: Accuracy stalls before gate
   - **Solution**: Increase practice variety, review fundamentals

2. **Regression**: Performance drops after advancement
   - **Solution**: Return to previous phase, strengthen foundation

3. **Topic Imbalance**: Strong in one topic, weak in others
   - **Solution**: Increase focus on weak topics

4. **Burnout**: Declining performance over time
   - **Solution**: Reduce session length, increase breaks

## Personalization

### Individual Adaptation

The system personalizes learning for each AI process:

1. **Pace Adjustment**: Faster/slower progression based on performance
2. **Content Preference**: More of preferred topic types
3. **Difficulty Tuning**: Adjust difficulty within phase
4. **Schedule Optimization**: Best times for training sessions

### Learning Style Detection

The system detects and adapts to learning styles:

1. **Sequential**: Prefers step-by-step progression
2. **Holistic**: Prefers seeing big picture first
3. **Visual**: Benefits from pattern recognition
4. **Analytical**: Prefers logical reasoning

## Future Enhancements

1. **Multi-Modal Learning**: Combine topics in single examples
2. **Transfer Learning**: Apply knowledge across domains
3. **Collaborative Learning**: Learn from other AI processes
4. **Meta-Learning**: Learn how to learn more effectively
5. **Curriculum Generation**: Automatically create new phases
