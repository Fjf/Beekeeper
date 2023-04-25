
#include <game.h>
#include <iostream>


int main(int argc, char **argv) {

    Game game = Game();


    for (int i = 0; i < 20; i++) {
        generate_children(game.root, 1e100);
        game.root = game.root.children[0];
        game.root.print();
    }


    std::cout << "Ran something" << std::endl;
    return 0;
}
