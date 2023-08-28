import random
from typing import List

import numpy as np
import torch
from scipy.signal import convolve2d

from games.Game import Game, GameNode


class TicTacToeNode(GameNode):
    width = 3
    height = 3

    INITIAL_STATE = 7

    def __init__(self, parent, board: np.array = None, move: int = -1):
        super().__init__(parent)
        self.n_players = 2

        self._finished = None
        self._turn = parent.turn + 1 if parent is not None else 0
        self.children = []

        if board is None:
            self._board = np.zeros((self.width, self.height), dtype=np.byte) + self.INITIAL_STATE
        else:
            self._board = board.copy()

        self.move = None

        if move != -1:
            self.move = move
            # We need to set the player for the previous turn, we incremented turn before
            player = (self.turn - 1) % self.n_players
            self._board[move // 3][move % 3] = player

    @property
    def turn(self):
        return self._turn

    def to_np(self, perspective) -> np.array:
        output_arr = np.zeros((2, *self._board.shape))

        if perspective == 0:
            output_arr[0, self._board == 0] = 1
            output_arr[1, self._board == 1] = 1
        else:
            output_arr[0, self._board == 1] = 1
            output_arr[1, self._board == 0] = 1

        return output_arr

    def encode(self) -> int:
        return self.move

    def expand(self):
        if self.winner() is not None:
            raise Exception("Cannot expand terminal game-state.")

        if len(self.children) == 0:
            for i in range(self.width * self.height):
                if self._board[i // 3][i % 3] == self.INITIAL_STATE:
                    self.children.append(TicTacToeNode(self, self._board, i))

    def winner(self):
        def check_finish():
            horizontal_kernel = np.array([[1, 1, 1]])
            vertical_kernel = np.transpose(horizontal_kernel)
            diag1_kernel = np.eye(3, dtype=np.uint8)
            diag2_kernel = np.fliplr(diag1_kernel)
            detection_kernels = [horizontal_kernel, vertical_kernel, diag1_kernel, diag2_kernel]

            # Check all connect 4 rules for both player 1 and 2.
            for kernel in detection_kernels:
                if (convolve2d(self._board == 0, kernel, mode="valid") == 3).any():
                    return 0
                if (convolve2d(self._board == 1, kernel, mode="valid") == 3).any():
                    return 1

            # Check if there are any valid moves left after checking for winning players.
            for i in range(self.width * self.height):
                if self._board[i // 3][i % 3] == self.INITIAL_STATE:
                    return None

            # If there are no moves left, this game is a draw.
            return -1

        if self._finished is None:
            self._finished = check_finish()

        return self._finished

    def __repr__(self):
        return "|" + "|\n|".join(' '.join(str(self._board[y, x]) for x in range(3)) for y in range(3)) + "|"

    def reset_children(self):
        self.children = []


class TicTacToe(Game):
    # Set neural network dimensions
    input_space = TicTacToeNode.width * TicTacToeNode.height + 1
    action_space = 9

    def __init__(self):
        super().__init__()

        # Amount of turns equal to amount of open spots.
        self.turn_limit = TicTacToeNode.width * TicTacToeNode.height

        self.history = []
        self.node = TicTacToeNode(None)
        self.history.append(self.node)  # Add first node too.

    def get_inverted(self, boards: List[torch.Tensor]) -> List[torch.Tensor]:
        output = []
        for board in boards:
            new_board = board.clone()
            new_board[[0, 1]] = new_board[[1, 0]]
            output.append(new_board)
        return output

    @property
    def turn(self) -> int:
        return self.node.turn

    def print(self):
        print(self.node)

    def winner(self):
        return self.node.winner()

    def children(self) -> List[GameNode]:
        return self.node.children

    def reset_children(self):
        self.node.mcts.reset()
        self.node.reset_children()

    def select_child(self, child):
        self.history.append(child)
        self.node = child
        self.node.parent = None

    def ai_move(self, algorithm: str):
        if algorithm == "random":
            self.node.expand()
            children = self.node.children
            self.select_child(random.sample(children, 1)[0])
            return
        raise NotImplemented(f"No '{algorithm}' algorithm is implemented for TicTacToe.")
