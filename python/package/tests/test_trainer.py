import os

from pytorch_lightning import Trainer
from torch.utils.data import DataLoader

from games.hive.hive import Hive
from games.hive.hive_dataset import HiveDataset
from games.hive.hive_nn import HiveNN


def test_trainer():
    import torch
    import numpy as np
    np.random.seed(1337)
    torch.manual_seed(1337)

    hive = Hive()
    aggregated_dataset = HiveDataset([], [], [])
    network = HiveNN()

    for i in range(10):
        hive.ai_move("random")

    board = hive.node.contents.to_np()
    board = torch.Tensor(board.reshape((1, *board.shape)))

    encodings = np.array([child.encode() for child in hive.children()], dtype=int)
    print(network(board)[0][0][encodings])

    for i in range(100):
        filename = f"example{i}.data"
        if not os.path.isfile(filename):
            break

        tensors, policy_vectors, outcomes = zip(*torch.load(filename))
        tensors = list(tensors)
        policy_vectors = list(policy_vectors)
        outcomes = list(outcomes)

        dataset = HiveDataset(tensors, policy_vectors, outcomes)
        aggregated_dataset += dataset

    print(sum(aggregated_dataset.outcomes) / len(aggregated_dataset.outcomes))
    print(len(aggregated_dataset))

    dataloader = DataLoader(aggregated_dataset, batch_size=16, shuffle=True)

    trainer = Trainer(gpus=0, max_epochs=1, default_root_dir="checkpoints")
    trainer.fit(network, dataloader)

    print(network(board)[0][0][encodings])


if __name__ == "__main__":
    test_trainer()
