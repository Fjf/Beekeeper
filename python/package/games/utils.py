import enum

import numpy as np
from torch import nn


def remove_tile_idx(x):
    return x & ((1 << 6) - 1)


def flip(x):
    res = np.empty(x.shape, dtype=int)
    mask = x != 0
    res[mask] = x[mask] ^ (1 << 5)
    res[~mask] = x[~mask]
    return res


class ResNetBlock(nn.Module):
    def __init__(self, in_channels, out_channels, intermediate_factor=1, stride=1):
        """
        Args:
          in_channels (int):  Number of input channels.
          out_channels (int): Number of output channels.
          stride (int):       Controls the stride.
        """
        super(ResNetBlock, self).__init__()

        intermediate_channels = in_channels // intermediate_factor

        self.block = nn.Sequential(
            nn.BatchNorm2d(in_channels),
            nn.ReLU(),
            nn.Conv2d(in_channels=in_channels, out_channels=intermediate_channels, kernel_size=3, padding=1, stride=1,
                      bias=False),
            nn.BatchNorm2d(intermediate_channels),
            nn.ReLU(),
            nn.Conv2d(in_channels=intermediate_channels, out_channels=out_channels, kernel_size=3, padding=1, stride=1,
                      bias=False),
            # nn.BatchNorm2d(out_channels)
        )

        self.skip = nn.Sequential(
            nn.Conv2d(in_channels=in_channels, out_channels=out_channels, kernel_size=1, stride=stride, bias=False),
            nn.BatchNorm2d(out_channels)
        )

    def forward(self, x):
        out = self.block(x)
        out += self.skip(x)
        # out = torch.nn.functional.relu(out)
        return out
