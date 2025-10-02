
/**
 * @file test_numa_topology.c
 * @brief Test NUMA topology detection
 */

#include "numa_topology.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    printf("Testing NUMA topology detection...\n\n");
    
    // Initialize
    numa_topology_t* topo = numa_topology_init();
    assert(topo != NULL);
    
    // Print topology
    numa_topology_print();
    
    // Test CPU to node mapping
    int cpu = numa_topology_current_cpu();
    printf("Current CPU: %d\n", cpu);
    
    if (cpu >= 0) {
        int node = numa_topology_cpu_to_node(cpu);
        printf("Current NUMA node: %d\n", node);
        assert(node >= 0);
    }
    
    // Test distance matrix
    if (topo->num_nodes > 1) {
        int dist = numa_topology_distance(0, 1);
        printf("Distance from node 0 to node 1: %d\n", dist);
        assert(dist > 0);
    }
    
    // Test availability
    bool available = numa_topology_available();
    printf("NUMA available: %s\n", available ? "yes" : "no");
    
    // Cleanup
    numa_topology_destroy();
    
    printf("\nAll NUMA topology tests passed!\n");
    return 0;
}
