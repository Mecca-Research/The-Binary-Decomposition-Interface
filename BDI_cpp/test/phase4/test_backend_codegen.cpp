
#include "gtest/gtest.h"
#include "Architecture.hpp"
#include "InstructionSelection.hpp"
#include "RegisterAllocator.hpp"
#include "CodeEmitter.hpp"
#include "SsaTypes.hpp"
#include "GraphBuilder.hpp"
#include "MetadataStore.hpp"

using namespace bdi::compiler::backend;
using namespace bdi::compiler::ssa;
using namespace bdi::frontend::api;
using namespace bdi::meta;

class BackendCodegenTest : public ::testing::Test {
protected:
    void SetUp() override {
        x86_arch = std::make_unique<X86_64Architecture>();
        arm_arch = std::make_unique<ARM64Architecture>();
        meta_store = std::make_unique<MetadataStore>();
    }
    
    std::unique_ptr<Architecture> x86_arch;
    std::unique_ptr<Architecture> arm_arch;
    std::unique_ptr<MetadataStore> meta_store;
};

// Architecture Tests
TEST_F(BackendCodegenTest, X86_64ArchitectureInfo) {
    EXPECT_EQ(x86_arch->getType(), ArchType::X86_64);
    EXPECT_EQ(x86_arch->getName(), "x86-64");
    EXPECT_EQ(x86_arch->getPointerSize(), 8);
    EXPECT_EQ(x86_arch->getRegisterCount(), 16);
    EXPECT_TRUE(x86_arch->isLittleEndian());
    EXPECT_EQ(x86_arch->getStackAlignment(), 16);
}

TEST_F(BackendCodegenTest, ARM64ArchitectureInfo) {
    EXPECT_EQ(arm_arch->getType(), ArchType::ARM64);
    EXPECT_EQ(arm_arch->getName(), "ARM64");
    EXPECT_EQ(arm_arch->getPointerSize(), 8);
    EXPECT_EQ(arm_arch->getRegisterCount(), 31);
    EXPECT_TRUE(arm_arch->isLittleEndian());
    EXPECT_EQ(arm_arch->getStackAlignment(), 16);
}

TEST_F(BackendCodegenTest, X86_64CallingConvention) {
    auto arg_regs = x86_arch->getArgumentRegisters();
    EXPECT_EQ(arg_regs.size(), 6); // rdi, rsi, rdx, rcx, r8, r9
    
    auto ret_regs = x86_arch->getReturnRegisters();
    EXPECT_EQ(ret_regs.size(), 2); // rax, rdx
}

TEST_F(BackendCodegenTest, ARM64CallingConvention) {
    auto arg_regs = arm_arch->getArgumentRegisters();
    EXPECT_EQ(arg_regs.size(), 8); // x0-x7
    
    auto ret_regs = arm_arch->getReturnRegisters();
    EXPECT_EQ(ret_regs.size(), 2); // x0, x1
}

TEST_F(BackendCodegenTest, ArchitectureFactory) {
    auto arch = ArchitectureFactory::create(ArchType::X86_64);
    ASSERT_NE(arch, nullptr);
    EXPECT_EQ(arch->getType(), ArchType::X86_64);
    
    auto native = ArchitectureFactory::createNative();
    ASSERT_NE(native, nullptr);
}

// Instruction Selection Tests
TEST_F(BackendCodegenTest, InstructionSelectorCreation) {
    InstructionSelector selector(*x86_arch);
    // Should initialize with architecture-specific patterns
}

TEST_F(BackendCodegenTest, PatternMatching) {
    InstructionSelector selector(*x86_arch);
    
    GraphBuilder builder(*meta_store, "test");
    NodeID add = builder.addNode(OpType::ARITH_ADD);
    auto graph = builder.finalizeGraph();
    
    auto pattern = selector.findBestPattern(add, *graph);
    ASSERT_NE(pattern, nullptr);
    EXPECT_EQ(pattern->bdi_op, BDIOperationType::ARITH_ADD);
}

TEST_F(BackendCodegenTest, InstructionSelectionX86) {
    InstructionSelector selector(*x86_arch);
    
    SsaForm ssa;
    ssa.createVariable("x", 0, BDIType::INT32, 1);
    ssa.createVariable("y", 0, BDIType::INT32, 1);
    
    GraphBuilder builder(*meta_store, "test");
    auto graph = builder.finalizeGraph();
    
    auto instructions = selector.selectInstructions(ssa, *graph);
    EXPECT_GT(instructions.size(), 0);
}

TEST_F(BackendCodegenTest, InstructionSelectionARM) {
    InstructionSelector selector(*arm_arch);
    
    SsaForm ssa;
    ssa.createVariable("x", 0, BDIType::INT32, 1);
    
    GraphBuilder builder(*meta_store, "test");
    auto graph = builder.finalizeGraph();
    
    auto instructions = selector.selectInstructions(ssa, *graph);
    EXPECT_GT(instructions.size(), 0);
}

TEST_F(BackendCodegenTest, CustomPattern) {
    InstructionSelector selector(*x86_arch);
    
    InstructionPattern pattern("custom", BDIOperationType::ARITH_ADD,
                              {"add", "mov"}, 2);
    selector.addPattern(pattern);
}

// Register Allocation Tests
TEST_F(BackendCodegenTest, InterferenceGraphCreation) {
    InterferenceGraph graph;
    
    graph.addNode(1);
    graph.addNode(2);
    graph.addNode(3);
    
    EXPECT_EQ(graph.getNodes().size(), 3);
}

TEST_F(BackendCodegenTest, InterferenceGraphEdges) {
    InterferenceGraph graph;
    
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    
    EXPECT_TRUE(graph.hasEdge(1, 2));
    EXPECT_TRUE(graph.hasEdge(2, 3));
    EXPECT_FALSE(graph.hasEdge(1, 3));
}

TEST_F(BackendCodegenTest, InterferenceGraphDegree) {
    InterferenceGraph graph;
    
    graph.addEdge(1, 2);
    graph.addEdge(1, 3);
    graph.addEdge(1, 4);
    
    EXPECT_EQ(graph.getDegree(1), 3);
    EXPECT_EQ(graph.getDegree(2), 1);
}

TEST_F(BackendCodegenTest, RegisterAllocationSimple) {
    RegisterAllocator allocator(*x86_arch);
    
    SsaForm ssa;
    auto v1 = ssa.createVariable("x", 0, BDIType::INT32, 1);
    auto v2 = ssa.createVariable("y", 0, BDIType::INT32, 1);
    
    auto result = allocator.allocate(ssa);
    EXPECT_TRUE(result.success || !result.spilled_variables.empty());
}

TEST_F(BackendCodegenTest, RegisterAllocationWithInterference) {
    RegisterAllocator allocator(*x86_arch);
    
    SsaForm ssa;
    // Create many variables to force spilling
    for (int i = 0; i < 20; ++i) {
        ssa.createVariable("v" + std::to_string(i), 0, BDIType::INT32, 1);
    }
    
    auto result = allocator.allocate(ssa);
    // With 20 variables and ~14 allocatable registers, some should spill
}

TEST_F(BackendCodegenTest, GraphColoring) {
    RegisterAllocator allocator(*x86_arch);
    
    InterferenceGraph graph;
    graph.addEdge(1, 2);
    graph.addEdge(2, 3);
    graph.addEdge(3, 1);
    
    auto coloring = allocator.colorGraph(graph, 3);
    
    // Triangle graph needs 3 colors
    EXPECT_EQ(coloring.size(), 3);
    
    // Check that adjacent nodes have different colors
    EXPECT_NE(coloring[1], coloring[2]);
    EXPECT_NE(coloring[2], coloring[3]);
    EXPECT_NE(coloring[3], coloring[1]);
}

TEST_F(BackendCodegenTest, LiveRangeAnalysis) {
    SsaForm ssa;
    ssa.createVariable("x", 0, BDIType::INT32, 1);
    ssa.createVariable("y", 0, BDIType::INT32, 2);
    
    auto ranges = LiveRangeAnalysis::computeLiveRanges(ssa);
    EXPECT_EQ(ranges.size(), 2);
}

TEST_F(BackendCodegenTest, LiveRangeOverlap) {
    LiveRangeAnalysis::LiveRange r1{1, 0, 10};
    LiveRangeAnalysis::LiveRange r2{2, 5, 15};
    LiveRangeAnalysis::LiveRange r3{3, 20, 30};
    
    EXPECT_TRUE(r1.overlaps(r2));
    EXPECT_FALSE(r1.overlaps(r3));
    EXPECT_TRUE(LiveRangeAnalysis::interfere(r1, r2));
    EXPECT_FALSE(LiveRangeAnalysis::interfere(r1, r3));
}

// Code Emission Tests
TEST_F(BackendCodegenTest, CodeBufferCreation) {
    CodeBuffer buffer;
    EXPECT_EQ(buffer.getSize(), 0);
    EXPECT_GT(buffer.getCapacity(), 0);
}

TEST_F(BackendCodegenTest, CodeBufferEmit) {
    CodeBuffer buffer;
    
    buffer.emit8(0x90);  // NOP
    buffer.emit16(0x1234);
    buffer.emit32(0x12345678);
    
    EXPECT_EQ(buffer.getSize(), 7);
}

TEST_F(BackendCodegenTest, CodeBufferResize) {
    CodeBuffer buffer(10);
    
    for (int i = 0; i < 20; ++i) {
        buffer.emit8(0x90);
    }
    
    EXPECT_EQ(buffer.getSize(), 20);
    EXPECT_GE(buffer.getCapacity(), 20);
}

TEST_F(BackendCodegenTest, CodeEmitterX86) {
    CodeEmitter emitter(*x86_arch);
    
    std::vector<MachineInstruction> instructions;
    instructions.emplace_back("add");
    instructions.emplace_back("mov");
    instructions.emplace_back("ret");
    
    AllocationResult allocation;
    allocation.success = true;
    
    auto code = emitter.emit(instructions, allocation);
    ASSERT_NE(code, nullptr);
    EXPECT_GT(code->getSize(), 0);
}

TEST_F(BackendCodegenTest, CodeEmitterARM) {
    CodeEmitter emitter(*arm_arch);
    
    std::vector<MachineInstruction> instructions;
    instructions.emplace_back("add");
    instructions.emplace_back("sub");
    instructions.emplace_back("ret");
    
    AllocationResult allocation;
    allocation.success = true;
    
    auto code = emitter.emit(instructions, allocation);
    ASSERT_NE(code, nullptr);
    EXPECT_GT(code->getSize(), 0);
}

TEST_F(BackendCodegenTest, AssemblyGeneration) {
    CodeEmitter emitter(*x86_arch);
    
    std::vector<MachineInstruction> instructions;
    MachineInstruction instr("add", "Add two registers");
    instr.operands = {0, 1};
    instructions.push_back(instr);
    
    AllocationResult allocation;
    allocation.register_assignment[1] = 0;
    allocation.register_assignment[2] = 1;
    allocation.success = true;
    
    std::string asm_text = emitter.generateAssembly(instructions, allocation);
    EXPECT_FALSE(asm_text.empty());
    EXPECT_NE(asm_text.find("add"), std::string::npos);
}

// Integration Tests
TEST_F(BackendCodegenTest, FullCodeGenerationPipeline) {
    CodeGenerator generator(*x86_arch);
    
    SsaForm ssa;
    ssa.createVariable("x", 0, BDIType::INT32, 1);
    ssa.createVariable("y", 0, BDIType::INT32, 1);
    
    GraphBuilder builder(*meta_store, "test");
    NodeID add = builder.addNode(OpType::ARITH_ADD);
    auto graph = builder.finalizeGraph();
    
    auto code = generator.generate(ssa, *graph);
    ASSERT_NE(code, nullptr);
    EXPECT_GT(code->getSize(), 0);
    
    std::string asm_text = generator.getAssembly();
    EXPECT_FALSE(asm_text.empty());
}

TEST_F(BackendCodegenTest, MultipleInstructions) {
    CodeGenerator generator(*x86_arch);
    
    SsaForm ssa;
    for (int i = 0; i < 10; ++i) {
        ssa.createVariable("v" + std::to_string(i), 0, BDIType::INT32, 1);
    }
    
    GraphBuilder builder(*meta_store, "test");
    auto graph = builder.finalizeGraph();
    
    auto code = generator.generate(ssa, *graph);
    ASSERT_NE(code, nullptr);
}

// Performance Tests
TEST_F(BackendCodegenTest, LargeFunction) {
    CodeGenerator generator(*x86_arch);
    
    SsaForm ssa;
    for (int i = 0; i < 100; ++i) {
        ssa.createVariable("v" + std::to_string(i), 0, BDIType::INT32, 1);
    }
    
    GraphBuilder builder(*meta_store, "large_test");
    auto graph = builder.finalizeGraph();
    
    auto code = generator.generate(ssa, *graph);
    ASSERT_NE(code, nullptr);
}

TEST_F(BackendCodegenTest, ComplexInterferenceGraph) {
    RegisterAllocator allocator(*x86_arch);
    
    InterferenceGraph graph;
    // Create complete graph K10
    for (int i = 1; i <= 10; ++i) {
        for (int j = i + 1; j <= 10; ++j) {
            graph.addEdge(i, j);
        }
    }
    
    auto coloring = allocator.colorGraph(graph, 14);
    // K10 needs 10 colors, should succeed with 14 available
    EXPECT_EQ(coloring.size(), 10);
}

// Edge Cases
TEST_F(BackendCodegenTest, EmptySSA) {
    CodeGenerator generator(*x86_arch);
    
    SsaForm ssa;
    GraphBuilder builder(*meta_store, "empty");
    auto graph = builder.finalizeGraph();
    
    auto code = generator.generate(ssa, *graph);
    ASSERT_NE(code, nullptr);
}

TEST_F(BackendCodegenTest, SingleVariable) {
    RegisterAllocator allocator(*x86_arch);
    
    SsaForm ssa;
    ssa.createVariable("x", 0, BDIType::INT32, 1);
    
    auto result = allocator.allocate(ssa);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.register_assignment.size(), 1);
}

TEST_F(BackendCodegenTest, NoInterference) {
    RegisterAllocator allocator(*x86_arch);
    
    InterferenceGraph graph;
    graph.addNode(1);
    graph.addNode(2);
    graph.addNode(3);
    // No edges - no interference
    
    auto coloring = allocator.colorGraph(graph, 3);
    EXPECT_EQ(coloring.size(), 3);
    
    // All nodes can have the same color
    EXPECT_EQ(coloring[1], coloring[2]);
    EXPECT_EQ(coloring[2], coloring[3]);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
