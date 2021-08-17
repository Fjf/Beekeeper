import ctypes
import enum
import os
from ctypes import *
from typing import Iterable

lib = CDLL(os.path.join(os.getcwd(), "/home/duncan/CLionProjects/hive_engine/python/package/libhive.so"))
board_size = c_uint.in_dll(lib, "pboardsize").value
tile_stack_size = c_uint.in_dll(lib, "ptilestacksize").value


class TileStack(Structure):
    _fields_ = [
        ('type', c_ubyte),
        ('location', c_int),
        ('z', c_ubyte)
    ]


class Tile(Structure):
    _fields_ = [
        ('free', c_bool),
        ('type', c_ubyte)
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
        ('tiles', Tile * board_size * board_size),
        ('turn', c_int),
        ('players', Player * 2),

        ('light_queen_position', c_int),
        ('dark_queen_position', c_int),

        ('min_x', c_int),
        ('min_y', c_int),
        ('max_x', c_int),
        ('max_y', c_int),

        ('n_stacked', c_byte),
        ('stack', TileStack * tile_stack_size),

        ('move_location_tracker', c_int),

        ('zobrist_hash', c_longlong),
        ('hash_history', c_longlong * 180),

        ('has_updated', c_bool),
    ]


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


class Move(Structure):
    _fields_ = [
        ('tile', c_byte),
        ('next_to', c_byte),
        ('direction', c_byte),
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
        ('data', c_uint)
    ]

    def to_np(self):
        return numpy.frombuffer(self.board.contents.tiles, float)

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
lib.mcts.restype = POINTER(Node)


class HiveState(enum.Enum):
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


class Hive:
    def __init__(self, track_history=False):
        self.history: list[POINTER(Node)] = []
        self.node = lib.game_init()
        self.track_history = track_history

    def generate_moves(self):
        """
        Generates the moves for the current root node

        :return:
        """
        lib.generate_moves(self.node)

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
        result = lib.finished_board(self.node.contents.board)
        return HiveState(result)

    def children(self) -> Iterable[Node]:
        """
        A generator returning child pointers, wrap it with list() to use all children in a python list.

        :return:
        """
        if self.node.contents.board.contents.move_location_tracker == 0:
            self.generate_moves()

        head = self.node.contents.children.next

        while ctypes.addressof(head.contents) != ctypes.addressof(self.node.contents.children.head):
            # Get struct offset
            child = lib.list_get_node(head)

            yield child.contents

            head = head.contents.next

    def select_child(self, child: POINTER(Node)):
        """
        Selects a child to be used for the next step, it will free the current root node and replace it with
          the passed child.

        :param child: the new root node
        :return:
        """
        if self.track_history:
            copy = pointer(Node())
            lib.node_copy(copy, self.node)
            lib.list_empty(pointer(copy.contents.children))
            self.history.append(copy)

        lib.list_remove(pointer(child.contents.node))
        lib.node_free(self.node)
        self.node = child

    def ai_move(self, algorithm="random"):
        """
        Do an AI move based on passed algorithm

        :param algorithm: one of [random, mm, mcts]
        :return: the selected child based on the algorithms heuristics
        """
        config = PlayerArguments()
        config.time_to_move = 0.01
        if algorithm == "random":
            child = lib.random_moves(self.node, 1)
        elif algorithm == "mm":
            child = lib.minimax(self.node, config)
        elif algorithm == "mcts":
            child = lib.minimax(self.node, config)
        else:
            raise ValueError("Unknown algorithm type.")

        self.select_child(child)

    def reinitialize(self):
        """
        Release the root node from previous games, and re-initialize all data required to start a new game.

        :return:
        """
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

