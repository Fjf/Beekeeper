
#include <game.h>
#include <iostream>


int main(int argc, char **argv) {

    Game game = Game();


    for (int i = 0; i < 20; i++) {
        generate_children(game.root);
        game.random_move();
        game.root.print();
    }


    std::cout << "Ran something" << std::endl;
    return 0;
}
