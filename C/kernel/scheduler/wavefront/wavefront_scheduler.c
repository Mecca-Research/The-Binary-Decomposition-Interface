
// ===================================================================
// Phase 5.3: Wavefront Scheduler Implementation
// ===================================================================
#include "wavefront_scheduler.h"
#include <stdlib.h>
#include <string.h>
#include <threads.h>

// --- Wavefront Utilities ---

Wavefront* wavefront_create(uint32_t id) {
    Wavefront* wf = malloc(sizeof(Wavefront));
    if (!wf) return NULL;
    
    wf->ready_capacity = 16;
    wf->ready_nodes = malloc(sizeof(NodeId) * wf->ready_capacity);
    if (!wf->ready_nodes) {
        free(wf);
        return NULL;
    }
    
    wf->ready_count = 0;
    atomic_init(&wf->completed_count, 0);
    wf->wavefront_id = id;
    
    return wf;
}

void wavefront_free(Wavefront* wf) {
    if (!wf) return;
    free(wf->ready_nodes);
    free(wf);
}

int wavefront_add_node(Wavefront* wf, NodeId node_id) {
    if (!wf) return -1;
    
    if (wf->ready_count >= wf->ready_capacity) {
        wf->ready_capacity *= 2;
        wf->ready_nodes = realloc(wf->ready_nodes, sizeof(NodeId) * wf->ready_capacity);
        if (!wf->ready_nodes) return -1;
    }
    
    wf->ready_nodes[wf->ready_count++] = node_id;
    return 0;
}

bool wavefront_is_complete(const Wavefront* wf) {
    if (!wf) return true;
    return atomic_load(&wf->completed_count) >= wf->ready_count;
}

// --- Wavefront Scheduler ---

WavefrontScheduler* wavefront_scheduler_create(BdiGraph* graph, DeviceVTable** devices, size_t device_count) {
    if (!graph || !devices) return NULL;
    
    WavefrontScheduler* sched = malloc(sizeof(WavefrontScheduler));
    if (!sched) return NULL;
    
    sched->graph = graph;
    sched->devices = devices;
    sched->device_count = device_count;
    sched->wavefront_capacity = 32;
    sched->wavefront_count = 0;
    sched->wavefronts = malloc(sizeof(Wavefront) * sched->wavefront_capacity);
    
    if (!sched->wavefronts) {
        free(sched);
        return NULL;
    }
    
    sched->visited = calloc(graph->node_count, sizeof(bool));
    if (!sched->visited) {
        free(sched->wavefronts);
        free(sched);
        return NULL;
    }
    
    atomic_init(&sched->running, false);
    
    return sched;
}

void wavefront_scheduler_free(WavefrontScheduler* sched) {
    if (!sched) return;
    
    for (size_t i = 0; i < sched->wavefront_count; i++) {
        wavefront_free(&sched->wavefronts[i]);
    }
    
    free(sched->wavefronts);
    free(sched->visited);
    free(sched);
}

int scheduler_get_next_wavefront(WavefrontScheduler* sched, Wavefront** out_wavefront) {
    if (!sched || !out_wavefront) return -1;
    
    // Build dependency levels
    
    Wavefront* wf = wavefront_create((uint32_t)sched->wavefront_count);
    if (!wf) {
        return -1;
    }
    
    // Find nodes with no unvisited dependencies
    for (size_t i = 0; i < sched->graph->node_count; i++) {
        if (sched->visited[i]) continue;
        
        const GraphNode* node = &sched->graph->nodes[i];
        bool all_deps_ready = true;
        
        for (size_t j = 0; j < node->input_count; j++) {
            NodeId input_id = node->inputs[j];
            if (input_id < sched->graph->node_count && !sched->visited[input_id]) {
                all_deps_ready = false;
                break;
            }
        }
        
        if (all_deps_ready) {
            wavefront_add_node(wf, node->id);
            sched->visited[i] = true;
        }
    }
    
    
    if (wf->ready_count == 0) {
        wavefront_free(wf);
        return -1;  // No more wavefronts
    }
    
    *out_wavefront = wf;
    return 0;
}

int scheduler_execute_wavefront(WavefrontScheduler* sched, Wavefront* wavefront) {
    if (!sched || !wavefront) return -1;
    
    // Execute all nodes in the wavefront in parallel
    for (size_t i = 0; i < wavefront->ready_count; i++) {
        NodeId node_id = wavefront->ready_nodes[i];
        if (node_id >= sched->graph->node_count) continue;
        
        const GraphNode* node = &sched->graph->nodes[node_id];
        
        // Select device (simple round-robin for now)
        DeviceVTable* device = sched->devices[i % sched->device_count];
        
        // Lower and execute
        void* kernel = NULL;
        if (device->lower(node, &kernel) == 0) {
            // TODO: Prepare HAM regions for execution
            device->enqueue(kernel, NULL, 0);
            free(kernel);
        }
        
        atomic_fetch_add(&wavefront->completed_count, 1);
    }
    
    // Sync all devices
    for (size_t i = 0; i < sched->device_count; i++) {
        sched->devices[i]->sync();
    }
    
    return 0;
}

int wavefront_scheduler_run(WavefrontScheduler* sched) {
    if (!sched) return -1;
    
    atomic_store(&sched->running, true);
    
    while (atomic_load(&sched->running)) {
        Wavefront* wf = NULL;
        if (scheduler_get_next_wavefront(sched, &wf) != 0) {
            break;  // No more wavefronts
        }
        
        scheduler_execute_wavefront(sched, wf);
        wavefront_free(wf);
    }
    
    atomic_store(&sched->running, false);
    return 0;
}
