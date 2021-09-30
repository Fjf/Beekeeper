import pytorch_lightning as pl
import torch
from pytorch_lightning.utilities.types import STEP_OUTPUT
from torch import nn


def hive_loss(output, target):
    loss = torch.mean((output - target) ** 2)
    return loss


class HiveNN(pl.LightningModule):
    def __init__(self, input_size, output_size):
        super().__init__()
        self.encoder = nn.Sequential(
            nn.Conv2d(5, 16, (3, 3), padding=1),
            nn.ReLU(),
            nn.MaxPool2d(2),

            nn.Conv2d(16, 16, (3, 3), padding=2),
            nn.ReLU(),
            nn.MaxPool2d(2),

            nn.Conv2d(16, 16, (3, 3), padding=2),
            nn.ReLU(),
            nn.MaxPool2d(2),

            nn.Conv2d(16, 16, (2, 2), padding=1),
            nn.ReLU(),

            nn.Conv2d(16, 16, (2, 2), padding=1),
            nn.ReLU(),

            nn.Flatten(),
            nn.Linear(576, output_size + 1),
            nn.Sigmoid(),
        )
        self.output_size = output_size
        self.input_size = input_size

    def training_step(self, batch, batch_idx) -> STEP_OUTPUT:
        x, expected = batch
        policy, value = self(x)
        loss = hive_loss(policy, expected)
        return loss

    def forward(self, x):
        # in lightning, forward defines the prediction/inference actions
        embedding = self.encoder(x)
        out = embedding.reshape((self.output_size + 1, -1))
        segments = torch.tensor_split(out, (self.output_size,))
        policy, value = segments[0], segments[1]
        return policy, value

    def configure_optimizers(self):
        return torch.optim.SGD(self.parameters(), lr=0.009, momentum=0.9)
        # return torch.optim.Adam(self.parameters(), lr=0.02)
