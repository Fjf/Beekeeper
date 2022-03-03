import random
from typing import List

import numpy as np
import torch
from scipy.signal import convolve2d

from games.Game import Game, GameNode
from games.utils import GameState, Perspectives


class Connect4Node(GameNode):
    width = 7
    height = 5

    def __init__(self, parent, board: np.array = None, move: int = -1):
        super().__init__(parent)
        self._turn = parent.turn() + 1 if parent is not None else 0
        self.children = []

        if board is None:
            self._board = np.zeros((self.width, self.height), dtype=np.byte)
        else:
            self._board = board.copy()

        self.move = None

        if move != -1:
            self.move = move
            for i in range(self.height):
                if self._board[move][i] == 0:
                    self._board[move][i] = ((self.turn() - 1) % 2) + 1
                    break

    def turn(self):
        return self._turn

    def to_np(self, perspective) -> np.array:
        arr = self._board.flatten()
        if perspective == Perspectives.PLAYER1:
            arr = arr
        else:
            maskp2 = arr == 2
            maskp1 = arr == 1
            arr[maskp2] = 1
            arr[maskp1] = 2

        # Set values to 1, -1 instead of 1, 2
        arr[arr == 2] = -1
        return np.append(arr, [self.turn() % 2])

    def encode(self) -> int:
        return self.move

    def get_children(self) -> List[GameNode]:
        if self.finished() != GameState.UNDETERMINED:
            return []

        if len(self.children) == 0:
            for i in range(self.width):
                if self._board[i][self.height - 1] == 0:
                    self.children.append(Connect4Node(self, self._board, i))

        return self.children

    def finished(self) -> GameState:
        horizontal_kernel = np.array([[1, 1, 1, 1]])
        vertical_kernel = np.transpose(horizontal_kernel)
        diag1_kernel = np.eye(4, dtype=np.uint8)
        diag2_kernel = np.fliplr(diag1_kernel)
        detection_kernels = [horizontal_kernel, vertical_kernel, diag1_kernel, diag2_kernel]

        # Check all connect 4 rules for both player 1 and 2.
        for kernel in detection_kernels:
            if (convolve2d(self._board == 1, kernel, mode="valid") == 4).any():
                return GameState.WHITE_WON
            if (convolve2d(self._board == 2, kernel, mode="valid") == 4).any():
                return GameState.BLACK_WON

        # Check if there are any valid moves left after checking for winning players.
        for i in range(self.width):
            if self._board[i][self.height - 1] == 0:
                return GameState.UNDETERMINED

        # If there are no moves left, this game is a draw.
        return GameState.DRAW_TURN_LIMIT

    def print(self):
        print(np.rot90(self._board))


class Connect4(Game):
    # Set neural network dimensions
    input_space = Connect4Node.width * Connect4Node.height + 1
    action_space = 7

    def __init__(self):
        super().__init__()

        # Amount of turns equal to amount of open spots.
        self.turn_limit = Connect4Node.width * Connect4Node.height

        self.history = []
        self.node = Connect4Node(None)
        self.history.append(self.node)  # Add first node too.

    def turn(self) -> int:
        return self.node.turn()

    def get_inverted(self, boards: List[torch.Tensor]) -> List[torch.Tensor]:
        output = []
        for board in boards:
            new_board = -board
            new_board[-1] += 1  # Last number is [0,1] [-0 + 1 = 1, -1 + 1 = 0]
        return output

    def print(self):
        return self.node.print()

    def finished(self) -> GameState:
        return self.node.finished()

    def children(self) -> List[GameNode]:
        return self.node.get_children()

    def select_child(self, child):
        self.history.append(child)
        self.node = child

    def ai_move(self, algorithm: str):
        if algorithm == "random":
            children = self.node.get_children()
            self.select_child(random.sample(children, 1)[0])
            return
        raise NotImplemented("No move algorithms are implemented for Connect4.")
