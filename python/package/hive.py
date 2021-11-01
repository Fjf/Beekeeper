import ctypes
import os
from collections import defaultdict
from ctypes import *
from typing import Iterable

import numpy as np

from utils import HiveState

lib = CDLL(os.path.join(os.getcwd(), "libhive.so"))
# lib = CDLL("/home/duncan/CLionProjects/hive_engine/c/cmake-build-debug/libhive.so")
BOARD_SIZE = c_uint.in_dll(lib, "pboardsize").value
TILE_STACK_SIZE = c_uint.in_dll(lib, "ptilestacksize").value

MAX_TURNS = c_uint.in_dll(lib, "pmaxturns").value
TILE_MASK = ((1 << 5) - 1)
COLOR_MASK = (1 << 5)
NUMBER_MASK = (3 << 7)

N_TILES = 22


class TileStack(Structure):
    _fields_ = [
        ('type', c_ubyte),
        ('location', c_int),
        ('z', c_ubyte)
    ]


class Player(Structure):
    _fields_ = [
        ('beetles_left', c_ubyte),
        ('grasshoppers_left', c_ubyte),
        ('queens_left', c_ubyte),
        ('ants_left', c_ubyte),
        ('spiders_left', c_ubyte)
    ]


class Board(Structure):
    _fields_ = [
        ('tiles', c_ubyte * BOARD_SIZE * BOARD_SIZE),
        ('free', c_bool * BOARD_SIZE * BOARD_SIZE),
        ('turn', c_int),
        ('players', Player * 2),

        ('light_queen_position', c_int),
        ('dark_queen_position', c_int),

        ('min_x', c_int),
        ('min_y', c_int),
        ('max_x', c_int),
        ('max_y', c_int),

        ('n_stacked', c_byte),
        ('stack', TileStack * TILE_STACK_SIZE),

        ('move_location_tracker', c_int),

        ('zobrist_hash', c_longlong),
        ('hash_history', c_longlong * 180),

        ('has_updated', c_bool),
    ]

    def to_np(self):
        data = np.zeros((5, 26, 26), dtype=c_ubyte)

        # Fill zero array with raw binary data from library
        data[0, :, :] = np.frombuffer(self.tiles, c_ubyte).reshape((26, 26))

        # Compute highest tile in each stack
        m = defaultdict(int)
        for tile in self.stack:
            z, location, tile_type = tile.z, tile.location, tile.type
            if location == -1:
                continue
            m[location] = max(m[location], z)

        # Move current tiles to the point above the highest in the stack
        for location, m in m.items():
            x, y = location % 26, location // 26
            data[m + 1, x, y] = data[0, x, y]

        # Fill the rest of the stacks with the correct tiles
        for tile in self.stack:
            z, location, tile_type = tile.z, tile.location, tile.type
            if location == -1:
                continue
            x, y = location % 26, location // 26
            data[z, x, y] = tile_type
        return data


class List(Structure):
    pass


List._fields_ = [
    ('head', POINTER(List)),
    ('next', POINTER(List)),
    ('prev', POINTER(List)),
]


class MMData(Structure):
    _fields_ = [
        ('mm_value', c_float)
    ]


class MCTSData(Structure):
    _fields_ = [
        ('white', c_uint),
        ('black', c_uint),
        ('draw', c_uint),
        ('keep', c_bool),
        ('priority', c_float),
    ]


class Move(Structure):
    _fields_ = [
        ('tile', c_ubyte),
        ('next_to', c_ubyte),
        ('direction', c_ubyte),
        ('previous_location', c_int),
        ('location', c_int),
    ]


#
# The PlayerArguments structure is used to specify to MCTS or Minimax what parameters to use in the search.
# Algorithm can be between 0 and 3;
#   0 - Minimax
#   1 - MCTS
#   2 - Random
#   3 - Manual
# MCTS constant is the constant used for UCB1 to define the exploration factor.
# Time to move is the amount of allotted time to select a move.
# Prioritization is an MCTS playout prioritization strategy to reduce the amount of draws.
# First play urgency is an MCTS enhancement to quickly identify good branches early on.
# Verbose generates more output per algorithm.
# Evaluation function is a switch case for Minimax, it can be 0 or 1;
#   0 - Queen surrounding prioritization
#   1 - Opponent tile blocking prioritization
#


class PlayerArguments(Structure):
    _fields_ = [
        ('algorithm', c_int),
        ('mcts_constant', c_double),
        ('time_to_move', c_double),
        ('prioritization', c_bool),
        ('first_play_urgency', c_bool),
        ('verbose', c_bool),
        ('evaluation_function', c_int),
    ]


#
# The Arguments structure stored for each player what algorithm they use and what parameters to use for this algorithm.
#


class Arguments(Structure):
    _fields_ = [
        ('p1', PlayerArguments),
        ('p2', PlayerArguments),
    ]


class Node(Structure):
    _fields_ = [
        ('children', List),
        ('node', List),
        ('move', Move),
        ('board', POINTER(Board)),
        ('data', POINTER(MCTSData))
    ]

    def to_np(self):
        return self.board.contents.to_np()

    def encode(self):
        move: Move = self.move
        tile = move.tile
        next_to = move.next_to
        direction = move.direction

        tile_idx = lib.to_tile_index(next_to)
        return ((tile & TILE_MASK) * 22 + tile_idx) * 7 + direction

    def generate_moves(self):
        """
        Generates the moves for the current root node

        :return:
        """
        res = lib.generate_children(pointer(self), ctypes.c_double(1e64), 0)
        if res != 0:
            print(f"Error: generate_children returned {res}, exiting.")
            exit(1)

    def get_children(self):
        """
        A generator for all children of this node.

        :return:
        """
        if self.board.contents.move_location_tracker == 0:
            self.generate_moves()

        head = self.children.next

        while ctypes.addressof(head.contents) != ctypes.addressof(self.children.head):
            # Get struct offset
            child = lib.list_get_node(head)

            yield child.contents

            head = head.contents.next

    def finished(self) -> HiveState:
        result = lib.finished_board(self.board)
        return HiveState(result)

    def print(self):
        lib.print_board(self.board)


# Set return types for all functions we're using here.
lib.game_init.restype = POINTER(Node)
lib.list_get_node.restype = POINTER(Node)
lib.default_init.restype = POINTER(Node)
lib.init_board.restype = POINTER(Board)
lib.performance_testing.restype = ctypes.c_int
lib.random_moves.restype = POINTER(Node)
lib.minimax.restype = POINTER(Node)
lib.mcts_select_leaf.restype = POINTER(Node)

lib.mcts.restype = POINTER(Node)


class Hive(object):
    def __init__(self, track_history=False):
        # TODO: Combine history and history idx into a single list. (easier management)
        self.history: list[POINTER(Node)] = []
        self.history_idx: list[int] = []

        self.node = lib.game_init()

        self.track_history = track_history
        self.turn_limit = MAX_TURNS

    def __del__(self):
        for node in self.history:
            lib.node_free(node)
        lib.node_free(self.node)

    def turn(self):
        return self.node.contents.board.contents.turn

    def print(self):
        """
        Prints the current board-state

        :return:
        """
        self.node.contents.print()

    def finished(self) -> HiveState:
        """
        Returns the boards finished state

        :return:
        """
        return self.node.contents.finished()

    def children(self) -> Iterable[Node]:
        """
        A generator returning child pointers, wrap it with list() to use all children in a python list.

        :return:
        """
        yield from self.node.contents.get_children()

    def select_child(self, child: Node, n=None):
        """
        Selects a child to be used for the next step, it will free the current root node and replace it with
          the passed child.

        :param n: the index of the child
        :param child: the new root node
        :return:
        """

        if self.track_history:
            if n is not None:
                self.history_idx.append(n)

            copy = lib.default_init()
            lib.node_copy(copy, self.node)
            lib.list_empty(pointer(copy.contents.children))
            self.history.append(copy)

        lib.list_remove(pointer(child.node))
        lib.node_free(self.node)
        # Ensure no dangling children.
        lib.node_free_children(pointer(child))
        self.node = pointer(child)

    def ai_move(self, algorithm="random", config: PlayerArguments = None):
        """
        Do an AI move based on passed algorithm

        :param config: optional configuration for the algorithm.
        :param algorithm: one of [random, mm, mcts]
        :return: the selected child based on the algorithms heuristics
        """

        if not config:
            config = PlayerArguments()
            config.time_to_move = 0.1
            config.verbose = False
            config.evaluation_function = 3

        if algorithm == "random":
            child = lib.random_moves(self.node, 1)
        elif algorithm == "mm":
            child = lib.minimax(self.node, pointer(config))
        elif algorithm == "mcts":
            child = lib.mcts(self.node, pointer(config))
        else:
            raise ValueError("Unknown algorithm type.")

        self.select_child(child.contents)

    def reinitialize(self):
        """
        Release the root node from previous games, and re-initialize all data required to start a new game.

        :return:
        """
        if self.track_history:
            # Free all history nodes
            for node in self.history:
                lib.node_free(node)
            self.history = []
            self.history_idx = []

        # Cleanup old node
        lib.node_free(self.node)

        node = lib.default_init()
        self.node = node
        self.node.contents.board = lib.init_board()

    def test(self):
        """
        Quick test doing 20 random moves using the engine and printing the output.

        :return:
        """
        for i in range(20):
            self.ai_move()
            self.print()
            if self.finished() != HiveState.UNDETERMINED:
                return
