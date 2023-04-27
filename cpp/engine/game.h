
#ifndef BEEKEEPER_GAME_H
#define BEEKEEPER_GAME_H


#include "tree.h"

class Game {
public:
    Node root = Node();

    Game();

    void random_move();
};

int generate_children(Node &root, double end_time);
void generate_moves(Node& node);

#endif //BEEKEEPER_GAME_H
