 #include "gtest/gtest.h"
 #include "bdi/optimizer/OptimizationEngine.hpp"
 #include "bdi/optimizer/passes/ConstantFolding.hpp"
 #include "bdi/frontend/api/GraphBuilder.hpp"
 #include "bdi/meta/MetadataStore.hpp" // Need store for builder
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
