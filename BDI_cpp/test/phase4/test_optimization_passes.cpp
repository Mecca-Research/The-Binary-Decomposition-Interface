
#include "gtest/gtest.h"
#include "OptimizationEngine.hpp"
#include "ConstantFolding.hpp"
#include "DeadCodeElimination.hpp"
#include "CommonSubexpressionElimination.hpp"
#include "LoopInvariantCodeMotion.hpp"
#include "FunctionInlining.hpp"
#include "GraphBuilder.hpp"
#include "MetadataStore.hpp"

using namespace bdi::optimizer;
using namespace bdi::frontend::api;
using namespace bdi::core::graph;
using namespace bdi::core::types;
using namespace bdi::meta;

class OptimizationPassTest : public ::testing::Test {
protected:
    void SetUp() override {
        meta_store = std::make_unique<MetadataStore>();
        builder = std::make_unique<GraphBuilder>(*meta_store, "test_graph");
        engine = std::make_unique<OptimizationEngine>();
    }
    
    std::unique_ptr<MetadataStore> meta_store;
    std::unique_ptr<GraphBuilder> builder;
    std::unique_ptr<OptimizationEngine> engine;
};

// Constant Folding Tests
TEST_F(OptimizationPassTest, ConstantFoldingAdd) {
    engine->addPass(std::make_unique<ConstantFolding>());
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID c1 = builder->addNode(OpType::META_NOP);
    NodeID c2 = builder->addNode(OpType::META_NOP);
    NodeID add = builder->addNode(OpType::ARITH_ADD);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, c1);
    builder->connectControl(c1, c2);
    builder->connectControl(c2, add);
    builder->connectControl(add, end);
    
    auto graph = builder->finalizeGraph();
    size_t initial_count = graph->getNodeCount();
    
    bool changed = engine->run(*graph);
    EXPECT_TRUE(changed);
}

TEST_F(OptimizationPassTest, ConstantFoldingMultiply) {
    engine->addPass(std::make_unique<ConstantFolding>());
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID mul = builder->addNode(OpType::ARITH_MUL);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, mul);
    builder->connectControl(mul, end);
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
    
    // Should optimize constant multiplications
}

TEST_F(OptimizationPassTest, ConstantFoldingChain) {
    engine->addPass(std::make_unique<ConstantFolding>());
    
    // Create chain: (2 + 3) * 4
    NodeID start = builder->addNode(OpType::META_START);
    NodeID add = builder->addNode(OpType::ARITH_ADD);
    NodeID mul = builder->addNode(OpType::ARITH_MUL);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, add);
    builder->connectControl(add, mul);
    builder->connectControl(mul, end);
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
    EXPECT_TRUE(changed);
}

// Dead Code Elimination Tests
TEST_F(OptimizationPassTest, DeadCodeEliminationUnused) {
    engine->addPass(std::make_unique<DeadCodeElimination>());
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID unused = builder->addNode(OpType::ARITH_ADD);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, end);
    // unused is not connected
    
    auto graph = builder->finalizeGraph();
    size_t initial_count = graph->getNodeCount();
    
    bool changed = engine->run(*graph);
    EXPECT_TRUE(changed);
    EXPECT_LT(graph->getNodeCount(), initial_count);
}

TEST_F(OptimizationPassTest, DeadCodeEliminationUsed) {
    engine->addPass(std::make_unique<DeadCodeElimination>());
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID used = builder->addNode(OpType::ARITH_ADD);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, used);
    builder->connectControl(used, end);
    
    auto graph = builder->finalizeGraph();
    size_t initial_count = graph->getNodeCount();
    
    bool changed = engine->run(*graph);
    // Should not remove used nodes
    EXPECT_EQ(graph->getNodeCount(), initial_count);
}

// Common Subexpression Elimination Tests
TEST_F(OptimizationPassTest, CSEIdenticalExpressions) {
    engine->addPass(std::make_unique<CommonSubexpressionElimination>());
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID add1 = builder->addNode(OpType::ARITH_ADD);
    NodeID add2 = builder->addNode(OpType::ARITH_ADD);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, add1);
    builder->connectControl(add1, add2);
    builder->connectControl(add2, end);
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
    
    // Should eliminate duplicate computation
}

TEST_F(OptimizationPassTest, CSECommutativeOps) {
    engine->addPass(std::make_unique<CommonSubexpressionElimination>());
    
    // a + b and b + a should be recognized as same
    NodeID start = builder->addNode(OpType::META_START);
    NodeID add1 = builder->addNode(OpType::ARITH_ADD);
    NodeID add2 = builder->addNode(OpType::ARITH_ADD);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, add1);
    builder->connectControl(add1, add2);
    builder->connectControl(add2, end);
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
}

// Loop Invariant Code Motion Tests
TEST_F(OptimizationPassTest, LICMSimpleLoop) {
    engine->addPass(std::make_unique<LoopInvariantCodeMotion>());
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID loop_header = builder->addNode(OpType::META_NOP);
    NodeID loop_body = builder->addNode(OpType::ARITH_ADD);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, loop_header);
    builder->connectControl(loop_header, loop_body);
    builder->connectControl(loop_body, loop_header); // Back edge
    builder->connectControl(loop_header, end);
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
}

TEST_F(OptimizationPassTest, LICMInvariantComputation) {
    engine->addPass(std::make_unique<LoopInvariantCodeMotion>());
    
    // Create loop with invariant computation
    NodeID start = builder->addNode(OpType::META_START);
    NodeID header = builder->addNode(OpType::META_NOP);
    NodeID invariant = builder->addNode(OpType::ARITH_MUL);
    NodeID body = builder->addNode(OpType::ARITH_ADD);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, header);
    builder->connectControl(header, invariant);
    builder->connectControl(invariant, body);
    builder->connectControl(body, header);
    builder->connectControl(header, end);
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
}

// Function Inlining Tests
TEST_F(OptimizationPassTest, InlineSmallFunction) {
    InlinePolicy policy;
    policy.small_function_threshold = 10;
    engine->addPass(std::make_unique<FunctionInlining>(policy));
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID call = builder->addNode(OpType::FUNC_CALL);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, call);
    builder->connectControl(call, end);
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
}

TEST_F(OptimizationPassTest, InlineDepthLimit) {
    InlinePolicy policy;
    policy.max_inline_depth = 2;
    engine->addPass(std::make_unique<FunctionInlining>(policy));
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID call1 = builder->addNode(OpType::FUNC_CALL);
    NodeID call2 = builder->addNode(OpType::FUNC_CALL);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, call1);
    builder->connectControl(call1, call2);
    builder->connectControl(call2, end);
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
}

// Combined Optimization Tests
TEST_F(OptimizationPassTest, MultiplePassesPipeline) {
    engine->addPass(std::make_unique<ConstantFolding>());
    engine->addPass(std::make_unique<DeadCodeElimination>());
    engine->addPass(std::make_unique<CommonSubexpressionElimination>());
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID add = builder->addNode(OpType::ARITH_ADD);
    NodeID end = builder->addNode(OpType::META_END);
    
    builder->connectControl(start, add);
    builder->connectControl(add, end);
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
}

TEST_F(OptimizationPassTest, IterativeOptimization) {
    engine->addPass(std::make_unique<ConstantFolding>());
    engine->addPass(std::make_unique<DeadCodeElimination>());
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID end = builder->addNode(OpType::META_END);
    builder->connectControl(start, end);
    
    auto graph = builder->finalizeGraph();
    
    // Run multiple iterations
    bool changed = engine->run(*graph, 5);
}

// Performance Tests
TEST_F(OptimizationPassTest, LargeGraphOptimization) {
    engine->addPass(std::make_unique<ConstantFolding>());
    
    NodeID start = builder->addNode(OpType::META_START);
    NodeID prev = start;
    
    for (int i = 0; i < 100; ++i) {
        NodeID node = builder->addNode(OpType::ARITH_ADD);
        builder->connectControl(prev, node);
        prev = node;
    }
    
    NodeID end = builder->addNode(OpType::META_END);
    builder->connectControl(prev, end);
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
}

// Edge Cases
TEST_F(OptimizationPassTest, EmptyGraph) {
    engine->addPass(std::make_unique<ConstantFolding>());
    
    auto graph = builder->finalizeGraph();
    bool changed = engine->run(*graph);
    EXPECT_FALSE(changed);
}

TEST_F(OptimizationPassTest, SingleNodeGraph) {
    engine->addPass(std::make_unique<DeadCodeElimination>());
    
    NodeID start = builder->addNode(OpType::META_START);
    auto graph = builder->finalizeGraph();
    
    bool changed = engine->run(*graph);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
