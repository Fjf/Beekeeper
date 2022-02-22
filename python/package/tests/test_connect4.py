import os

from games.connect4.connect4 import Connect4
from games.utils import GameState


def test_trainer():
    game = Connect4()
    while game.turn() != game.turn_limit:
        children = list(game.children())
        game.select_child(children[0])
        game.print()
        if game.finished() != GameState.UNDETERMINED:
            print(game.finished())
            break


if __name__ == "__main__":
    test_trainer()
