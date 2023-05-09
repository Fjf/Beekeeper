
#include <game.h>
#include "ai_mcts.h"


torch::Tensor ai_mcts::evaluate(torch::jit::script::Module &model) {
    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(torch::ones({1, 1, 1, 1}));

    model.forward(inputs);

    Game game = Game<MCTSNode>();
    game.root.generate_moves();

    for (MCTSNode& node : game.root.children) {

    }
}


PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
    m.def("evaluate", &ai_mcts::evaluate, "Get next move with MCTS");
}