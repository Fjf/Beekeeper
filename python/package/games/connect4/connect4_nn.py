import pytorch_lightning as pl
import torch
from pytorch_lightning.utilities.types import STEP_OUTPUT
from torch import nn


class Connect4NN(pl.LightningModule):
    def __init__(self, input_size=676, output_size=26 * 26 * 11):
        super().__init__()
        self.encoder = nn.Sequential(
            nn.Linear(input_size, 64),
            nn.ReLU(),
            nn.Linear(64, 128),
            nn.Dropout(p=0.3),
            nn.ReLU(),
            nn.Linear(128, 64),
            nn.Dropout(p=0.3),
            nn.ReLU(),
            nn.Linear(64, 32),
            nn.ReLU(),
            nn.Linear(32, output_size + 1)
        )
        self.policy_activation = nn.Softmax(dim=1)
        self.value_activation = nn.Tanh()

        self.value_loss = nn.MSELoss()
        self.policy_loss = nn.CrossEntropyLoss()

        self.output_size = output_size
        self.input_size = input_size

    def training_step(self, batch, batch_idx) -> STEP_OUTPUT:
        x, pi, game_value = batch

        policy, value = self(x)

        # idx = torch.argmax(pi, dim=1)
        # return self.value_loss(game_value, value) + self.policy_loss(policy, idx)

        value_loss = self.loss_v(game_value, value)
        policy_loss = self.loss_pi(pi, policy)
        return (value_loss - policy_loss).mean()

    @staticmethod
    def loss_pi(targets, outputs):
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
        policy, value = segments[0], segments[1]

        return self.policy_activation(policy), self.value_activation(value)

    def configure_optimizers(self):
        return torch.optim.SGD(self.parameters(), lr=0.01)
        # return torch.optim.Adam(self.parameters(), lr=0.01)
