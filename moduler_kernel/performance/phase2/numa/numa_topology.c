
/**
 * @file numa_topology.c
 * @brief NUMA topology detection implementation
 */

#define _GNU_SOURCE
#include "numa_topology.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sched.h>
#include <errno.h>

// Global topology instance
static numa_topology_t g_topology = {0};

/**
 * @brief Parse CPU list from sysfs
 * 
 * Parses strings like "0-3,8-11" into array of CPU IDs.
 */
static int parse_cpu_list(const char* str, uint32_t* cpus, uint32_t max_cpus) {
    int count = 0;
    const char* p = str;
    
    while (*p && count < max_cpus) {
        char* end;
        unsigned long start = strtoul(p, &end, 10);
        
        if (end == p) break;
        
        if (*end == '-') {
            // Range: start-end
            p = end + 1;
            unsigned long finish = strtoul(p, &end, 10);
            for (unsigned long i = start; i <= finish && count < max_cpus; i++) {
                cpus[count++] = (uint32_t)i;
            }
        } else {
            // Single CPU
            cpus[count++] = (uint32_t)start;
        }
        
        if (*end == ',') {
            p = end + 1;
        } else {
            break;
        }
    }
    
    return count;
}

/**
 * @brief Read sysfs file into buffer
 */
static int read_sysfs_file(const char* path, char* buf, size_t size) {
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    
    if (!fgets(buf, size, f)) {
        fclose(f);
        return -1;
    }
    
    fclose(f);
    
    // Remove trailing newline
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') {
        buf[len-1] = '\0';
    }
    
    return 0;
}

/**
 * @brief Detect NUMA nodes
 */
static int detect_numa_nodes(numa_topology_t* topo) {
    DIR* dir = opendir("/sys/devices/system/node");
    if (!dir) {
        // NUMA not available, assume single node
        topo->num_nodes = 1;
        topo->nodes[0].node_id = 0;
        topo->nodes[0].online = true;
        return 0;
    }
    
    struct dirent* entry;
    topo->num_nodes = 0;
    
    while ((entry = readdir(dir)) != NULL && topo->num_nodes < NUMA_MAX_NODES) {
        if (strncmp(entry->d_name, "node", 4) != 0) continue;
        
        uint32_t node_id = atoi(entry->d_name + 4);
        numa_node_info_t* node = &topo->nodes[topo->num_nodes];
        
        node->node_id = node_id;
        node->online = true;
        
        // Read CPU list
        char path[256];
        char buf[1024];
        snprintf(path, sizeof(path), "/sys/devices/system/node/node%u/cpulist", node_id);
        
        if (read_sysfs_file(path, buf, sizeof(buf)) == 0) {
            node->cpu_count = parse_cpu_list(buf, node->cpu_ids, NUMA_MAX_CPUS_PER_NODE);
            
            // Update CPU to node mapping
            for (uint32_t i = 0; i < node->cpu_count; i++) {
                uint32_t cpu_id = node->cpu_ids[i];
                if (cpu_id < NUMA_MAX_CPUS) {
                    topo->cpu_to_node[cpu_id] = node_id;
                    if (cpu_id >= topo->num_cpus) {
                        topo->num_cpus = cpu_id + 1;
                    }
                }
            }
        }
        
        // Read memory info
        snprintf(path, sizeof(path), "/sys/devices/system/node/node%u/meminfo", node_id);
        FILE* f = fopen(path, "r");
        if (f) {
            char line[256];
            while (fgets(line, sizeof(line), f)) {
                unsigned long kb;
                if (sscanf(line, "Node %*u MemTotal: %lu kB", &kb) == 1) {
                    node->memory_size = kb * 1024;
                    topo->total_memory += node->memory_size;
                } else if (sscanf(line, "Node %*u MemFree: %lu kB", &kb) == 1) {
                    node->free_memory = kb * 1024;
                    topo->total_free_memory += node->free_memory;
                }
            }
            fclose(f);
        }
        
        topo->num_nodes++;
    }
    
    closedir(dir);
    return 0;
}

/**
 * @brief Detect NUMA distances
 */
static int detect_numa_distances(numa_topology_t* topo) {
    // Initialize with default distances
    for (uint32_t i = 0; i < NUMA_MAX_NODES; i++) {
        for (uint32_t j = 0; j < NUMA_MAX_NODES; j++) {
            if (i == j) {
                topo->distance[i][j] = NUMA_LOCAL_DISTANCE;
            } else {
                topo->distance[i][j] = NUMA_LOCAL_DISTANCE * 2;  // Default remote distance
            }
        }
    }
    
    // Read actual distances from sysfs
    for (uint32_t i = 0; i < topo->num_nodes; i++) {
        uint32_t node_id = topo->nodes[i].node_id;
        char path[256];
        snprintf(path, sizeof(path), "/sys/devices/system/node/node%u/distance", node_id);
        
        FILE* f = fopen(path, "r");
        if (!f) continue;
        
        char line[1024];
        if (fgets(line, sizeof(line), f)) {
            char* p = line;
            for (uint32_t j = 0; j < topo->num_nodes; j++) {
                char* end;
                unsigned long dist = strtoul(p, &end, 10);
                if (end != p) {
                    uint32_t target_node = topo->nodes[j].node_id;
                    topo->distance[node_id][target_node] = (uint32_t)dist;
                    p = end;
                }
            }
        }
        
        fclose(f);
    }
    
    return 0;
}

numa_topology_t* numa_topology_init(void) {
    if (g_topology.initialized) {
        return &g_topology;
    }
    
    memset(&g_topology, 0, sizeof(g_topology));
    
    // Detect nodes and CPUs
    if (detect_numa_nodes(&g_topology) < 0) {
        return NULL;
    }
    
    // Detect distances
    if (detect_numa_distances(&g_topology) < 0) {
        return NULL;
    }
    
    g_topology.initialized = true;
    return &g_topology;
}

numa_topology_t* numa_topology_get(void) {
    return g_topology.initialized ? &g_topology : NULL;
}

int numa_topology_cpu_to_node(uint32_t cpu_id) {
    if (!g_topology.initialized || cpu_id >= NUMA_MAX_CPUS) {
        return -1;
    }
    return g_topology.cpu_to_node[cpu_id];
}

int numa_topology_distance(uint32_t from_node, uint32_t to_node) {
    if (!g_topology.initialized || 
        from_node >= NUMA_MAX_NODES || 
        to_node >= NUMA_MAX_NODES) {
        return -1;
    }
    return g_topology.distance[from_node][to_node];
}

int numa_topology_current_cpu(void) {
    return sched_getcpu();
}

int numa_topology_current_node(void) {
    int cpu = numa_topology_current_cpu();
    if (cpu < 0) return -1;
    return numa_topology_cpu_to_node(cpu);
}

bool numa_topology_available(void) {
    return g_topology.initialized && g_topology.num_nodes > 1;
}

int numa_topology_node_with_most_free_memory(void) {
    if (!g_topology.initialized || g_topology.num_nodes == 0) {
        return -1;
    }
    
    uint32_t best_node = 0;
    uint64_t max_free = 0;
    
    for (uint32_t i = 0; i < g_topology.num_nodes; i++) {
        if (g_topology.nodes[i].online && 
            g_topology.nodes[i].free_memory > max_free) {
            max_free = g_topology.nodes[i].free_memory;
            best_node = g_topology.nodes[i].node_id;
        }
    }
    
    return best_node;
}

int numa_topology_closest_node(uint32_t node) {
    if (!g_topology.initialized || node >= NUMA_MAX_NODES) {
        return -1;
    }
    
    uint32_t min_dist = UINT32_MAX;
    int closest = -1;
    
    for (uint32_t i = 0; i < g_topology.num_nodes; i++) {
        uint32_t target = g_topology.nodes[i].node_id;
        if (target == node) continue;
        
        uint32_t dist = g_topology.distance[node][target];
        if (dist < min_dist) {
            min_dist = dist;
            closest = target;
        }
    }
    
    return closest;
}

int numa_topology_update_memory(uint32_t node) {
    if (!g_topology.initialized || node >= g_topology.num_nodes) {
        return -1;
    }
    
    char path[256];
    snprintf(path, sizeof(path), "/sys/devices/system/node/node%u/meminfo", node);
    
    FILE* f = fopen(path, "r");
    if (!f) return -1;
    
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        unsigned long kb;
        if (sscanf(line, "Node %*u MemFree: %lu kB", &kb) == 1) {
            g_topology.nodes[node].free_memory = kb * 1024;
            break;
        }
    }
    
    fclose(f);
    return 0;
}

void numa_topology_print(void) {
    if (!g_topology.initialized) {
        printf("NUMA topology not initialized\n");
        return;
    }
    
    printf("NUMA Topology:\n");
    printf("  Nodes: %u\n", g_topology.num_nodes);
    printf("  CPUs: %u\n", g_topology.num_cpus);
    printf("  Total Memory: %lu MB\n", g_topology.total_memory / (1024*1024));
    printf("  Total Free: %lu MB\n", g_topology.total_free_memory / (1024*1024));
    printf("\n");
    
    for (uint32_t i = 0; i < g_topology.num_nodes; i++) {
        numa_node_info_t* node = &g_topology.nodes[i];
        printf("  Node %u:\n", node->node_id);
        printf("    CPUs: %u (", node->cpu_count);
        for (uint32_t j = 0; j < node->cpu_count; j++) {
            printf("%u%s", node->cpu_ids[j], j < node->cpu_count-1 ? "," : "");
        }
        printf(")\n");
        printf("    Memory: %lu MB\n", node->memory_size / (1024*1024));
        printf("    Free: %lu MB\n", node->free_memory / (1024*1024));
        printf("    Online: %s\n", node->online ? "yes" : "no");
    }
    printf("\n");
    
    printf("  Distance Matrix:\n");
    printf("       ");
    for (uint32_t i = 0; i < g_topology.num_nodes; i++) {
        printf("%5u", g_topology.nodes[i].node_id);
    }
    printf("\n");
    
    for (uint32_t i = 0; i < g_topology.num_nodes; i++) {
        printf("  %4u:", g_topology.nodes[i].node_id);
        for (uint32_t j = 0; j < g_topology.num_nodes; j++) {
            uint32_t from = g_topology.nodes[i].node_id;
            uint32_t to = g_topology.nodes[j].node_id;
            printf("%5u", g_topology.distance[from][to]);
        }
        printf("\n");
    }
}

void numa_topology_destroy(void) {
    memset(&g_topology, 0, sizeof(g_topology));
}
