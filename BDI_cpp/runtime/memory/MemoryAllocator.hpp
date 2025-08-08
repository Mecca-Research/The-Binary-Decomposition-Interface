#ifndef BDIOS_SERVICES_MEMORYALLOCATOR_HPP 
#define BDIOS_SERVICES_MEMORYALLOCATOR_HPP 
#include "BDIGraph.hpp" 
#include "GraphBuilder.hpp" 
#include "MetadataStore.hpp" 
namespace bdios::services { 
// Service ID for the allocator 
const uint64_t ALLOCATOR_SERVICE_ID = 1; 
enum AllocatorOp : uint32_t { 
    INIT = 0, 
    ALLOC = 1, 
    FREE = 2 
}; 
/** 
 * @brief Generates the BDI graph implementing the free-list memory allocator service. 
 * @param builder The GraphBuilder instance to use. 
 * @param meta_store The MetadataStore instance. 
 * @return The NodeID of the entry point for the allocator service graph. 
 */ 
bdi::core::graph::NodeID generateAllocatorGraph( 
    bdi::frontend::api::GraphBuilder& builder, 
    bdi::meta::MetadataStore& meta_store 
); 
} // namespace bdios::services 
#endif // BDIOS_SERVICES_MEMORYALLOCATOR_HPP 
