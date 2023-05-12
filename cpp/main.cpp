
#include <game.h>
#include <iostream>
#include "tree_impl.cpp"
#include "ml/ai_mcts.h"

void run_random() {
    Game game = Game<BaseNode<DefaultData>>();


    for (int i = 0; i < 20; i++) {
        game.root.generate_children();
        game.random_move();
        game.root.print();
    }

    std::cout << "Ran random moves" << std::endl;
}

void run_mcts() {
    Game game = Game<BaseNode<MCTSData>>();

    ai_mcts::run_mcts(game.root);

    for (BaseNode<MCTSData>& child : game.root.children) {
        std::cout << " has value " << child.data.value / child.data.visitCount << " with visit count " << child.data.visitCount << std::endl;
    }
}

int main(int argc, char **argv) {
    run_mcts();

    return 0;
}
