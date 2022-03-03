import enum

import numpy as np


def remove_tile_idx(x):
    return x & ((1 << 6) - 1)


def flip(x):
    res = np.empty(x.shape, dtype=int)
    mask = x != 0
    res[mask] = x[mask] ^ (1 << 5)
    res[~mask] = x[~mask]
    return res


class GameState(enum.Enum):
    UNDETERMINED = 0
    WHITE_WON = 1
    BLACK_WON = 2
    DRAW_REPETITION = 3
    DRAW_TURN_LIMIT = 4

    def __str__(self):
        m = [
            "Undetermined",
            "White Won",
            "Black Won",
            "Draw by Repetition",
            "Draw by Turnlimit",
        ]
        return m[self.value]


class Perspectives:
    PLAYER1 = 0
    PLAYER2 = 1
