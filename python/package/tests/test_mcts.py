import unittest
from collections import defaultdict

import numpy as np
import torch

from MCTS import MCTS
from games.connect4.connect4 import Connect4
from games.connect4.connect4_nn import Connect4NN
from games.hive.hive import Hive, lib, N_NODES
from games.hive.hive_nn import HiveNN
from games.tictactoe.tictactoe import TicTacToe
from games.tictactoe.tictactoe_nn import TicTacToeNN
from games.utils import GameState
from simulator import Simulator


class MyTestCase(unittest.TestCase):
    game = Connect4
    nn = Connect4NN

    def test_game_outcome(self):

        results = defaultdict(int)
        for i in range(1000):
            game = self.game()
            while game.finished() == GameState.UNDETERMINED:
                game.ai_move("random")

            results[game.finished()] += 1

        print(results)

    def test_something(self):
        # torch.manual_seed(134)

        game = self.game()
        network = self.nn(game.input_space, game.action_space)
        simulator = Simulator(self.game, mcts_iterations=15)

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

        # assert game.turn == game.turn_limit

    def test_encodings(self):
        import torch
        torch.manual_seed(1337)

        mcts = MCTS(iterations=1500)
        network = self.nn()
        game = self.game()
        for i, child in enumerate(game.node.children):
            print(f"For child {i}: encoding: {child.encode()}")

        assert N_NODES.value == 0, N_NODES.value

    # def test_model(self):
    #     import torch
    #     torch.manual_seed(100)
    #
    #     # Initialize model
    #     model = HiveNN(Connect4.input_space, Connect4.action_space)
    #
    #     dataset_storage = []
    #     for i in range(20):
    #         # Initialize dataset
    #         filename = f"example{i}.data"
    #         tensors, policy_vectors, outcomes = zip(*torch.load(filename))
    #         tensors, policy_vectors, outcomes = torch.stack(tensors), torch.stack(policy_vectors), torch.stack(outcomes)
    #         new_dataset = HiveDataset(tensors, policy_vectors, outcomes)
    #         dataset_storage.insert(0, new_dataset)
    #
    #         dataset_storage = dataset_storage[:5]
    #
    #         dataset = HiveDataset([], [], [])
    #         for dss in dataset_storage:
    #             dataset += dss
    #
    #         ratio = 0.8
    #         n_train = int(len(dataset) * ratio)
    #         sizes = [n_train, len(dataset) - n_train]
    #         train_dataset, test_dataset = random_split(dataset, sizes)
    #         train_dataloader = DataLoader(train_dataset, shuffle=True, num_workers=1, batch_size=16)
    #         test_dataloader = DataLoader(test_dataset, num_workers=10, batch_size=16)
    #
    #         trainer = Trainer(gpus=1, max_epochs=20)
    #         trainer.fit(model, train_dataloader)
    #         trainer.test(model, dataloaders=test_dataloader, verbose=True)


if __name__ == '__main__':
    unittest.main()
