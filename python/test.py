import ctypes
import datetime
import random
from ctypes import *

f = "/home/duncan/CLionProjects/TheHive/c/cmake-build-debug/libhive.so"
lib = CDLL(f)
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
        ('hash_history', c_longlong * 100),

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
        ('mm_value', c_double),
        ('mm_evaluated', c_bool)
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

    def print(self):
        lib.print_board(self.board)


# Set return types for all functions we're using here.
lib.game_init.restype = POINTER(Node)
lib.list_get_node.restype = POINTER(Node)
lib.default_init.restype = POINTER(Node)
lib.init_board.restype = POINTER(Board)
lib.performance_testing.restype = ctypes.c_int


class Hive:
    def __init__(self):
        self.node = lib.game_init()

    def generate_moves(self):
        lib.generate_moves(self.node)

    def print(self):
        self.node.contents.print()

    def finished(self):
        return lib.finished_board(self.node.contents.board)

    def children(self):
        """
        A generator returning child pointers
        :return:
        """
        if self.node.contents.board.contents.move_location_tracker == 0:
            self.generate_moves()

        head = self.node.contents.children.next

        while ctypes.addressof(head.contents) != ctypes.addressof(self.node.contents.children.head):
            # Get struct offset
            child = lib.list_get_node(head)

            yield child

            head = head.contents.next

    def reinitialize(self):
        # Cleanup old node
        lib.node_free(self.node)

        node = lib.default_init()
        self.node = node
        self.node.contents.board = lib.init_board()


def branching_factor(hive):
    start_time = datetime.datetime.now()

    total_nodes = 0

    n_samples = 500
    n_moves = 100
    data_store = [[] for _ in range(n_moves)]

    for move in range(n_moves):
        hive.generate_moves()
        n_children = hive.node.contents.board.contents.move_location_tracker

        data_store[move].append(n_children)

        total_nodes += n_children
        if n_children == 0:
            # Terminal state
            break

        random_child = random.randint(0, n_children - 1)

        for i, child in enumerate(hive.children()):
            if i == random_child:
                lib.list_remove(byref(child.contents.node))
                lib.node_free(hive.node)
                hive.node = child
                break

    # hive.reinitialize()

    elapsed = (datetime.datetime.now() - start_time).total_seconds()
    print("Generated %d nodes in %.2f seconds (%.2f knodes/s)" % (total_nodes, elapsed, total_nodes / 1000 / elapsed))

    import matplotlib.pyplot as plt

    moves = [i for i in range(n_moves)]
    data = [sum(data_store[i]) / n_samples for i in moves]

    plt.plot(moves, data)
    plt.title("Trend of available moves per turn. Played by a random agent.")
    plt.xlabel("Turn")
    plt.ylabel("Avg. Available Moves (over 500 samples)")
    plt.show()


def performance_factor(hive):

    total_nodes = 0

    n_samples = 20
    n_moves = 100
    data_store = [[] for _ in range(n_moves)]
    for sample in range(n_samples):
        print(sample)
        for move in range(n_moves):
            # Do performance testing (generate all nodes to depth N)
            start_time = datetime.datetime.now()
            nodes = lib.performance_testing(hive.node, 2)
            elapsed = (datetime.datetime.now() - start_time).total_seconds()
            data_store[move].append(nodes / 1000 / elapsed)

            hive.generate_moves()
            n_children = hive.node.contents.board.contents.move_location_tracker

            total_nodes += n_children
            if n_children == 0:
                # Terminal state
                break

            random_child = random.randint(0, n_children - 1)

            for i, child in enumerate(hive.children()):
                if i == random_child:
                    lib.list_remove(byref(child.contents.node))
                    lib.node_free(hive.node)
                    hive.node = child
                    break

        hive.reinitialize()

    import matplotlib.pyplot as plt

    moves = [i for i in range(n_moves)]
    data = [sum(data_store[i]) / n_samples for i in moves]

    plt.plot(moves, data)
    plt.title("Move generation performance per turn.")
    plt.xlabel("Turn")
    plt.ylim(bottom=1)
    plt.ylabel("knodes/second (over %d samples)" % n_samples)
    plt.show()


def test(hive):
    for i in range(10):
        hive.generate_moves()
        if i == 9:
            for child in hive.children():
                lib.print_board(child.contents.board)

        # select child
        for child in hive.children():
            # detach child
            lib.list_remove(byref(child.contents.node))
            # free parent
            lib.node_free(hive.node)
            hive.node = child
            break


def to_matrix(hive):
    lib.print_matrix(hive.node.contents.board)


def generate_graphs(hive):
    n_moves = 100
    for move in range(n_moves):
        to_matrix(hive)

        # Do performance testing (generate all nodes to depth N)
        hive.generate_moves()
        n_children = hive.node.contents.board.contents.move_location_tracker

        if n_children == 0:
            # Terminal state
            break

        random_child = random.randint(0, n_children - 1)
        for i, child in enumerate(hive.children()):
            if i == random_child:
                lib.list_remove(byref(child.contents.node))
                lib.node_free(hive.node)
                hive.node = child
                break

    hive.reinitialize()


def main():
    hive = Hive()

    # branching_factor(hive)

    # performance_factor(hive)
    # test(hive)

    generate_graphs(hive)


if __name__ == "__main__":
    main()
