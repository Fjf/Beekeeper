
#ifndef BEEKEEPER_AI_MCTS_H
#define BEEKEEPER_AI_MCTS_H

#include <torch/extension.h>
#include <tree.h>

class ai_mcts {
public:

    static torch::Tensor evaluate(torch::jit::script::Module &model);
};

#pragma pack(push, 1)
class alignas(8) MCTSNode : public BaseNode<MCTSNode> {
    float value = 0.0f;
    int visitCount = 0;

    [[nodiscard]] inline BaseNode copy() const {
        MCTSNode node;
        const size_t n_bytes = ((sizeof(Move) + sizeof(Board) + 8) / 8) * 8;
        memcpy((void*) &node, this, n_bytes);
        return node;
    }
};
#pragma pack(pop)


#endif //BEEKEEPER_AI_MCTS_H
