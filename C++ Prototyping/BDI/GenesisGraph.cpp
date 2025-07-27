#include "GenesisGraph.hpp" 
#include "MemoryAllocator.hpp" // For service ID / Op Enum 
#include "Scheduler.hpp"     
// For service ID / Op Enum 
// ... include other service/task headers ... 
#include "CoreBDIGraphHelpers.hpp" 
namespace bdios { 
// Assume helper functions like addOsServiceCall, addConstU64 etc. exist and work 
// Assume mechanism exists to get Entry NodeIDs for loaded BDI graphs 
// NodeID getAllocatorInitEntry(); NodeID getSchedulerInitEntry(); etc. 
// NodeID getAllocatorServiceEntry(); NodeID getSchedulerServiceEntry(); etc. 
// NodeID getIdleTaskEntry(); NodeID getSchedulerLoopEntry(); 
bool generateGenesisGraph( 
    GraphBuilder& builder, 
    MetadataStore& meta_store, 
    NodeID allocator_init_entry, 
    NodeID scheduler_init_entry, 
    NodeID event_init_entry, 
    NodeID ledger_init_entry, 
    NodeID idle_task_entry, // Entry point of the compiled idle task BDI graph 
    NodeID scheduler_loop_entry // Entry point of the compiled scheduler loop BDI graph 
/* Removed service_entry_nodes map - assumes lookup mechanism */ 
/* Removed scheduler_loop_entry_id - assumes lookup mechanism */ 
        ) 
{ 
std::cout << "Generating Final BDIOS Genesis Graph..." << std::endl; 
try { 
            NodeID start = builder.addNode(OpType::META_START, "BDIOS_Genesis"); 
            NodeID current_cfg = start; 
// Assume Firmware Info Ptr available via SYS_REG_READ or initial context 
// --- Initialize Core Services --- 
// Convention: Init functions take FW Info Ptr (if needed), return bool success 
std::cout << "  Generating Allocator Init Call..." << std::endl; 
            NodeID alloc_init_call = addOsServiceCall(builder, services::ALLOCATOR_SERVICE_ID, services::AllocatorOp::INIT, { /* FW Info Ref *
 // TODO: Add META_ASSERT to check success output of service call 
            current_cfg = alloc_init_call; // Assuming synchronous call returns control here 
std::cout << "  Generating Scheduler Init Call..." << std::endl; 
            NodeID sched_init_call = addOsServiceCall(builder, services::SCHEDULER_SERVICE_ID, services::SchedulerOp::INIT, {}, current_cfg, "
            current_cfg = sched_init_call; 
std::cout << "  Generating Event Dispatcher Init Call..." << std::endl; 
            NodeID event_init_call = addOsServiceCall(builder, services::EVENT_SERVICE_ID, services::EventOp::INIT, {}, current_cfg, "CallEven
            current_cfg = event_init_call; 
// --- Load Initial Idle Task --- 
std::cout << "  Generating Add Idle Task Call..." << std::endl; 
             NodeID idle_entry_const = addConstU64(builder, getIdleTaskEntry(), current_cfg, "IdleEntryConst"); 
             NodeID idle_prio_const = addConstU32(builder, 0, current_cfg, "IdlePrioConst"); // Lowest priority 
             NodeID idle_stack_const = addConstU64(builder, 4096, current_cfg, "IdleStackConst"); // 4k stack 
             current_cfg = addOsServiceCall(builder, services::SCHEDULER_SERVICE_ID, services::SchedulerOp::ADD_TASK, {idle_entry_const, idle_
 // --- Jump to Scheduler Loop --- 
std::cout << "  Generating Jump to Scheduler..." << std::endl; 
            NodeID sched_loop_entry = getSchedulerLoopEntry(); // Get scheduler entry point 
            NodeID jump_to_sched = builder.addNode(OpType::CTRL_JUMP); 
auto* jump_node = builder.getGraph().getNodeMutable(jump_to_sched); 
            jump_node->control_outputs = {sched_loop_entry}; // Link directly (or via target stored in payload?) 
            builder.connectControl(current_cfg, jump_to_sched); 
std::cout << "Genesis Graph Generation Complete." << std::endl; 

std::cout << "Generating Final BDIOS Genesis Graph..." << std::endl; 
try { 
        NodeID start = builder.addNode(OpType::META_START, "BDIOS_Genesis"); 
        NodeID current_cfg = start; 
// Assume Firmware Info Ptr is implicit arg0 or loaded via SYS_REG_READ 
// Call Initializers using OS_SERVICE_CALL pattern (conceptual helper) 
        current_cfg = addOsServiceCall(builder, ALLOCATOR_SERVICE_ID, AllocatorOp::INIT, { /* FW Info Ref */ }, current_cfg); 
        current_cfg = addOsServiceCall(builder, SCHEDULER_SERVICE_ID, SchedulerOp::INIT, {}, current_cfg); 
        current_cfg = addOsServiceCall(builder, EVENT_SERVICE_ID, EventOp::INIT, {}, current_cfg); 
        current_cfg = addOsServiceCall(builder, LEDGER_SERVICE_ID, LedgerOp::INIT, {}, current_cfg); 
// Load Idle Task 
        NodeID idle_entry_const = addConstU64(builder, idle_task_entry, current_cfg); 
        NodeID idle_prio_const = addConstU32(builder, 0, current_cfg); // Lowest priority 
        current_cfg = addOsServiceCall(builder, SCHEDULER_SERVICE_ID, SchedulerOp::ADD_TASK, {idle_entry_const, idle_prio_const}, current_cfg)
 // Jump to Scheduler Loop 
        NodeID jump_to_sched = builder.addNode(OpType::CTRL_JUMP); 
auto* jump_node = builder.getGraph().getNodeMutable(jump_to_sched); 
        jump_node->control_outputs = {scheduler_loop_entry}; // Set target 
        builder.connectControl(current_cfg, jump_to_sched); 
std::cout << "Genesis Graph Generation Complete." << std::endl; 
return true;
return true; 
        } 
catch (const std::exception& e) { /* ... error ... */ return false; } 
    }
 } // namespace 
