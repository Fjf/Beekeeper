
#include <game.h>
#include <utils.h>
#include <limits>
#include <cmath>
#include <random>
#include <cassert>
#include "ai_mcts.h"

int node_encode_absolute(BaseNode<MCTSData> &node) {
    uint8_t idx = to_tile_index(node.move.tile);
    return idx;
}


int ai_mcts::naive_playout(BaseNode<MCTSData> *root) {
    BaseNode<MCTSData> node = *root;

    int depth = 0;
    while (true) {
        depth += 1;
        int finalState = node.board.finished();
        if (finalState > 0) {
            return node.board.finished();
        }

        int err = node.generate_children();
        if (err == ERR_NOMOVES) {
            throw std::runtime_error("No moves can be generated, but no final state was determined.");
        }

        int random_child = rand() % int(node.children.size());
        int i = 0;
        for (const BaseNode<MCTSData> &child : node.children) {
            if (i == random_child) {
                node = child;
                node.parent->children.clear();
                node.parent = nullptr;
                break;
            }
            i++;
        }
    }
}


torch::Tensor board_to_tensor(Board& board) {
    auto options = torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCUDA, 1);
    torch::Tensor tensor = torch::from_blob(board.tiles, {BOARD_SIZE, BOARD_SIZE}, options);
    return tensor;
}

float ai_mcts::model_playout(BaseNode<MCTSData> &root, BaseNode<MCTSData> &leaf, torch::jit::script::Module &model) {
    int finalState = root.board.finished();
    if (finalState > 0) {
        int game_value = root.board.finished();
        float value;
        if (game_value == LIGHT_WON) {
            value = 1;
        } else if (game_value == DARK_WON) {
            value = 0;
        } else {
            value = 0.5;
        }

        if (root.board.turn % 2 == 1) {
            value = 1 - value;
        }
        return value;
    }

    // Process forward pass of board input
    std::vector<torch::jit::IValue> inputs;
    inputs.emplace_back(board_to_tensor(root.board));
    auto outputs = model.forward(inputs).toTuple();
    torch::Tensor value = outputs->elements()[0].toTensor();

    return value.item<float>();
}

BaseNode<MCTSData> &ai_mcts::select_leaf(BaseNode<MCTSData> *root) {
    BaseNode<MCTSData> *parent = root;

    int depth = 0;
    while (!parent->children.empty()) {
        depth++;
        BaseNode<MCTSData> *best = nullptr;
        float best_value = -std::numeric_limits<float>::infinity();

        // Select best node to explore
        for (BaseNode<MCTSData> &child : parent->children) {
            // Unvisited nodes get priority.
            if (child.data.visitCount == 0) {
                best = &child;
                break;
            }
            // Player 2 has inverted value (good move for p1 is bad for p2)
            double exploration_value =
                    root->board.turn % 2 == 0 ? child.data.value : float(child.data.visitCount) - child.data.value;

            double MCTS_CONSTANT = 1.41; // Exploration/exploitation param.

            exploration_value = exploration_value / child.data.visitCount +
                                MCTS_CONSTANT * sqrt(log(double(parent->data.visitCount)) / child.data.visitCount);
            if (best_value < exploration_value) {
                best = &child;
                best_value = float(exploration_value);
            }
        }
        if (best == nullptr) {
            std::cout << "Invalid best" << std::endl;
            exit(1);
        }
        parent = best;
    }
    return *parent;
}

void ai_mcts::cascade_result(BaseNode<MCTSData> *leaf, float value) {
    BaseNode<MCTSData> *node = leaf;
    while (true) {
        if (node->board.turn % 2 == 1) {
            node->data.value += value;
        } else {
            node->data.value += 1 - value;
        }
        node->data.visitCount++;

        if (node->parent == nullptr) return;

        node = node->parent;
    }
}

void ai_mcts::run_mcts(BaseNode<MCTSData> &root) {
    int n_iters = 1000;

    root.generate_children();

    for (int i = 0; i < n_iters; i++) {
        BaseNode<MCTSData> &leaf = select_leaf(&root);
        int game_value = naive_playout(&leaf);

        float value;
        if (game_value == LIGHT_WON) {
            value = 1;
        } else if (game_value == DARK_WON) {
            value = 0;
        } else {
            value = 0.5;
        }

        if (root.board.turn % 2 == 1) {
            value = 1 - value;
        }

        cascade_result(&leaf, value);
    }
}

void ai_mcts::run_ai_mcts(BaseNode<MCTSData> &root, torch::jit::script::Module &model) {
    int n_iterations = 1000;

    root.generate_children();

    for (int i = 0; i < n_iterations; i++) {
        BaseNode<MCTSData> &leaf = select_leaf(&root);
        float value = model_playout(root, leaf, model);
        cascade_result(&leaf, value);
    }

    // Values are positive by definition, so a negative best_value initial value will ensure a selected child.
    float best_value = -1.0f;
    BaseNode<MCTSData>* best_child;
    for (BaseNode<MCTSData>& child : root.children) {
        if (child.data.value / child.data.visitCount > best_value) {
            best_child = &child;
            best_value = child.data.value / child.data.visitCount;
        }
    }


}


//torch::Tensor ai_mcts::evaluate(torch::jit::script::Module &model) {
//
//    Game game = Game<BaseNode<MCTSData>>();
//
//    // Process forward pass of board input
//    std::vector<torch::jit::IValue> inputs;
//    inputs.emplace_back(board_to_tensor(game.root.board));
//    auto outputs = model.forward(inputs).toTuple();
//    torch::Tensor value = outputs->elements()[0].toTensor();
//    torch::Tensor prediction = outputs->elements()[1].toTensor();
//
//    game.root.generate_moves();
//    for (BaseNode<MCTSData>& child : game.root.children) {
//        int node_idx = node_encode_absolute(child);
//        child.value = prediction[node_idx].item<float>();
//        child.visitCount++;
//    }
//}


//PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
//    m.def("evaluate", &ai_mcts::evaluate, "Get next move with MCTS");
//}