import unittest
from ctypes import pointer

from pytorch_lightning import Trainer
from torch.utils.data import DataLoader, random_split

from MCTS import MCTS
from games.connect4.connect4 import Connect4
from games.connect4.connect4_nn import Connect4NN
from games.hive.hive import Hive, lib
from games.hive.hive_dataset import HiveDataset
from games.hive.hive_nn import HiveNN
from games.utils import GameState


class MyTestCase(unittest.TestCase):
    # @staticmethod
    # def test_something():
    #     import torch
    #     torch.manual_seed(133)
    #
    #     mcts = MCTS(iterations=50)
    #     game = Connect4()
    #     network = Connect4NN(game.input_space, game.action_space)
    #
    #     while game.finished() == GameState.UNDETERMINED:
    #         arr = game.node.to_np(game.turn() % 2)
    #         arr = arr.reshape(1, *arr.shape)
    #         policy, v = network(torch.Tensor(arr))
    #         policy = policy[0]
    #
    #         pi = mcts.process(game, network)
    #
    #         # hive.print()
    #         for i, child in enumerate(game.node.get_children()):
    #             print(
    #                 f"Predict: {policy[child.encode()]}, "
    #                 f"Improved: {pi[child.encode()]} {child.mcts.n_sims}. "
    #                 f"For move {child.move}")
    #
    #         print(network.loss_v(torch.Tensor([1]), v.reshape(1, *v.shape)),
    #               -network.loss_pi(pi.reshape(1, *pi.shape), policy.reshape(1, *policy.shape)))
    #
    #         encodings = [child.encode() for child in game.node.get_children()]
    #         selection = torch.multinomial(pi[encodings], 1).item()
    #         print(f"Selected {selection}")
    #         game.select_child(game.node.get_children()[selection])
    #         game.print()
    #
    #     assert (True)
    #
    # @staticmethod
    # def test_encodings():
    #     import torch
    #     torch.manual_seed(1337)
    #
    #     mcts = MCTS(iterations=1500)
    #     network = Connect4NN()
    #     game = Connect4()
    #
    #     for i, child in enumerate(game.node.get_children()):
    #         print(f"For child {i}: encoding: {child.encode()}")

    def test_model(self):
        import torch
        torch.manual_seed(100)

        # Initialize model
        model = Connect4NN(Connect4.input_space, Connect4.action_space)

        dataset_storage = []
        for i in range(20):
            # Initialize dataset
            filename = f"example{i}.data"
            tensors, policy_vectors, outcomes = zip(*torch.load(filename))
            tensors, policy_vectors, outcomes = list(tensors), list(policy_vectors), list(outcomes)
            new_dataset = HiveDataset(tensors, policy_vectors, outcomes)
            dataset_storage.insert(0, new_dataset)

            dataset_storage = dataset_storage[:5]

            dataset = HiveDataset([], [], [])
            for dss in dataset_storage:
                dataset += dss



            ratio = 0.8
            n_train = int(len(dataset) * ratio)
            sizes = [n_train, len(dataset) - n_train]
            train_dataset, test_dataset = random_split(dataset, sizes)
            train_dataloader = DataLoader(train_dataset, shuffle=True, num_workers=1, batch_size=16)
            test_dataloader = DataLoader(test_dataset, num_workers=10, batch_size=16)

            trainer = Trainer(gpus=1, max_epochs=20)
            trainer.fit(model, train_dataloader)
            trainer.test(model, dataloaders=test_dataloader, verbose=True)


if __name__ == '__main__':
    unittest.main()
