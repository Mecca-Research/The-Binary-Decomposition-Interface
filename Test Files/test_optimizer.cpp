 #include "gtest/gtest.h"
 #include "bdi/optimizer/OptimizationEngine.hpp"
 #include "bdi/optimizer/passes/ConstantFolding.hpp"
 #include "bdi/frontend/api/GraphBuilder.hpp"
 #include "bdi/meta/MetadataStore.hpp" // Need store for builder
 #include "gtest/gtest.h"
 #include "bdi/optimizer/passes/DeadCodeElimination.hpp" // Include DCE (to be created)
 using namespace bdi::optimizer;
 using namespace bdi::frontend::api;
 using namespace bdi::core::graph;
 using namespace bdi::core::types;
 using namespace bdi::core::payload;
 using namespace bdi::meta;
 TEST(OptimizerTest, ConstantFoldingSimpleAdd) {
    MetadataStore meta_store;
    GraphBuilder builder(meta_store, "ConstFoldTest");
 OptimizationEngine engine;
    engine.addPass(std::make_unique<ConstantFolding>());
    NodeID start = builder.addNode(OpType::META_START);
    NodeID current = start;
    // Use NOPs as constants for now
    NodeID c10 = builder.addNode(OpType::META_NOP);
    builder.setNodePayload(c10, TypedPayload::createFrom(int32_t{10}));
    builder.defineDataOutput(c10, 0, BDIType::INT32);
    builder.connectControl(current, c10);
    current = c10;
    NodeID c20 = builder.addNode(OpType::META_NOP);
    builder.setNodePayload(c20, TypedPayload::createFrom(int32_t{20}));
    builder.defineDataOutput(c20, 0, BDIType::INT32);
    builder.connectControl(current, c20);
    current = c20;
    // Add operation
    NodeID add_node = builder.addNode(OpType::ARITH_ADD);
    builder.defineDataOutput(add_node, 0, BDIType::INT32);
    builder.connectControl(current, add_node); // Control from last const
    builder.connectData(c10, 0, add_node, 0);
    builder.connectData(c20, 0, add_node, 1);
    current = add_node;
    NodeID end = builder.addNode(OpType::META_END);
    builder.connectControl(current, end);
    auto graph = builder.finalizeGraph();
    ASSERT_NE(graph, nullptr);
    size_t initial_node_count = graph->getNodeCount(); // Should be 5
    // Run optimizer
    bool changed = engine.run(*graph);
    ASSERT_TRUE(changed);
    // Verify graph changes
    // Expect: START -> new_const_30 -> END
    // Nodes c10, c20, add_node should be removed
    EXPECT_EQ(graph->getNodeCount(), 3); // Start, New Const, End
    // Find the new constant node (tricky without known ID)
    NodeID folded_const_id = 0;
    for (const auto& pair : *graph) {
        if (pair.second->operation == OpType::META_NOP && pair.first != start) {
             folded_const_id = pair.first;
             break;
        }
    }
    ASSERT_NE(folded_const_id, 0);
    // Check its payload
    auto folded_node_opt = graph->getNode(folded_const_id);
    ASSERT_TRUE(folded_node_opt.has_value());
    auto folded_payload = folded_node_opt.value().get().payload;
    EXPECT_EQ(folded_payload.type, BDIType::INT32);
    EXPECT_EQ(folded_payload.getAs<int32_t>(), 30);
    // Check control flow
    EXPECT_EQ(graph->getNode(start).value().get().control_outputs[0], folded_const_id);
    EXPECT_EQ(graph->getNode(folded_const_id).value().get().control_inputs[0], start);
    EXPECT_EQ(graph->getNode(folded_const_id).value().get().control_outputs[0], end);
    EXPECT_EQ(graph->getNode(end).value().get().control_inputs[0], folded_const_id);
 }
// --- Test Fixture for Optimizer Tests --
class OptimizerTest : public ::testing::Test {
 protected:
    MetadataStore meta_store;
    std::unique_ptr<GraphBuilder> builder;
    OptimizationEngine engine;
    void SetUp() override {
        builder = std::make_unique<GraphBuilder>(meta_store, "OptimizerTestGraph");
        // Add passes in desired order
        engine.addPass(std::make_unique<ConstantFolding>());
        engine.addPass(std::make_unique<DeadCodeElimination>()); // Add DCE pass
        // engine.addPass(std::make_unique<DeadCodeElimination>()); // Add DCE later
    }
    void TearDown() override {
        builder.reset();
    }
    // Helper to create a constant node (using NOP)
    NodeID addConst(int32_t value, NodeID& current_ctl) {
        NodeID node_id = builder->addNode(OpType::META_NOP);
        builder->setNodePayload(node_id, TypedPayload::createFrom(value));
        builder->defineDataOutput(node_id, 0, BDIType::INT32);
        if (current_ctl != 0) builder->connectControl(current_ctl, node_id);
        current_ctl = node_id;
        return node_id;
    }
     NodeID addConst(float value, NodeID& current_ctl) {
        NodeID node_id = builder->addNode(OpType::META_NOP);
        builder->setNodePayload(node_id, TypedPayload::createFrom(value));
        builder->defineDataOutput(node_id, 0, BDIType::FLOAT32);
         if (current_ctl != 0) builder->connectControl(current_ctl, node_id);
        current_ctl = node_id;
        return node_id;
    }
     NodeID addConst(bool value, NodeID& current_ctl) {
        NodeID node_id = builder->addNode(OpType::META_NOP);
        builder->setNodePayload(node_id, TypedPayload::createFrom(value));
        builder->defineDataOutput(node_id, 0, BDIType::BOOL);
         if (current_ctl != 0) builder->connectControl(current_ctl, node_id);
        current_ctl = node_id;
        return node_id;
    }
 };
 TEST_F(OptimizerTest, ConstantFoldingSequence) {
    NodeID start = builder->addNode(OpType::META_START);
    NodeID current = start;
    NodeID c2 = addConst(int32_t{2}, current);
    NodeID c3 = addConst(int32_t{3}, current);
    NodeID c5 = addConst(int32_t{5}, current);
    // add1 = 2 + 3 (= 5)
    NodeID add1 = builder->addNode(OpType::ARITH_ADD);
    builder->defineDataOutput(add1, 0, BDIType::INT32);
    builder->connectControl(current, add1); // Control depends on last const
    builder->connectData(c2, 0, add1, 0);
    builder->connectData(c3, 0, add1, 1);
    current = add1;
    // mul1 = add1 * 5 (= 25)
    NodeID mul1 = builder->addNode(OpType::ARITH_MUL);
    builder->defineDataOutput(mul1, 0, BDIType::INT32);
    builder->connectControl(current, mul1);
    builder->connectData(add1, 0, mul1, 0); // Use result of add1
    builder->connectData(c5, 0, mul1, 1);
    current = mul1;
    NodeID end = builder->addNode(OpType::META_END);
    builder->connectControl(current, end);
    auto graph = builder->finalizeGraph();
    ASSERT_NE(graph, nullptr);
    size_t initial_node_count = graph->getNodeCount(); // Start, c2, c3, c5, add1, mul1, end = 7
    // Run optimizer - may take multiple passes
    bool changed = engine.run(*graph, 5); // Allow multiple passes
    ASSERT_TRUE(changed);
    // Expect: START -> new_const_25 -> END
    // Nodes c2, c3, c5, add1, mul1 should be marked as NOP/deleted by DCE later
    // After folding alone, intermediate nodes might still exist but be NOPs
    // Let's count non-trivial nodes
    size_t final_active_nodes = 0;
    NodeID final_const_id = 0;
    for(const auto& pair : *graph) {
        if(pair.second->operation != OpType::META_NOP || pair.second->payload.data.empty()) {
            if (pair.first != start && pair.first != end) {
                 // If it's not START/END and not a NOP-Constant, something went wrong
                 ASSERT_TRUE(false) << "Unexpected active node found: ID " << pair.first;
            }
             final_active_nodes++; // Count START/END
        } else if (pair.second->operation == OpType::META_NOP && !pair.second->payload.data.empty()){
             final_active_nodes++; // Count the final constant node
             final_const_id = pair.first;
        }
    }
    EXPECT_EQ(final_active_nodes, 3); // START, final CONST, END
    ASSERT_NE(final_const_id, 0);
    // Check final constant value
    auto final_node_opt = graph->getNode(final_const_id);
     ASSERT_TRUE(final_node_opt.has_value());
     EXPECT_EQ(final_node_opt.value().get().payload.getAs<int32_t>(), 25);
     // Check final control flow
     EXPECT_EQ(graph->getNode(start).value().get().control_outputs[0], final_const_id);
     EXPECT_EQ(graph->getNode(final_const_id).value().get().control_outputs[0], end);
 }
 TEST_F(OptimizerTest, ConstantFoldingWithBranch) {
    // Test if constant condition folds the branch path
    NodeID start = builder->addNode(OpType::META_START);
    NodeID current = start;
    NodeID c_true = addConst(bool{true}, current);
    NodeID c10 = addConst(int32_t{10}, current);
    NodeID c20 = addConst(int32_t{20}, current);
    NodeID branch = builder->addNode(OpType::CTRL_BRANCH_COND);
    builder->connectControl(current, branch);
    builder->connectData(c_true, 0, branch, 0);
    current = branch; // Control flow technically ends here, split below
    // True Path -> Add
    NodeID true_path_add = builder->addNode(OpType::ARITH_ADD);
    builder->defineDataOutput(true_path_add, 0, BDIType::INT32);
    builder->connectControl(branch, true_path_add); // True target
    builder->connectData(c10, 0, true_path_add, 0);
    builder->connectData(c20, 0, true_path_add, 1);
    // False Path -> Sub
    NodeID false_path_sub = builder->addNode(OpType::ARITH_SUB);
    builder->defineDataOutput(false_path_sub, 0, BDIType::INT32);
    builder->connectControl(branch, false_path_sub); // False target
    builder->connectData(c10, 0, false_path_sub, 0);
    builder->connectData(c20, 0, false_path_sub, 1);
    // Merge (not strictly needed if one path is eliminated)
    NodeID merge = builder->addNode(OpType::META_NOP);
    builder->connectControl(true_path_add, merge);
    builder->connectControl(false_path_sub, merge);
    NodeID end = builder->addNode(OpType::META_END);
    builder->connectControl(merge, end);
    auto graph = builder->finalizeGraph();
    ASSERT_NE(graph, nullptr);
    // --- Constant Folding Only --
    // Expectation: The ADD operation itself might be folded if inputs are constant,
    // but the ConstantFolding pass defined doesn't simplify control flow.
    bool changed = engine.run(*graph, 5);
    ASSERT_TRUE(changed);
    // Check that ADD node was folded into a const 30 node
     NodeID folded_add_const_id = 0;
     for(const auto& pair : *graph) {
         if(pair.second->operation == OpType::META_NOP && !pair.second->payload.data.empty()) {
             if(pair.second->payload.getAs<int32_t>() == 30) {
                folded_add_const_id = pair.first;
                break;
             }
         }
     }
     ASSERT_NE(folded_add_const_id, 0);
     // Check that SUB node was NOT folded (assuming inputs weren't const to it yet)
     // or folded separately if inputs were const
     bool sub_node_exists = false;
      for(const auto& pair : *graph) {
          if(pair.second->operation == OpType::ARITH_SUB) {
             sub_node_exists = true;
             break;
          }
      }
      EXPECT_TRUE(sub_node_exists); // SUB should still be there
 TEST_F(OptimizerTest, DCEA) {
    NodeID start = builder->addNode(OpType::META_START);
    NodeID current = start;
    NodeID c2 = addConst(int32_t{2}, current);
    NodeID c3 = addConst(int32_t{3}, current);
    NodeID c_unused = addConst(int32_t{99}, current); // This constant is unused
    // add1 = 2 + 3 (= 5) -> Folds to const 5
    NodeID add1 = builder->addNode(OpType::ARITH_ADD);
    builder->defineDataOutput(add1, 0, BDIType::INT32);
    builder->connectControl(current, add1);
    builder->connectData(c2, 0, add1, 0);
    builder->connectData(c3, 0, add1, 1);
    current = add1;
    // Use the result of add1 (which becomes const 5)
    NodeID neg_node = builder->addNode(OpType::ARITH_NEG);
    builder->defineDataOutput(neg_node, 0, BDIType::INT32);
    builder->connectControl(current, neg_node);
    builder->connectData(add1, 0, neg_node, 0); // -> Folds to const -5
    current = neg_node;
    NodeID end = builder->addNode(OpType::META_END); // Essential node
    builder->connectControl(current, end);
    auto graph = builder->finalizeGraph();
    ASSERT_NE(graph, nullptr);
    size_t initial_node_count = graph->getNodeCount(); // Start, c2, c3, c_unused, add1, neg, end = 7
    // Run optimizer (Folding + DCE)
    bool changed = engine.run(*graph, 5);
    ASSERT_TRUE(changed);
    // Expect: START -> new_const_-5 -> END
    // Nodes c2, c3, c_unused, add1 (folded), neg (folded) should be gone.
    EXPECT_EQ(graph->getNodeCount(), 3);
    // Verify remaining nodes and connections
    NodeID final_const_id = 0;
     for (const auto& pair : *graph) {
         if (pair.second->operation == OpType::META_NOP && !pair.second->payload.data.empty()) {
             final_const_id = pair.first;
             break;
         }
     }
     ASSERT_NE(final_const_id, 0);
     EXPECT_EQ(graph->getNode(final_const_id).value().get().payload.getAs<int32_t>(), -5); // Check final result
     EXPECT_EQ(graph->getNode(start).value().get().control_outputs[0], final_const_id);
     EXPECT_EQ(graph->getNode(final_const_id).value().get().control_outputs[0], end);
     EXPECT_EQ(graph->getNode(end).value().get().control_inputs[0], final_const_id);
 }
    // --- TODO: Future Test with Branch Elimination Pass --
    // Add a specific Branch Elimination pass to the engine
    // engine.addPass(std::make_unique<ConstantBranchElimination>());
    // changed = engine.run(*graph);
    // ASSERT_TRUE(changed);
    // // Expectation: START -> c_true -> c10 -> c20 -> new_const_30 -> END (False path removed)
    // Check node count, check false path node is gone.
    GTEST_SKIP() << "Skipping Branch Elimination check - Requires dedicated pass.";
 }
