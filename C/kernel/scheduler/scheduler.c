// ===================================================================
// DESC: Implements the scheduler with its secure policy gate.
// ===================================================================
#include "c23_compat.h"
#include "scheduler.h"
#include <stdlib.h>
#include <stdio.h>

// Forward declaration for the hash function
void aeon_hash_meta(const GraphNode* node, uint8_t out_hash[32]);

// --- Scheduler API Implementation ---
[[nodiscard]] 
Scheduler* aeon_scheduler_create(BdiGraph* g, DeviceVTable** devices, size_t dev_count) {
    Scheduler* sched = (Scheduler*)calloc(1, sizeof(Scheduler));
    if (!sched) return nullptr;
    sched->graph = g;
    sched->devices = devices;
    sched->device_count = dev_count;
    // Default policy is insecure mode
    sched->policy = (SecurityPolicy){ .secure_mode = false, .required_proof_class = PROOF_CLASS_NONE };
    return sched;
}

void aeon_scheduler_free(Scheduler* sched) {
    if (!sched) return;
    free(sched->ready_set);
    free(sched);
}

void aeon_scheduler_set_policy(Scheduler* sched, SecurityPolicy policy) {
    if (!sched) return;
    sched->policy = policy;
}

// The core logic of M4: The Policy Gate.
static bool policy_gate_check(Scheduler* sched, GraphNode* node) {
    // If not in secure mode, all checks pass.
    if (!sched->policy.secure_mode) {
        return true;
    }

    // In secure mode, the node must have attached metadata.
    if (node->meta_off == 0 && sched->graph->meta_size > 0) { // meta_off is 0-based index
         // Special case: if meta_off is 0 but points to valid meta, it's okay.
         // A better system would use a sentinel value like SIZE_MAX.
         // For now, we assume meta_off > 0 is required for attached meta.
    } else if (node->meta_off > sched->graph->meta_size) {
        printf("POLICY_GATE: REJECT Node %llu - No metadata in secure mode.\n", (unsigned long long)node->id);
        return false;
    }

    NodeMeta* meta = (NodeMeta*)(sched->graph->meta_arena + node->meta_off);

    // Check 1: The node must meet the minimum required proof class.
    if ((meta->proof_class & sched->policy.required_proof_class) != sched->policy.required_proof_class) {
        printf("POLICY_GATE: REJECT Node %llu - Does not meet required proof class.\n", (unsigned long long)node->id);
        return false;
    }

    // Check 2: The metadata hash must be valid (Merkle-hashing).
    uint8_t computed_hash[32];
    aeon_hash_meta(node, computed_hash);
    if (memcmp(meta->hash, computed_hash, 32) != 0) {
        printf("POLICY_GATE: REJECT Node %llu - Metadata hash mismatch! (Tampering detected)\n", (unsigned long long)node->id);
        return false;
    }

    printf("POLICY_GATE: PASS Node %llu.\n", (unsigned long long)node->id);
    return true; // All checks passed.
}

int aeon_scheduler_run_wave(Scheduler* sched) {
    if (!sched) return -1;
    BdiGraph* g = sched->graph;

    // 1. Find ready nodes (simple version: all nodes with no inputs).
    // A real scheduler would track dependencies.
    for (size_t i = 0; i < g->node_count; i++) {
        if (g->nodes[i].input_count == 0) {
            // In a real scheduler, we would manage the ready_set array.
            GraphNode* node = &g->nodes[i];
            
            // 2. Apply the Policy Gate.
            if (policy_gate_check(sched, node)) {
                // 3. Dispatch the node if it passes.
                DeviceId hint = node->device_hint > 0 ? node->device_hint : DEVICE_ID_CPU;
                DeviceVTable* device = sched->devices[hint];
                if (device) {
                    printf("SCHEDULER: Dispatching Node %llu to Device '%s'.\n", (unsigned long long)node->id, device->name);
                    void* kernel;
                    device->lower(node, &kernel);
                    device->enqueue(kernel, nullptr, 0);
                }
            }
        }
    }
    
    // 4. Sync devices.
    for (size_t i = 1; i <= sched->device_count; i++) {
        if (sched->devices[i]) sched->devices[i]->sync();
    }
    
    return 0;
}
