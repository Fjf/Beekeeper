
#ifndef BEEKEEPER_AI_MCTS_H
#define BEEKEEPER_AI_MCTS_H

#include <torch/extension.h>
#include <tree_impl.cpp>


class ai_mcts {
public:

//    static torch::Tensor evaluate(torch::jit::script::Module &model);

    static int naive_playout(BaseNode<MCTSData> *root);

    [[nodiscard]] static BaseNode<MCTSData>& select_leaf(BaseNode<MCTSData> *root);

    static void cascade_result(BaseNode<MCTSData> *leaf, float value);

    static void run_mcts(BaseNode<MCTSData> &root);

    static void run_ai_mcts(BaseNode<MCTSData> &root, torch::jit::script::Module &model);

    static float model_playout(BaseNode<MCTSData>& root, BaseNode<MCTSData>& leaf, torch::jit::script::Module &model);
};


#endif //BEEKEEPER_AI_MCTS_H
