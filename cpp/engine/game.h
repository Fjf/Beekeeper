
#ifndef BEEKEEPER_GAME_H
#define BEEKEEPER_GAME_H

#include <csignal>

#include "tree.h"
#include "tt.h"


template<typename T>
class Game {
public:
    static_assert(std::is_base_of<BaseNode<T>, T>::value, "Instantiated class not derived from BaseNode.");
    T root = T();

    Game() {
        unsigned long seed;
        // Initialize zobrist hashing table
        if (zobrist_table == nullptr)
            zobrist_init();
        if (tt_table == nullptr) {
            // Initialize transposition table (set flag to -1 to know if its empty)
            tt_init();
            seed = mix(clock(), time(nullptr), getpid());
            // Randomized seed
            srand(seed);
        }

        root.board.initialize();
    }

    void random_move() {
        size_t selection = std::rand() % root.children.size();
        for (BaseNode<T> &child : root.children) {
            if (selection == 0) {
                root = child;
                root.parent = nullptr;
                return;
            }
            selection -= 1;
        }
    }
};


#endif //BEEKEEPER_GAME_H
