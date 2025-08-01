#ifndef BDIOS_GENESISGRAPH_HPP 
#define BDIOS_GENESISGRAPH_HPP 
#include "BDIGraph.hpp" 
#include "GraphBuilder.hpp" 
#include "MetadataStore.hpp" 
namespace bdios { 
/** 
 * @brief Generates the initial BDIOS Genesis Graph. 
 * @param builder An initialized GraphBuilder. 
 * @param meta_store The metadata store. 
 * @param service_entry_nodes Map containing NodeIDs for core service entry points (Allocator Init, Scheduler Init etc.). 
 * @param scheduler_loop_entry_id NodeID of the main scheduler loop entry. 
 * @return True on success, false on failure. 
 */ 
bool generateGenesisGraph( 
    bdi::frontend::api::GraphBuilder& builder, 
    bdi::meta::MetadataStore& meta_store, 
const std::map<std::string, bdi::core::graph::NodeID>& service_entry_nodes, 
    bdi::core::graph::NodeID scheduler_loop_entry_id 
};
} // namespace bdios 
#endif // BDIOS_GENESISGRAPH_HPP 
// File: bdios/GenesisGraph.cpp 
#include "GenesisGraph.hpp" 
#include "services/MemoryAllocator.hpp" // For service ID 
#include "services/Scheduler.hpp"     
// For service ID 
// ... include other service headers ... 
#include "bdios/CoreBDIGraphHelpers.hpp" // For helpers 
namespace bdios { 
using namespace bdi::core::graph; 
using namespace bdi::core::types; 
using namespace bdi::frontend::api; 
using namespace bdios::helpers; 
bool generateGenesisGraph( 
    GraphBuilder& builder, 
    MetadataStore& meta_store, 
const std::map<std::string, NodeID>& service_entry_nodes, 
    NodeID scheduler_loop_entry_id) 
{ 
std::cout << "Generating BDIOS Genesis Graph..." << std::endl; 
try { 
        NodeID start_node = builder.addNode(BDIOperationType::META_START, "BDIOS_Genesis"); 
        NodeID current_cfg = start_node; 
// --- Assume Firmware Info Block address is available (e.g., passed in Arg0 to START?) --- 
// NodeID fw_info_addr_node = addFuncInput(builder, 0, BDIType::POINTER, current_cfg); 
// --- Call Allocator Init --- 
// NodeID alloc_init_call = addOsServiceCall(builder, services::ALLOCATOR_SERVICE_ID, services::AllocatorOp::INIT, {fw_info_addr_node}
 // current_cfg = alloc_init_call; 
std::cout << "  -> Adding Allocator Init Call (Conceptual)" << std::endl; 
// --- Call Scheduler Init --- 
// NodeID sched_init_call = addOsServiceCall(builder, services::SCHEDULER_SERVICE_ID, services::SchedulerOp::INIT, {}, current_cfg); 
// current_cfg = sched_init_call; 
std::cout << "  -> Adding Scheduler Init Call (Conceptual)" << std::endl; 
// --- Call Event Dispatcher Init --- 
// ... 
// --- Call Ledger Init --- 
// ... 
// --- Load Idle Task --- 
// NodeID idle_task_entry = ... // Get NodeID for idle task graph entry 
// NodeID idle_task_size = addConstU64(builder, 0, current_cfg); // Size 0 context? 
// NodeID add_idle_call = addOsServiceCall(builder, services::SCHEDULER_SERVICE_ID, services::SchedulerOp::ADD_TASK, {idle_task_entry,
 // current_cfg = add_idle_call; 
std::cout << "  -> Adding Load Idle Task Call (Conceptual)" << std::endl; 
// --- Jump to Scheduler Loop --- 
        NodeID jump_to_sched = builder.addNode(BDIOperationType::CTRL_JUMP); 
auto* jump_node = builder.getGraph().getNodeMutable(jump_to_sched); // Need mutable access 
// jump_node->control_outputs = {scheduler_loop_entry_id}; // Set target directly? Or via op_data? 
        builder.connectControl(current_cfg, jump_to_sched); 
std::cout << "Genesis Graph Generation Complete (Conceptual)." << std::endl; 
return true;
    } 
    }
 } 
catch (const std::exception& e) { 
std::cerr << "Error generating Genesis Graph: " << e.what() << std::endl; 
return false; 
} // namespace bdios
