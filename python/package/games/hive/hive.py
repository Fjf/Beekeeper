import copy
import ctypes
import os
from collections import defaultdict
from ctypes import *
from typing import Iterable

import numpy as np
import torch

from games.Game import Game, GameNode
from games.utils import GameState, Perspectives

lib = CDLL(os.path.join(os.path.dirname(os.path.realpath(__file__)), "libhive.so"))
BOARD_SIZE = c_uint.in_dll(lib, "pboardsize").value
TILE_STACK_SIZE = c_uint.in_dll(lib, "ptilestacksize").value
N_NODES = c_uint.in_dll(lib, "n_nodes")

MAX_TURNS = c_uint.in_dll(lib, "pmaxturns").value
TILE_MASK = ((1 << 5) - 1)
COLOR_MASK = (1 << 5)
COLOR_SHIFT = 5
NUMBER_MASK = (3 << 7)
NUMBER_SHIFT = 6

TILE_ANT = 1
TILE_GRASSHOPPER = 2
TILE_BEETLE = 3
TILE_SPIDER = 4
TILE_QUEEN = 5

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

    def to_np(self, perspective: Perspectives):
        """
        Convert internal array representation to numpy array
        It will always display the board from white's perspective.
        :return:
        """
        player = 0 if perspective == Perspectives.PLAYER1 else 1

        n_planes = 2
        height = 1
        # Fill zero array with raw binary data from library
        planes = np.zeros((n_planes, height, BOARD_SIZE, BOARD_SIZE), dtype=c_ubyte)
        planes[0] = np.frombuffer(self.tiles, c_ubyte).reshape((26, 26))
        planes[1] = np.frombuffer(self.tiles, c_ubyte).reshape((26, 26))

        data_color = (planes[0] & COLOR_MASK) >> COLOR_SHIFT

        planes[0][data_color == player] = 0
        planes[1][data_color != player] = 0

        # Remove color from the data as it is now split into planes
        planes &= 223

        return planes.reshape((n_planes, BOARD_SIZE, BOARD_SIZE))

        # Compute highest tile in each stack
        m = defaultdict(int)
        for tile in self.stack:
            z, location, tile_type = tile.z, tile.location, tile.type
            if location == -1:
                continue
            m[location] = max(m[location], z)

        # Move current tiles to the point above the highest in the stack
        for location, height in m.items():
            x, y = location % 26, location // 26
            data[height + 1, x, y] = data[0, x, y]

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
        ('value', c_double),
        ('n_sims', c_uint),
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


# Set return types for all functions we're using here.
lib.game_init.restype = POINTER(Node)
lib.list_get_node.restype = POINTER(Node)
lib.default_init.restype = POINTER(Node)
lib.init_board.restype = POINTER(Board)
lib.performance_testing.restype = ctypes.c_int
lib.random_moves.restype = POINTER(Node)
lib.minimax.restype = POINTER(Node)
lib.mcts_select_leaf.restype = POINTER(Node)
lib.mcts_cascade_result.argtypes = [POINTER(Node), POINTER(Node), c_double]
lib.count_tiles_around.restype = ctypes.c_int
lib.mcts.restype = POINTER(Node)

lib.string_move.argtypes = [POINTER(Node)]
lib.string_move.restype = c_char_p


class HiveNode(GameNode):
    encoding = "absolute"

    def __init__(self, parent, node: POINTER(Node)):
        super().__init__(parent)

        self.cnode = lib.default_init()
        lib.node_copy(self.cnode, node)
        self.children = []

    def turn(self):
        return self.cnode.contents.board.contents.turn

    def to_np(self, perspective: Perspectives):
        return self.cnode.contents.board.contents.to_np(perspective)

    def encode(self):
        # We have no move to get to an initial state.
        if self.turn() == 0:
            return None

        move: Move = self.cnode.contents.move
        tile = move.tile
        my_idx = lib.to_tile_index(tile & ~COLOR_MASK) - 1  # Range 0:11  (only my tiles (-1 because cannot be empty))

        if HiveNode.encoding == "absolute":
            location = move.location
            return my_idx * 26 * 26 + location
        elif HiveNode.encoding == "relative":
            next_to = move.next_to
            direction = move.direction

            # Returns number of the tile in this order; A3 G3 B2 S2 Q1 -> max idx = 11*2 (W + B)
            tile_idx = lib.to_tile_index(next_to)  # Range 0:23  (all tiles + empty)

            return (my_idx * 23 + tile_idx) * 7 + direction

    def _generate_children(self):
        """
        Generates the moves for the current root node

        :return:
        """
        res = lib.generate_children(self.cnode, ctypes.c_double(1e64), 0)
        if res != 0:
            print(f"Error: generate_children returned {res}, exiting.")
            exit(1)

    def get_children(self):
        """
        A generator for all children of this node.

        :return:
        """

        if len(self.children) != 0:
            return self.children

        if self.finished() != GameState.UNDETERMINED:
            return []

        if self.cnode.contents.board.contents.move_location_tracker == 0:
            self._generate_children()

        head = self.cnode.contents.children.next

        while ctypes.addressof(head.contents) != ctypes.addressof(self.cnode.contents.children.head):
            # Get struct offset
            child = lib.list_get_node(head)

            self.children.append(HiveNode(self, child))

            head = head.contents.next

        # After generating python nodes, delete the old ones
        lib.node_free_children(self.cnode)
        return self.children

    def finished(self) -> GameState:
        result = lib.finished_board(self.cnode.contents.board)
        return GameState(result)

    def print(self):
        lib.print_board(self.cnode.contents.board)

    def print_move(self):
        lib.print_move(self.cnode)

    def __deepcopy__(self, memo):
        cls = self.__class__
        gnode = cls.__new__(cls)
        memo[id(self)] = gnode
        lib.node_copy(gnode.cnode, self.cnode)
        return gnode

    def __del__(self):
        lib.node_free(self.cnode)


class Hive(Game):
    encodings = {
        "absolute": 11 * 26 * 26,
        "relative": 11 * 23 * 7
    }
    input_space = 26 * 26
    action_space = encodings["absolute"]

    def __init__(self):
        super().__init__()

        self.history: list[HiveNode] = []

        cnode = lib.game_init()
        self.node = HiveNode(None, cnode)
        lib.node_free(cnode)

        self.history.append(self.node)

        self.turn_limit = MAX_TURNS

    def get_inverted(self, boards: list[torch.Tensor]) -> list[torch.Tensor]:
        return []  # TODO: Implement this.

    def turn(self):
        return self.node.turn()

    def print(self):
        self.node.print()

    def finished(self) -> GameState:
        return self.node.finished()

    def children(self) -> Iterable[HiveNode]:
        yield from self.node.get_children()

    def select_child(self, child: HiveNode):
        self.history.append(child)

        # Remove C-references to memory
        lib.node_free_children(self.node.cnode)

        del self.node.children

        self.node = child

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
            child = lib.random_moves(self.node.cnode, 1)
        elif algorithm == "mm":
            child = lib.minimax(self.node.cnode, pointer(config))
        elif algorithm == "mcts":
            child = lib.mcts(self.node.cnode, pointer(config))
        else:
            raise ValueError("Unknown algorithm type.")

        self.select_child(child.contents)
