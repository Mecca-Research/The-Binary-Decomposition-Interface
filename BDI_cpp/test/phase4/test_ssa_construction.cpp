
#include "gtest/gtest.h"
#include "SsaConstruction.hpp"
#include "SsaTypes.hpp"

using namespace bdi::compiler::ssa;

class SsaConstructionTest : public ::testing::Test {
protected:
    void SetUp() override {
        constructor = std::make_unique<SsaConstructor>();
    }
    
    std::unique_ptr<SsaConstructor> constructor;
};

// Basic SSA Construction Tests
TEST_F(SsaConstructionTest, EmptyCFG) {
    ControlFlowGraph cfg;
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
    EXPECT_EQ(ssa->getVariables().size(), 0);
}

TEST_F(SsaConstructionTest, SingleBlockCFG) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    cfg.setEntryBlock(entry);
    
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
}

TEST_F(SsaConstructionTest, LinearCFG) {
    ControlFlowGraph cfg;
    BasicBlockID b1 = cfg.createBlock("b1");
    BasicBlockID b2 = cfg.createBlock("b2");
    BasicBlockID b3 = cfg.createBlock("b3");
    
    cfg.setEntryBlock(b1);
    cfg.addEdge(b1, b2);
    cfg.addEdge(b2, b3);
    
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
}

TEST_F(SsaConstructionTest, DiamondCFG) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    BasicBlockID left = cfg.createBlock("left");
    BasicBlockID right = cfg.createBlock("right");
    BasicBlockID merge = cfg.createBlock("merge");
    
    cfg.setEntryBlock(entry);
    cfg.addEdge(entry, left);
    cfg.addEdge(entry, right);
    cfg.addEdge(left, merge);
    cfg.addEdge(right, merge);
    
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
    
    // Merge block should have phi nodes
    auto merge_block = cfg.getBlock(merge);
    ASSERT_NE(merge_block, nullptr);
}

TEST_F(SsaConstructionTest, LoopCFG) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    BasicBlockID header = cfg.createBlock("header");
    BasicBlockID body = cfg.createBlock("body");
    BasicBlockID exit = cfg.createBlock("exit");
    
    cfg.setEntryBlock(entry);
    cfg.addEdge(entry, header);
    cfg.addEdge(header, body);
    cfg.addEdge(body, header); // Back edge
    cfg.addEdge(header, exit);
    
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
}

// Dominance Tests
TEST_F(SsaConstructionTest, DominanceSimple) {
    ControlFlowGraph cfg;
    BasicBlockID b1 = cfg.createBlock("b1");
    BasicBlockID b2 = cfg.createBlock("b2");
    
    cfg.setEntryBlock(b1);
    cfg.addEdge(b1, b2);
    
    auto dom_info = constructor->computeDominance(cfg);
    
    EXPECT_EQ(dom_info[b1].immediate_dominator, b1); // Entry dominates itself
    EXPECT_EQ(dom_info[b2].immediate_dominator, b1); // b1 dominates b2
}

TEST_F(SsaConstructionTest, DominanceDiamond) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    BasicBlockID left = cfg.createBlock("left");
    BasicBlockID right = cfg.createBlock("right");
    BasicBlockID merge = cfg.createBlock("merge");
    
    cfg.setEntryBlock(entry);
    cfg.addEdge(entry, left);
    cfg.addEdge(entry, right);
    cfg.addEdge(left, merge);
    cfg.addEdge(right, merge);
    
    auto dom_info = constructor->computeDominance(cfg);
    
    EXPECT_EQ(dom_info[entry].immediate_dominator, entry);
    EXPECT_EQ(dom_info[left].immediate_dominator, entry);
    EXPECT_EQ(dom_info[right].immediate_dominator, entry);
    EXPECT_EQ(dom_info[merge].immediate_dominator, entry);
}

TEST_F(SsaConstructionTest, DominanceFrontierDiamond) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    BasicBlockID left = cfg.createBlock("left");
    BasicBlockID right = cfg.createBlock("right");
    BasicBlockID merge = cfg.createBlock("merge");
    
    cfg.setEntryBlock(entry);
    cfg.addEdge(entry, left);
    cfg.addEdge(entry, right);
    cfg.addEdge(left, merge);
    cfg.addEdge(right, merge);
    
    auto dom_info = constructor->computeDominance(cfg);
    
    // Both left and right should have merge in their dominance frontier
    EXPECT_TRUE(dom_info[left].dominance_frontier.find(merge) != 
                dom_info[left].dominance_frontier.end());
    EXPECT_TRUE(dom_info[right].dominance_frontier.find(merge) != 
                dom_info[right].dominance_frontier.end());
}

// Phi Node Tests
TEST_F(SsaConstructionTest, PhiNodeInsertion) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    BasicBlockID left = cfg.createBlock("left");
    BasicBlockID right = cfg.createBlock("right");
    BasicBlockID merge = cfg.createBlock("merge");
    
    cfg.setEntryBlock(entry);
    cfg.addEdge(entry, left);
    cfg.addEdge(entry, right);
    cfg.addEdge(left, merge);
    cfg.addEdge(right, merge);
    
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
    
    // Should have phi nodes for variables defined in both branches
    EXPECT_GT(ssa->getPhiNodes().size(), 0);
}

TEST_F(SsaConstructionTest, PhiNodeOperands) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    BasicBlockID left = cfg.createBlock("left");
    BasicBlockID right = cfg.createBlock("right");
    BasicBlockID merge = cfg.createBlock("merge");
    
    cfg.setEntryBlock(entry);
    cfg.addEdge(entry, left);
    cfg.addEdge(entry, right);
    cfg.addEdge(left, merge);
    cfg.addEdge(right, merge);
    
    auto ssa = constructor->convertToSsa(cfg);
    auto dom_info = constructor->computeDominance(cfg);
    
    SsaForm ssa_form;
    constructor->insertPhiNodes(cfg, dom_info, ssa_form);
    
    // Check that phi nodes have correct number of operands
    for (const auto& phi : ssa_form.getPhiNodes()) {
        auto block = cfg.getBlock(phi.block);
        if (block) {
            EXPECT_EQ(phi.operands.size(), block->predecessors.size());
        }
    }
}

// Variable Renaming Tests
TEST_F(SsaConstructionTest, VariableVersioning) {
    SsaForm ssa;
    
    auto v1 = ssa.createVariable("x", 0, BDIType::INT32, 1);
    auto v2 = ssa.createVariable("x", 1, BDIType::INT32, 2);
    auto v3 = ssa.createVariable("x", 2, BDIType::INT32, 3);
    
    auto var1 = ssa.getVariable(v1);
    auto var2 = ssa.getVariable(v2);
    auto var3 = ssa.getVariable(v3);
    
    ASSERT_NE(var1, nullptr);
    ASSERT_NE(var2, nullptr);
    ASSERT_NE(var3, nullptr);
    
    EXPECT_EQ(var1->version, 0);
    EXPECT_EQ(var2->version, 1);
    EXPECT_EQ(var3->version, 2);
    
    EXPECT_EQ(var1->getFullName(), "x_0");
    EXPECT_EQ(var2->getFullName(), "x_1");
    EXPECT_EQ(var3->getFullName(), "x_2");
}

// SSA Validation Tests
TEST_F(SsaConstructionTest, ValidateSsaForm) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    cfg.setEntryBlock(entry);
    
    SsaForm ssa;
    ssa.createVariable("x", 0, BDIType::INT32, entry);
    
    EXPECT_TRUE(SsaUtils::validateSsa(ssa, cfg));
}

TEST_F(SsaConstructionTest, SsaPrintOutput) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    cfg.setEntryBlock(entry);
    
    SsaForm ssa;
    auto v1 = ssa.createVariable("x", 0, BDIType::INT32, entry);
    auto v2 = ssa.createVariable("y", 0, BDIType::INT32, entry);
    
    std::string output = SsaUtils::printSsa(ssa, cfg);
    EXPECT_FALSE(output.empty());
    EXPECT_NE(output.find("x_0"), std::string::npos);
    EXPECT_NE(output.find("y_0"), std::string::npos);
}

// Complex CFG Tests
TEST_F(SsaConstructionTest, NestedLoops) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    BasicBlockID outer_header = cfg.createBlock("outer_header");
    BasicBlockID inner_header = cfg.createBlock("inner_header");
    BasicBlockID inner_body = cfg.createBlock("inner_body");
    BasicBlockID outer_body = cfg.createBlock("outer_body");
    BasicBlockID exit = cfg.createBlock("exit");
    
    cfg.setEntryBlock(entry);
    cfg.addEdge(entry, outer_header);
    cfg.addEdge(outer_header, inner_header);
    cfg.addEdge(inner_header, inner_body);
    cfg.addEdge(inner_body, inner_header); // Inner loop back edge
    cfg.addEdge(inner_header, outer_body);
    cfg.addEdge(outer_body, outer_header); // Outer loop back edge
    cfg.addEdge(outer_header, exit);
    
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
}

TEST_F(SsaConstructionTest, MultipleExits) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    BasicBlockID body = cfg.createBlock("body");
    BasicBlockID exit1 = cfg.createBlock("exit1");
    BasicBlockID exit2 = cfg.createBlock("exit2");
    
    cfg.setEntryBlock(entry);
    cfg.addEdge(entry, body);
    cfg.addEdge(body, exit1);
    cfg.addEdge(body, exit2);
    
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
}

// Edge Cases
TEST_F(SsaConstructionTest, SelfLoop) {
    ControlFlowGraph cfg;
    BasicBlockID loop = cfg.createBlock("loop");
    
    cfg.setEntryBlock(loop);
    cfg.addEdge(loop, loop); // Self loop
    
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
}

TEST_F(SsaConstructionTest, IrreducibleCFG) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    BasicBlockID b1 = cfg.createBlock("b1");
    BasicBlockID b2 = cfg.createBlock("b2");
    
    cfg.setEntryBlock(entry);
    cfg.addEdge(entry, b1);
    cfg.addEdge(entry, b2);
    cfg.addEdge(b1, b2);
    cfg.addEdge(b2, b1); // Creates irreducible loop
    
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
}

// Performance Tests
TEST_F(SsaConstructionTest, LargeCFG) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    cfg.setEntryBlock(entry);
    
    BasicBlockID prev = entry;
    for (int i = 0; i < 100; ++i) {
        BasicBlockID block = cfg.createBlock("block_" + std::to_string(i));
        cfg.addEdge(prev, block);
        prev = block;
    }
    
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
}

TEST_F(SsaConstructionTest, ManyVariables) {
    SsaForm ssa;
    
    for (int i = 0; i < 1000; ++i) {
        ssa.createVariable("var" + std::to_string(i), 0, BDIType::INT32, 1);
    }
    
    EXPECT_EQ(ssa.getVariables().size(), 1000);
}

// Integration Tests
TEST_F(SsaConstructionTest, FullPipeline) {
    ControlFlowGraph cfg;
    BasicBlockID entry = cfg.createBlock("entry");
    BasicBlockID cond = cfg.createBlock("cond");
    BasicBlockID then_block = cfg.createBlock("then");
    BasicBlockID else_block = cfg.createBlock("else");
    BasicBlockID merge = cfg.createBlock("merge");
    BasicBlockID exit = cfg.createBlock("exit");
    
    cfg.setEntryBlock(entry);
    cfg.addEdge(entry, cond);
    cfg.addEdge(cond, then_block);
    cfg.addEdge(cond, else_block);
    cfg.addEdge(then_block, merge);
    cfg.addEdge(else_block, merge);
    cfg.addEdge(merge, exit);
    
    // Full SSA construction
    auto ssa = constructor->convertToSsa(cfg);
    ASSERT_NE(ssa, nullptr);
    
    // Validate result
    EXPECT_TRUE(SsaUtils::validateSsa(*ssa, cfg));
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
