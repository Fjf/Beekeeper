import unittest
from collections import defaultdict

from MCTS import MCTS
from games.hive.hive import Hive, lib, N_NODES
from games.hive.hive_nn import HiveNN
from games.utils import GameState
from simulator import Simulator


class MyTestCase(unittest.TestCase):
    game = Hive
    nn = HiveNN

    def test_simulator(self):
        results = defaultdict(int)
        for i in range(100):
            simulator = Simulator(self.game, mcts_iterations=10)

            p1 = self.nn(self.game.input_space, self.game.action_space)
            p2 = self.nn(self.game.input_space, self.game.action_space)
            game = self.game()

            result = simulator.play(game, p1, p2)[1]

            # if result == GameState.DRAW_REPETITION:
            #     for node in game.history:
            #         node.print()
            print(result)
            results[result] += 1
        print(results)


if __name__ == '__main__':
    unittest.main()
