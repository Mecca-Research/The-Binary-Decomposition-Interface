#include "gtest.h" 
#include "BDIVirtualMachine.hpp" 
#include "GraphBuilder.hpp" 
#include "MetadataStore.hpp" 
#include "ProofVerifier.hpp" // Stub 
#include "X86_64_HAL.hpp" // Stub HAL 
#include "MemoryAllocator.hpp" // Include generator function header 
// ... include other service headers ... 
TEST(BDIOSLevelTest, AllocatorServiceCall) { 
    bdi::hal::X86_64_HAL hal; // Use stub HAL 
    bdi::meta::MetadataStore meta_store; 
    bdi::verification::StubProofVerifier verifier; 
    bdi::runtime::BDIVirtualMachine vm(hal, meta_store, verifier, 1024*1024); // 1MB 
// 1. Generate Allocator Service Graph 
    bdi::frontend::api::GraphBuilder allocator_builder(meta_store, "AllocatorService"); 
    NodeID allocator_entry = bdios::services::generateAllocatorGraph(allocator_builder, meta_store, 0, 0); // Pass dummy state addresses 
auto allocator_graph = allocator_builder.finalizeGraph(); 
// TODO: Need way to load multiple graphs into VM or link them 
// 2. Generate Test Graph that calls the service 
    bdi::frontend::api::GraphBuilder test_builder(meta_store, "TestClient"); 
    NodeID start = test_builder.addNode(OpType::META_START); 
    NodeID current = start; 
    NodeID size_const = addConstU64(test_builder, 128, current); // Allocate 128 bytes 
    NodeID op_const = addConstU32(test_builder, services::AllocatorOp::ALLOC, current); // Alloc operation 
    NodeID service_call = test_builder.addNode(OpType::OS_SERVICE_CALL); 
    test_builder.setNodePayload(service_call, TypedPayload::createFrom(services::ALLOCATOR_SERVICE_ID)); 
    test_builder.defineDataOutput(service_call, 0, BDIType::POINTER); // Expect address back 
    test_builder.connectControl(current, service_call); 
    test_builder.connectData(op_const, 0, service_call, 0);     
    test_builder.connectData(size_const, 0, service_call, 1);    
    current = service_call; 
    NodeID end = test_builder.addNode(OpType::META_END); 
    test_builder.connectControl(current, end); 
auto test_graph = test_builder.finalizeGraph(); 
// Input 0: Op Code 
// Input 1: Size 
// 3. Run Test Graph (Needs VM support for multiple graphs / service calls) 
// ASSERT_TRUE(vm.execute(*test_graph, start)); 
// 4. Check result (output of service_call node) 
// auto result_addr = vm.getExecutionContext()->getPortValue(service_call, 0); 
// ASSERT_TRUE(result_addr.has_value()); 
// EXPECT_NE(convertValueOrThrow<uintptr_t>(result_addr.value()), 0); // Should get non-zero address 
    GTEST_SKIP() << "Skipping BDIOS Allocator Test - Requires VM multi-graph/service call support."; 
}
