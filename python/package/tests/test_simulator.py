import unittest
from collections import defaultdict

from games.hive.hive import Hive
from games.hive.hive_nn import HiveNN
from games.tictactoe.tictactoe import TicTacToe
from games.tictactoe.tictactoe_nn import TicTacToeNN
from simulator import Simulator


class MyTestCase(unittest.TestCase):
    game = TicTacToe
    nn = TicTacToeNN

    def test_simulator(self):

        p1 = self.nn(self.game.input_space, self.game.action_space)
        p2 = self.nn(self.game.input_space, self.game.action_space)

        for i in range(1):
            simulator = Simulator(self.game, mcts_iterations=25)
            data = simulator.parallel_play(1, p1, p2)

            boards, policys, result, perspective = data

            for board, policy in zip(boards, policys):
                print(board[0] + board[1] * 2)
                print(policy)
                print(result)
                print("\n\n")

            # if result == GameState.DRAW_REPETITION:
            #     for node in game.history:
            #         node.print()


if __name__ == '__main__':
    unittest.main()
