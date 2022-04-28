import random

import torch
import os.path

from games.connect4.connect4_nn import Connect4NN
from games.hive.hive import Hive
from games.hive.hive_nn import HiveNN
from games.tictactoe.tictactoe import TicTacToe
from games.tictactoe.tictactoe_nn import TicTacToeNN
from simulator import Simulator
from games.connect4.connect4 import Connect4, Connect4Node
from games.utils import GameState

# NN = Connect4NN
# Game = Connect4
# dir = "C4_models"
NN = HiveNN
Game = Hive
dir = "Hive_models"


def play(model, side=0, show=False):
    game = Game()

    model = model.to("cuda:1")

    simulator = Simulator(Connect4)
    simulator.temperature_threshold = 0
    while game.finished() == GameState.UNDETERMINED:
        if game.turn() % 2 == side or side == 2:
            simulator.nn_move(model, game, mcts=False)
        else:
            children = game.children()
            game.select_child(random.choice(list(children)))

        if show:
            game.print()

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
    for i in range(7, 100):
        filename = f"{dir}/iteration_{i}.pt"
        if not os.path.exists(filename):
            break

        print(f"Loading {filename}")
        model = NN(Game.input_space, Game.action_space)
        model.load_state_dict(torch.load(filename))
        model.eval()

        # outcomes = [play(model, side=2) for i in range(200)]
        # print(f"Outcomes: {[outcomes.count(result) for result in [0, 0.5, 1]]}")
        outcomes = [play(model, side=i % 2) for i in range(200)]
        print(f"Outcomes: {[outcomes.count(result) for result in [0, 0.5, 1]]}")
        print(f"{filename} winrate: {sum(outcomes) / len(outcomes)}")

        # play(model, 2, True)


if __name__ == "__main__":
    main()
