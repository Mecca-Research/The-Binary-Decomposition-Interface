
#ifndef BDI_OPTIMIZER_PASSES_FUNCTIONINLINING_HPP
#define BDI_OPTIMIZER_PASSES_FUNCTIONINLINING_HPP

#include "optimizer/engine/OptimizationPassBase.hpp"
#include <cstdint>

namespace bdi::optimizer {

/**
 * @brief Policy for function inlining decisions
 */
struct InlinePolicy {
    uint32_t max_inline_size = 100;      ///< Maximum function size to inline
    uint32_t max_inline_depth = 3;       ///< Maximum inlining depth
    bool inline_small_functions = true;  ///< Always inline very small functions
    uint32_t small_function_threshold = 10; ///< Size threshold for small functions
    
    InlinePolicy() = default;
};

/**
 * @brief Function inlining optimization pass
 * 
 * Replaces function calls with the function body when beneficial,
 * reducing call overhead and enabling further optimizations.
 */
class FunctionInlining : public OptimizationPassBase {
public:
    explicit FunctionInlining(const InlinePolicy& policy = InlinePolicy())
        : OptimizationPassBase("FunctionInlining"), policy_(policy) {}
    
    bool run(BDIGraph& graph) override;
    
    /**
     * @brief Check if a function should be inlined
     */
    bool shouldInline(NodeID call_node, const BDIGraph& graph);
    
    /**
     * @brief Inline a function call
     */
    void inlineFunction(NodeID call_node, BDIGraph& graph);
    
    /**
     * @brief Estimate the size of a function
     */
    uint32_t estimateFunctionSize(NodeID function_node, const BDIGraph& graph);
    
private:
    void visitNode(BDINode& node, BDIGraph& graph) override;
    
    InlinePolicy policy_;
    uint32_t current_depth_ = 0;
};

} // namespace bdi::optimizer

#endif // BDI_OPTIMIZER_PASSES_FUNCTIONINLINING_HPP
