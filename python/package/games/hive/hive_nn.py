import pytorch_lightning as pl
import torch
from pytorch_lightning.utilities.types import STEP_OUTPUT
from torch import nn


class HiveNN(pl.LightningModule):
    def __init__(self, input_size=26*26 * 2, output_size=26*26*11):
        super().__init__()
        self.encoder = nn.Sequential(
            nn.Conv2d(2, 4, (3, 3), padding=1),
            nn.ReLU(),

            nn.Conv2d(4, 16, (3, 3), padding=1),
            nn.ReLU(),

            nn.Conv2d(16, 16, (3, 3), padding=1),
            nn.ReLU(),

            nn.Conv2d(16, 16, (3, 3), padding=2),
            nn.ReLU(),

            nn.Conv2d(16, 16, (3, 3), padding=2),
            nn.ReLU(),
            nn.MaxPool2d((2, 2)),

            nn.Flatten(),
            nn.Linear(3600, output_size + 1)
        )

        self.policy_activation = nn.Softmax(dim=1)
        self.value_activation = nn.Tanh()

        self.output_size = output_size
        self.input_size = input_size

        self.policy_loss = nn.CrossEntropyLoss()
        self.value_loss = nn.MSELoss()

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
        policy, value = segments[0], segments[1]

        return self.policy_activation(policy), self.value_activation(value)

    def configure_optimizers(self):
        return torch.optim.SGD(self.parameters(), lr=0.05)
        # return torch.optim.Adam(self.parameters(), lr=0.02)
