namespace bdios { 
// ... includes ... 
// Assume service graphs are compiled and entry points known 
// NodeID ALLOCATOR_INIT_ENTRY = ...; NodeID SCHEDULER_INIT_ENTRY = ...; etc. 
bool generateGenesisGraph( 
    GraphBuilder& builder, 
    MetadataStore& meta_store, 
    NodeID allocator_init_entry, 
    NodeID scheduler_init_entry, 
    NodeID event_init_entry, 
    NodeID ledger_init_entry, 
    NodeID idle_task_entry, // Entry point of the compiled idle task BDI graph 
    NodeID scheduler_loop_entry // Entry point of the compiled scheduler loop BDI graph 
) { 
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
    } 
} 
catch (const std::exception& e) { /* ... error ... */ return false; }
 } // namespace
