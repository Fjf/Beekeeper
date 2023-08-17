from games.connect4.connect4 import Connect4
from games.connect4.connect4_nn import Connect4NN
from games.utils import GameState
from simulator import Simulator

def main():
    game = Connect4()
    network = Connect4NN(game.input_space, game.action_space)
    simulator = Simulator(Connect4, mcts_iterations=25)

    while game.finished() == GameState.UNDETERMINED:
        policy = simulator.nn_move(network, game, mcts=True)

        print("\n\n")
        game.print()
        if game.finished() != GameState.UNDETERMINED:
            break
        print(game.node.mcts.policy)
        print(policy)
        print("")

    print(game.finished())
    exit(0)

if __name__ == "__main__":
    main()