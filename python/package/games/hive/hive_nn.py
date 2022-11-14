import pytorch_lightning as pl
import torch
from pytorch_lightning.utilities.types import STEP_OUTPUT
from torch import nn

from games.utils import ResNetBlock
from games.hive.hive import Hive


class HiveNN(pl.LightningModule):
    def __init__(self, input_size=Hive.input_space, output_size=Hive.action_space):
        super().__init__()

        n_planes = 23

        # self.encoder = nn.Sequential(
        #     ResNetBlock(n_planes, n_planes),
        #     ResNetBlock(n_planes, n_planes),
        #     ResNetBlock(n_planes, n_planes),
        #     ResNetBlock(n_planes, n_planes),
        #
        #     nn.Flatten(),
        #     nn.Linear(n_planes * 26 * 26, output_size + 1)
        # )

        self.encoder = nn.Sequential(
            nn.Conv2d(in_channels=n_planes, out_channels=n_planes, kernel_size=3, stride=1, padding=1,
                      padding_mode="circular"),
            nn.ReLU(),
            nn.Conv2d(in_channels=n_planes, out_channels=n_planes, kernel_size=3, stride=1, padding=1,
                      padding_mode="circular"),
            nn.ReLU(),
            nn.MaxPool2d(2),

            nn.Conv2d(in_channels=n_planes, out_channels=n_planes, kernel_size=3, stride=1, padding=1,
                      padding_mode="circular"),
            nn.ReLU(),
            nn.Conv2d(in_channels=n_planes, out_channels=n_planes, kernel_size=3, stride=1, padding=1,
                      padding_mode="circular"),
            nn.ReLU(),
            nn.MaxPool2d(2),

            nn.Conv2d(in_channels=n_planes, out_channels=n_planes, kernel_size=3, stride=1, padding=1,
                      padding_mode="circular"),
            nn.ReLU(),
            nn.Conv2d(in_channels=n_planes, out_channels=n_planes, kernel_size=3, stride=1, padding=1,
                      padding_mode="circular"),
            nn.ReLU(),
            nn.MaxPool2d(2),

            nn.Conv2d(in_channels=n_planes, out_channels=n_planes, kernel_size=3, stride=1, padding=1,
                      padding_mode="circular"),
            nn.ReLU(),
            nn.Conv2d(in_channels=n_planes, out_channels=n_planes, kernel_size=3, stride=1, padding=1,
                      padding_mode="circular"),
            nn.ReLU(),

            nn.Flatten(),
            nn.Linear(207, output_size + 1)
        )

        self.policy_activation = nn.Softmax(dim=1)
        self.value_activation = nn.Tanh()

        self.output_size = output_size
        self.input_size = input_size

    def training_step(self, batch, batch_idx) -> STEP_OUTPUT:
        x, pi, game_value = batch

        # Increase class differences
        pi = torch.pow(pi, 1)
        # pi /= torch.sum(pi, dim=1).view(pi.shape[0], 1)

        policy, value = self(x)

        # l2_lambda = 0.001
        # l2_norm = sum(p.pow(2.0).sum() for p in self.encoder.parameters())
        # l2_regularization = l2_norm * l2_lambda

        value_loss = self.loss_v(game_value, value)
        policy_loss = self.loss_pi(pi, policy)

        loss = (value_loss - policy_loss).mean()
        return loss

    @staticmethod
    def loss_pi(targets: torch.Tensor, outputs):
        shape = targets.shape
        result = torch.bmm(targets.view(shape[0], 1, shape[1]), torch.log(outputs).view(shape[0], shape[1], 1))
        return result.flatten()

    @staticmethod
    def loss_v(targets, outputs):
        return ((targets - outputs) ** 2).flatten()

    def forward(self, x):
        embedding = self.encoder(x)
        segments = torch.tensor_split(embedding, (self.output_size,), dim=1)
        policy, value = segments[0], segments[1]

        return self.policy_activation(policy), self.value_activation(value)

    def configure_optimizers(self):
        return torch.optim.SGD(self.parameters(), lr=0.05)
        # return torch.optim.Adam(self.parameters(), lr=0.01)
