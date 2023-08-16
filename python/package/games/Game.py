from __future__ import annotations

from abc import ABC
from typing import Iterable, List, Union

import numpy as np
import torch

from games.utils import GameState, Perspectives


class MCTSData(object):
    def __init__(self):
        self.n_sims = 0
        self.value = 0
        self.prior = 0
        self.policy = None

    def reset(self):
        self.n_sims = 0
        self.prior = 0
        self.value = 0
        self.policy = None

    def __repr__(self):
        return f"MCTSData: (sims: {self.n_sims}, value: {self.value})"


class GameNode(ABC):
    turn_limit = -1

    def __init__(self, parent):
        self.parent = parent
        self.n_players = 1
        self.children = []
        self.done = False
        self.mcts = MCTSData()

    def to_np(self, perspective: Union[Perspectives, int]) -> np.array:
        """
        Generates a numpy-representation of this node from the perspective of player

        :return: numpy array
        """

    def encode(self) -> int:
        """
        Encode the move resulting in this board as an integer in the range [0, Game.action_space - 1]

        :return: the encoded move
        """

    def expand(self):
        """
        Generates the children in the internal list.
        """

    def finished(self) -> GameState:
        """
        Gives a GameState with the current state of the game.

        :return:
        """

    def print(self):
        """
        Outputs the state of this node to stdout.

        :return:
        """

    @property
    def turn(self) -> int:
        """
        Gives the current turn in the game.

        :return:
        """
        return 0

    @property
    def to_play(self) -> int:
        return self.turn % self.n_players


class Game(ABC):
    """
    A base class for two-player games.
    Implement all functions for integration with automatic MPI training sample generation.
    """
    input_space = -1
    action_space = -1
    turn_limit = 0

    def __init__(self):
        self.node = None
        self.history = []

    def get_inverted(self, boards: List[torch.Tensor]) -> List[torch.Tensor]:
        """
        Invert the list of boards to create more data.
        :param boards:
        :return:
        """

    @property
    def turn(self) -> int:
        """
        Returns the current turn in the game

        :return:
        """
        return 0

    def to_play(self) -> int:
        return self.turn % 2

    def print(self):
        """
        Prints the current state of the game to stdout.

        :return:
        """

    def finished(self) -> GameState:
        """
        Returns the game state.

        :return: GameState
        """

    def reset_children(self):
        """
        Generates all children from this gamestate.

        :return:
        """

    def children(self) -> Iterable[GameNode]:
        """
        Returns an iterable for GameNodes, in no particular order.

        :return:
        """

    def select_child(self, child):
        """
        Sets the child as the root node of the game object.
        The history list should be managed here.

        :param child: a GameNode
        :return:
        """

    def ai_move(self, algorithm: str):
        """
        Do a move with a non-neural network algorithm instead.

        :param algorithm:
        :return:
        """
