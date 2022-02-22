import pytorch_lightning as pl
import torch
from pytorch_lightning.utilities.types import STEP_OUTPUT
from torch import nn


class HiveNN(pl.LightningModule):
    def __init__(self, input_size=676, output_size=26*26*11):
        super().__init__()
        self.encoder = nn.Sequential(
            nn.Conv2d(2, 4, (3, 3), padding=1),
            nn.ReLU(),
            nn.MaxPool2d(2),

            nn.Conv2d(4, 4, (3, 3), padding=2),
            nn.ReLU(),
            nn.MaxPool2d(2),

            nn.Conv2d(4, 8, (3, 3), padding=2),
            nn.ReLU(),
            nn.MaxPool2d(2),

            nn.Conv2d(8, 8, (2, 2), padding=1),
            nn.ReLU(),

            nn.Conv2d(8, 16, (2, 2), padding=1),
            nn.ReLU(),

            nn.Flatten(),
            nn.Linear(576, output_size + 1),
            nn.Sigmoid(),
        )
        self.output_size = output_size
        self.input_size = input_size

        self.policy_loss = nn.CrossEntropyLoss()
        self.value_loss = nn.MSELoss()

    def training_step(self, batch, batch_idx) -> STEP_OUTPUT:
        x, pi, game_value = batch

        policy, value = self(x)

        # value_error = (game_value - value) ** 2
        # policy_error = torch.sum((-pi * (1e-8 + policy).log()), 1)
        # return (value_error.view(-1) + policy_error).mean()

        value_loss = self.loss_v(game_value, value)
        return value_loss.mean()

        policy_loss = self.loss_pi(pi, policy)

        return (value_loss + policy_loss).mean()

    @staticmethod
    def loss_pi(targets, outputs):
        return -torch.sum(targets * torch.log(outputs), 1)

    @staticmethod
    def loss_v(targets, outputs):
        return (targets - outputs) ** 2

    def forward(self, x):
        # in lightning, forward defines the prediction/inference actions
        embedding = self.encoder(x)
        segments = torch.tensor_split(embedding, (self.output_size,), dim=1)
        policy, value = segments[0], segments[1]

        return policy, value

    def configure_optimizers(self):
        return torch.optim.SGD(self.parameters(), lr=0.0005)
        # return torch.optim.Adam(self.parameters(), lr=0.02)
