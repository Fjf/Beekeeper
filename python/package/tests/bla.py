import torch

from games.connect4.connect4 import Connect4
from games.connect4.connect4_nn import Connect4NN
from simulator import Simulator


def main():
    game = Connect4()
    network = Connect4NN(game.input_space, game.action_space)
    simulator = Simulator(Connect4, mcts_iterations=25)

    while game.winner() is None:
        policy = simulator.mcts_object.process(game, network)
        best_idx = torch.multinomial(policy, 1).item()

        print("\n\n")
        print("Parent initial", game.node.mcts.policy)
        print("Child value", [round(child.mcts.value, 2) for child in game.children()])
        print("Final policy", policy)
        print("")
        print("Best idx", best_idx)
        print([child.encode() for child in game.children()])
        for child in game.children():
            if best_idx == child.encode():
                game.select_child(child)
                break

        game.print()

    print(game.winner())
    exit(0)


if __name__ == "__main__":
    main()
