
// ===================================================================
// DESC: Control Flow Graph for Phase 3.3
//       Builds CFG for control flow analysis and optimization
// ===================================================================
#ifndef BCI_CFG_H
#define BCI_CFG_H

#include "c23_compat.h"
#include "../ast/bci_ast_extended.h"

// --- CFG Node ---
typedef struct CfgNode CfgNode;

typedef enum {
    CFG_NODE_ENTRY,
    CFG_NODE_EXIT,
    CFG_NODE_BASIC_BLOCK,
    CFG_NODE_BRANCH,
    CFG_NODE_LOOP
} CfgNodeKind;

struct CfgNode {
    int id;
    CfgNodeKind kind;
    AstNode* ast_node;
    BciVec(CfgNode*) predecessors;
    BciVec(CfgNode*) successors;
    bool visited;
};

// --- CFG ---
typedef struct {
    CfgNode* entry;
    CfgNode* exit;
    BciVec(CfgNode*) nodes;
    int next_id;
} ControlFlowGraph;

// --- CFG API ---

void cfg_init(ControlFlowGraph* cfg);
void cfg_free(ControlFlowGraph* cfg);

[[nodiscard]] CfgNode* cfg_new_node(ControlFlowGraph* cfg, CfgNodeKind kind);
void cfg_add_edge(CfgNode* from, CfgNode* to);

void cfg_build_from_ast(ControlFlowGraph* cfg, AstNode* program);
void cfg_compute_dominators(ControlFlowGraph* cfg);

[[nodiscard]] bool cfg_is_reachable(CfgNode* from, CfgNode* to);
[[nodiscard]] CfgNode* cfg_find_common_dominator(CfgNode* a, CfgNode* b);

// Compile-time invariants
static_assert(sizeof(void*) >= 4, "CFG requires at least 32-bit pointers");

#endif // BCI_CFG_H
