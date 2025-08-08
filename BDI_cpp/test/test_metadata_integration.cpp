 #include "gtest/gtest.h"
 #include "GraphBuilder.hpp"
 #include "MetadataStore.hpp"
 #include "BDIVirtualMachine.hpp" // Need VM to test assert
 using namespace bdi::frontend::api;
 using namespace bdi::meta;
 using namespace bdi::core::graph;
 using namespace bdi::core::types;
 using namespace bdi::core::payload;
 using namespace bdi::runtime;
 TEST(MetadataIntegrationTest, AddAndRetrieveMetadata) {
    MetadataStore meta_store;
    GraphBuilder builder(meta_store, "MetadataTest");
    SemanticTag st{"Source:A", "Node A"};
    ProofTag pt{ProofTag::ProofSystem::INTERNAL_HASH, {std::byte{1}}};
    NodeID node1 = builder.addNode(OpType::META_NOP, "Node1 Debug Name"); // Adds SemanticTag automatically
    NodeID node2 = builder.addNode(OpType::META_NOP, "", pt); // Add with initial ProofTag
    // Check node1's metadata
    auto h1_opt = builder.getNodeMetadataHandle(node1);
    ASSERT_TRUE(h1_opt.has_value());
    const MetadataVariant* m1 = meta_store.getMetadata(h1_opt.value());
    ASSERT_NE(m1, nullptr);
    ASSERT_TRUE(std::holds_alternative<SemanticTag>(*m1));
    EXPECT_EQ(std::get<SemanticTag>(*m1).description, "Node1 Debug Name");
    // Check node2's metadata
     auto h2_opt = builder.getNodeMetadataHandle(node2);
    ASSERT_TRUE(h2_opt.has_value());
    const MetadataVariant* m2 = meta_store.getMetadata(h2_opt.value());
    ASSERT_NE(m2, nullptr);
    ASSERT_TRUE(std::holds_alternative<ProofTag>(*m2));
    EXPECT_EQ(std::get<ProofTag>(*m2).system, ProofTag::ProofSystem::INTERNAL_HASH);
    // Update node1's metadata
    HardwareHints hh{HardwareHints::CacheLocality::HINT_L1};
    ASSERT_TRUE(builder.setNodeMetadata(node1, hh));
    const MetadataVariant* m1_updated = meta_store.getMetadata(h1_opt.value());
    ASSERT_NE(m1_updated, nullptr);
    EXPECT_TRUE(std::holds_alternative<HardwareHints>(*m1_updated));
 }
 TEST(MetadataIntegrationTest, VMAssertPass) {
    MetadataStore meta_store;
    GraphBuilder builder(meta_store, "AssertPassTest");
    BDIVirtualMachine vm(meta_store, 1024); // Pass store to VM
    NodeID start = builder.addNode(OpType::META_START);
    NodeID current = start;
    NodeID const_true = addConstNode(builder, TypedPayload::createFrom(bool{true}), current);
    NodeID assert_node = builder.addNode(OpType::META_ASSERT, "Should Pass");
    builder.connectControl(current, assert_node);
    builder.connectData(const_true, 0, assert_node, 0); // Condition = true
    current = assert_node;
    NodeID end = builder.addNode(OpType::META_END);
    builder.connectControl(current, end);
    auto graph = builder.finalizeGraph();
    ASSERT_NE(graph, nullptr);
    // Pre-populate
    vm.getExecutionContext()->setPortValue(const_true, 0, BDIValueVariant{bool{true}});
    // Execute - should pass
    EXPECT_TRUE(vm.execute(*graph, start));
 }
 TEST(MetadataIntegrationTest, VMAssertFail) {
    MetadataStore meta_store;
    GraphBuilder builder(meta_store, "AssertFailTest");
    BDIVirtualMachine vm(meta_store, 1024);
    NodeID start = builder.addNode(OpType::META_START);
    NodeID current = start;
    NodeID const_false = addConstNode(builder, TypedPayload::createFrom(bool{false}), current);
    NodeID assert_node = builder.addNode(OpType::META_ASSERT, "Should Fail");
    builder.connectControl(current, assert_node);
    builder.connectData(const_false, 0, assert_node, 0); // Condition = false
    current = assert_node;
    NodeID end = builder.addNode(OpType::META_END); // Might not be reached
    builder.connectControl(current, end);
    auto graph = builder.finalizeGraph();
    ASSERT_NE(graph, nullptr);
    vm.getExecutionContext()->setPortValue(const_false, 0, BDIValueVariant{bool{false}});
    // Execute - should fail
    EXPECT_FALSE(vm.execute(*graph, start));
 }
 // Add test for META_VERIFY_PROOF stub similarly if needed
