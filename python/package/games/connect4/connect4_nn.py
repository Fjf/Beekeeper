from typing import Optional

import pytorch_lightning as pl
import torch
from pytorch_lightning.utilities.types import STEP_OUTPUT
from torch import nn

from games.connect4.connect4 import Connect4
from games.utils import ResNetBlock


class Connect4NN(pl.LightningModule):
    def __init__(self, input_size=Connect4.input_space, output_size=Connect4.action_space):
        super().__init__()
        self.encoder = nn.Sequential(
            ResNetBlock(2, 2),
            ResNetBlock(2, 2),
            ResNetBlock(2, 2),
            ResNetBlock(2, 2),

            nn.Flatten(),
            nn.Linear(70, output_size + 1)
        )
        # self.encoder = nn.Sequential(
        #     nn.Flatten(),
        #     nn.Linear(36, 64),
        #     nn.ReLU(),
        #     nn.Linear(64, 64),
        #     nn.ReLU(),
        #     nn.Linear(64, 32),
        #     nn.Dropout(0.3),
        #     nn.ReLU(),
        #     nn.Linear(32, output_size + 1)
        # )

        self.policy_activation = nn.Softmax(dim=1)
        self.value_activation = nn.Tanh()

        # self.value_loss = nn.MSELoss()
        # self.policy_loss = nn.CrossEntropyLoss()

        self.output_size = output_size
        self.input_size = input_size

        # self.automatic_optimization = False

    def training_step(self, batch, batch_idx) -> STEP_OUTPUT:
        x, pi, game_value = batch

        # Increase class differences
        pi = torch.pow(pi, 4)
        pi /= torch.sum(pi, dim=1).view(pi.shape[0], 1)

        policy, value = self(x)

        l2_lambda = 0.001
        l2_norm = sum(p.pow(2.0).sum() for p in self.encoder.parameters())
        l2_regularization = l2_norm * l2_lambda

        value_loss = self.loss_v(game_value, value)
        policy_loss = self.loss_pi(pi, policy)

        loss = (value_loss - policy_loss).mean() + l2_regularization
        return loss

    def test_step(self, batch, batch_idx) -> Optional[STEP_OUTPUT]:
        x, pi, game_value = batch
        policy, value = self(x)

        value_loss = self.loss_v(game_value, value)
        policy_loss = self.loss_pi(pi, policy)

        loss = (value_loss - policy_loss).mean()
        self.log("policy_loss", -policy_loss)
        self.log("value_loss", value_loss)
        self.log("test_loss", loss)

    @staticmethod
    def loss_pi(targets: torch.Tensor, outputs):
        # return torch.sum(targets * torch.log(outputs)) / targets.size()[0]
        shape = targets.shape
        result = torch.bmm(targets.view(shape[0], 1, shape[1]), torch.log(outputs).view(shape[0], shape[1], 1))
        return result.flatten()

    @staticmethod
    def loss_v(targets, outputs):
        # return torch.sum((targets - outputs.view) ** 2) / targets.size()[0]
        return ((targets - outputs) ** 2).flatten()

    def forward(self, x):
        # in lightning, forward defines the prediction/inference actions
        embedding = self.encoder(x)
        segments = torch.tensor_split(embedding, (self.output_size,), dim=1)
        # policy = embedding[:, 0:self.output_size]
        # value = embedding[:, -1]
        policy, value = segments[0], segments[1]

        return self.policy_activation(policy), self.value_activation(value)

    def configure_optimizers(self):
        return torch.optim.SGD(self.parameters(), lr=0.05)
        # return torch.optim.Adam(self.parameters(), lr=0.1)
