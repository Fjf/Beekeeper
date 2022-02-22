import unittest
from ctypes import pointer

from MCTS import MCTS
from games.connect4.connect4 import Connect4
from games.connect4.connect4_nn import Connect4NN
from games.hive.hive import Hive, lib
from games.hive.hive_nn import HiveNN
from games.utils import GameState


class MyTestCase(unittest.TestCase):
    def test_something(self):
        import torch
        # torch.manual_seed(133)

        mcts = MCTS(iterations=200)
        game = Connect4()
        network = Connect4NN(game.input_space, game.action_space)

        while game.finished() == GameState.UNDETERMINED:
            arr = game.node.to_np(game.turn() % 2)
            arr = arr.reshape(1, *arr.shape)
            policy, v = network(torch.Tensor(arr))
            policy = policy[0]

            pi = mcts.process(game, policy, network)

            # hive.print()
            for i, child in enumerate(game.node.get_children()):
                print(
                    f"Predict: {policy[child.encode()]}, "
                    f"Improved: {pi[child.encode()]} {child.mcts['n_sims']}. "
                    f"For move {child.move}")

            print(network.loss_v(torch.Tensor([1]), v.reshape(1, *v.shape)),
                  network.loss_pi(pi.reshape(1, *pi.shape), policy.reshape(1, *policy.shape)))

            encodings = [child.encode() for child in game.node.get_children()]
            if 1 in pi:
                idx = (pi == 1).nonzero(as_tuple=True)[0]
                game.select_child(game.node.get_children()[encodings[idx]])
            else:
                selection = torch.multinomial(pi[encodings], 1).item()
                game.select_child(game.node.get_children()[selection])
            game.print()

        assert (True)

    def test_encodings(self):
        import torch
        torch.manual_seed(1337)

        mcts = MCTS(iterations=1500)
        network = Connect4NN()
        game = Connect4()

        for i, child in enumerate(game.node.get_children()):
            print(child.move)
            print(f"For child {i}: encoding: {child.encode()}")


if __name__ == '__main__':
    unittest.main()
