import random
from typing import List, Optional

import numpy as np
import torch
from scipy.signal import convolve2d

from games.Game import Game, GameNode


class Connect4Node(GameNode):
    width = 7
    height = 5
    INITIAL_STATE = 8

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
            for i in range(self.height):
                if self._board[move][i] == self.INITIAL_STATE:
                    player = (self.turn - 1) % self.n_players
                    self._board[move][i] = player
                    break

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
            for i in range(self.width):
                if self._board[i][self.height - 1] == self.INITIAL_STATE:
                    self.children.append(Connect4Node(self, self._board, i))

    def winner(self) -> Optional[int]:
        def check_finish():
            horizontal_kernel = np.array([[1, 1, 1, 1]])
            vertical_kernel = np.transpose(horizontal_kernel)
            diag1_kernel = np.eye(4, dtype=np.uint8)
            diag2_kernel = np.fliplr(diag1_kernel)
            detection_kernels = [horizontal_kernel, vertical_kernel, diag1_kernel, diag2_kernel]

            # Check all connect 4 rules for both player 1 and 2.
            for kernel in detection_kernels:
                if (convolve2d(self._board == 0, kernel, mode="valid") == 4).any():
                    return 0
                if (convolve2d(self._board == 1, kernel, mode="valid") == 4).any():
                    return 1

            # Check if there are any valid moves left after checking for winning players.
            for i in range(self.width):
                if self._board[i][self.height - 1] == self.INITIAL_STATE:
                    return None

            # If there are no moves left, this game is a draw.
            return -1

        if self._finished is None:
            self._finished = check_finish()

        return self._finished

    def __repr__(self):
        return "|" + "|\n|".join(
            ' '.join(str(self._board[x, y]) for x in range(self.width)) for y in range(self.height)) + "|"

    def print(self):
        print(np.rot90(self._board))

    def reset_children(self):
        self.children = []


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

    @property
    def turn(self) -> int:
        return self.node.turn

    def get_inverted(self, boards: List[torch.Tensor]) -> List[torch.Tensor]:
        output = []
        # for board in boards:
        #     new_board = board.clone()
        #     new_board[[0, 1]] = new_board[[1, 0]]
        #     output.append(new_board)
        return output

    def print(self):
        print(self.node)

    def winner(self) -> Optional[int]:
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
        raise NotImplemented("No move algorithms are implemented for Connect4.")
