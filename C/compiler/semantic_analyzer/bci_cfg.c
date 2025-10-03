
// ===================================================================
// DESC: Implementation of Control Flow Graph
// ===================================================================

#include "c23_compat.h"
#include "bci_cfg.h"
#include <stdlib.h>
#include <string.h>

// --- CFG Management ---

void cfg_init(ControlFlowGraph* cfg) {
    cfg->entry = nullptr;
    cfg->exit = nullptr;
    bci_vec_init(&cfg->nodes);
    cfg->next_id = 0;
}

void cfg_free(ControlFlowGraph* cfg) {
    if (!cfg) return;
    
    for (size_t i = 0; i < cfg->nodes.len; i++) {
        CfgNode* node = cfg->nodes.data[i];
        bci_vec_free(&node->predecessors);
        bci_vec_free(&node->successors);
        free(node);
    }
    bci_vec_free(&cfg->nodes);
}

// --- Node Creation ---

CfgNode* cfg_new_node(ControlFlowGraph* cfg, CfgNodeKind kind) {
    CfgNode* node = malloc(sizeof(CfgNode));
    if (!node) return nullptr;
    
    node->id = cfg->next_id++;
    node->kind = kind;
    node->ast_node = nullptr;
    bci_vec_init(&node->predecessors);
    bci_vec_init(&node->successors);
    node->visited = false;
    
    bci_vec_push(&cfg->nodes, node);
    return node;
}

void cfg_add_edge(CfgNode* from, CfgNode* to) {
    if (!from || !to) return;
    
    bci_vec_push(&from->successors, to);
    bci_vec_push(&to->predecessors, from);
}

// --- CFG Construction ---

void cfg_build_from_ast(ControlFlowGraph* cfg, AstNode* program) {
    if (!cfg || !program) return;
    
    // Create entry and exit nodes
    cfg->entry = cfg_new_node(cfg, CFG_NODE_ENTRY);
    cfg->exit = cfg_new_node(cfg, CFG_NODE_EXIT);
    
    // Build CFG from AST (simplified)
    CfgNode* current = cfg->entry;
    
    // Connect entry to first statement
    if (program->kind == AST_NODE_PROGRAM && program->as.block) {
        CfgNode* first = cfg_new_node(cfg, CFG_NODE_BASIC_BLOCK);
        cfg_add_edge(current, first);
        current = first;
    }
    
    // Connect last statement to exit
    cfg_add_edge(current, cfg->exit);
}

// --- Dominator Analysis ---

void cfg_compute_dominators(ControlFlowGraph* cfg) {
    if (!cfg) return;
    
    // Simplified dominator computation
    // Would use iterative algorithm to compute dominance frontier
}

bool cfg_is_reachable(CfgNode* from, CfgNode* to) {
    if (!from || !to) return false;
    if (from == to) return true;
    
    // Simple DFS reachability
    from->visited = true;
    
    for (size_t i = 0; i < from->successors.len; i++) {
        CfgNode* succ = from->successors.data[i];
        if (!succ->visited && cfg_is_reachable(succ, to)) {
            return true;
        }
    }
    
    return false;
}

CfgNode* cfg_find_common_dominator(CfgNode* a, CfgNode* b) {
    (void)a;
    (void)b;
    // Simplified: would compute immediate dominators
    return nullptr;
}
