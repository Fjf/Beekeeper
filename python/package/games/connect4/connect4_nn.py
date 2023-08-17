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
            ResNetBlock(2, 4),
            nn.MaxPool2d(2),
            ResNetBlock(4, 8),
            ResNetBlock(8, 8),

            nn.Flatten(),
            nn.ReLU(),
        )

        self.policy_head = nn.Sequential(
            nn.Linear(48, output_size),
            nn.ReLU(),
        )
        self.policy_activation = nn.LogSoftmax(dim=1)

        self.value_head = nn.Sequential(
            nn.Linear(48, 1),
            nn.Tanh()
        )

        self.value_loss = nn.MSELoss()
        self.policy_loss = nn.MSELoss()

        self.output_size = output_size
        self.input_size = input_size

    def training_step(self, batch, batch_idx) -> STEP_OUTPUT:
        x, pi, game_value = batch

        policy, value = self(x)

        policy = self.policy_activation(policy)

        # value_loss = self.value_loss(value, game_value)
        # policy_loss = self.policy_loss(policy, pi) * 100

        value_loss = self.loss_v(game_value, value)
        policy_loss = self.loss_pi(pi, policy)

        self.log("v_loss", value_loss, on_step=True, prog_bar=True, logger=True)
        self.log("p_loss", policy_loss, on_step=True, prog_bar=True, logger=True)

        return value_loss + policy_loss

    def loss_pi(self, targets, outputs):
        return -torch.mean(targets * outputs)

    def loss_v(self, targets, outputs):
        return torch.mean((targets - outputs.view(-1)) ** 2)

    def predict(self, board):
        batch = torch.Tensor(board).unsqueeze(0).to(self.device)

        policy, value = self(batch)
        policy = self.policy_activation(policy)
        policy = torch.exp(policy).to("cpu").squeeze()

        return policy, value

    def forward(self, x):
        embedding = self.encoder(x)
        return self.policy_head(embedding), self.value_head(embedding)

    def configure_optimizers(self):
        # return torch.optim.SGD(self.parameters(), lr=0.05)
        return torch.optim.AdamW(self.parameters(), lr=0.01, betas=(0.9, 0.999))
