import random

import torch
import os.path

from simulator import Simulator
from games.connect4.connect4 import Connect4
from games.utils import GameState


def play(model, side=0):
    game = Connect4()

    simulator = Simulator(game)
    while game.finished() == GameState.UNDETERMINED:
        if game.turn() % 2 == side:
            simulator.nn_move(model, game, mcts=False)
        else:
            children = game.children()
            game.select_child(random.choice(children))

    if game.finished() == GameState.WHITE_WON:
        result = 1
    elif game.finished() == GameState.BLACK_WON:
        result = 0
    else:
        result = 0.5
    if side == 1:
        result = 1 - result

    return result


def main():
    for i in range(100):
        filename = f"iteration_{i}.pt"
        if not os.path.exists(filename):
            break

        model = torch.load(filename)
        outcomes = [play(model, side=i % 2) for i in range(100)]

        print(f"{filename} winrate: {sum(outcomes)/len(outcomes)}")


if __name__ == "__main__":
    main()
