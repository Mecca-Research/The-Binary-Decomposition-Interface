
/**
 * @file numa_topology.h
 * @brief NUMA topology detection and management
 * 
 * Provides NUMA node discovery, CPU-to-node mapping, and distance matrix.
 * Integrates with Linux sysfs for topology information.
 */

#ifndef PHASE2_NUMA_TOPOLOGY_H
#define PHASE2_NUMA_TOPOLOGY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Maximum NUMA nodes supported
#define NUMA_MAX_NODES 8

// Maximum CPUs per node
#define NUMA_MAX_CPUS_PER_NODE 64

// Maximum total CPUs
#define NUMA_MAX_CPUS (NUMA_MAX_NODES * NUMA_MAX_CPUS_PER_NODE)

// NUMA distance normalization (local = 10)
#define NUMA_LOCAL_DISTANCE 10

/**
 * @brief NUMA node information
 */
typedef struct {
    uint32_t node_id;
    uint64_t memory_size;        // Total memory in bytes
    uint64_t free_memory;        // Free memory in bytes
    uint32_t cpu_count;          // Number of CPUs in this node
    uint32_t cpu_ids[NUMA_MAX_CPUS_PER_NODE];  // CPU IDs
    bool online;                 // Node is online
} numa_node_info_t;

/**
 * @brief NUMA topology structure
 */
typedef struct {
    uint32_t num_nodes;          // Number of NUMA nodes
    uint32_t num_cpus;           // Total number of CPUs
    
    // Node information
    numa_node_info_t nodes[NUMA_MAX_NODES];
    
    // Distance matrix (normalized, local = 10)
    uint32_t distance[NUMA_MAX_NODES][NUMA_MAX_NODES];
    
    // CPU to node mapping
    uint32_t cpu_to_node[NUMA_MAX_CPUS];
    
    // Statistics
    uint64_t total_memory;       // Total system memory
    uint64_t total_free_memory;  // Total free memory
    
    // Cached for fast lookup
    bool initialized;
} numa_topology_t;

/**
 * @brief Initialize NUMA topology
 * 
 * Detects NUMA nodes, CPUs, memory, and distances.
 * Parses /sys/devices/system/node/ for information.
 * 
 * @return Pointer to global topology, or NULL on failure
 */
numa_topology_t* numa_topology_init(void);

/**
 * @brief Get global NUMA topology
 * 
 * @return Pointer to global topology, or NULL if not initialized
 */
numa_topology_t* numa_topology_get(void);

/**
 * @brief Get NUMA node for CPU
 * 
 * @param cpu_id CPU ID
 * @return NUMA node ID, or -1 if invalid
 */
int numa_topology_cpu_to_node(uint32_t cpu_id);

/**
 * @brief Get distance between two NUMA nodes
 * 
 * @param from_node Source node
 * @param to_node Destination node
 * @return Distance (normalized, local = 10), or -1 if invalid
 */
int numa_topology_distance(uint32_t from_node, uint32_t to_node);

/**
 * @brief Get current CPU's NUMA node
 * 
 * @return NUMA node ID, or -1 on failure
 */
int numa_topology_current_node(void);

/**
 * @brief Get current CPU ID
 * 
 * @return CPU ID, or -1 on failure
 */
int numa_topology_current_cpu(void);

/**
 * @brief Check if NUMA is available
 * 
 * @return true if NUMA is available, false otherwise
 */
bool numa_topology_available(void);

/**
 * @brief Get node with most free memory
 * 
 * @return Node ID, or -1 if no nodes available
 */
int numa_topology_node_with_most_free_memory(void);

/**
 * @brief Get closest node to given node
 * 
 * @param node Node ID
 * @return Closest node ID (excluding self), or -1 if invalid
 */
int numa_topology_closest_node(uint32_t node);

/**
 * @brief Update node memory statistics
 * 
 * Re-reads memory information from sysfs.
 * 
 * @param node Node ID
 * @return 0 on success, -1 on failure
 */
int numa_topology_update_memory(uint32_t node);

/**
 * @brief Print NUMA topology
 * 
 * Prints topology information to stdout for debugging.
 */
void numa_topology_print(void);

/**
 * @brief Destroy NUMA topology
 * 
 * Cleans up resources. Topology must be re-initialized after this.
 */
void numa_topology_destroy(void);

#ifdef __cplusplus
}
#endif

#endif // PHASE2_NUMA_TOPOLOGY_H
