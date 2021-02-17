from ctypes import *

f = "/home/duncan/CLionProjects/TheHive/c/cmake-build-debug/libhive.so"
lib = CDLL(f)
board_size = c_uint.in_dll(lib, "pboardsize").value
tile_stack_size = c_uint.in_dll(lib, "ptilestacksize").value


class Tile(Structure):
    _fields_ = [('type', c_ubyte)]


class Player(Structure):
    _fields_ = [
        ('beetles_left', c_ubyte),
        ('grasshoppers_left', c_ubyte),
        ('queens_left', c_ubyte),
        ('ants_left', c_ubyte),
        ('spiders_left', c_ubyte)
    ]


class TileStack(Structure):
    _fields_ = [
        ('type', c_ubyte),
        ('location', c_int),
        ('z', c_ubyte)
    ]


class Board(Structure):
    _fields_ = [
        ('tiles', Tile * board_size * board_size),
        ('turn', c_int),
        ('players', Player * 2),
        ('queen1_position', c_int),
        ('queen2_position', c_int),
        ('n_stacked', c_int),
        ('stack', TileStack * tile_stack_size),
        ('n_stacked', c_int),
        ('move_location_tracker', c_int)
    ]

    def __str__(self):
        return lib.print_board(self)


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


class Node(Structure):
    _fields_ = [
        ('children', List),
        ('node', List),
        ('board', POINTER(Board)),
        ('data', POINTER(MMData))
    ]


def main():
    lib.game_init.restype = POINTER(Node)

    node = lib.game_init()
    for i in range(100):
        lib.minimax(pointer(node))

        lib.print_board(node.contents.board)

        won = lib.finished_board(node.contents.board)
        if won:
            print("Player %d won!" % won)
            break


if __name__ == "__main__":
    main()
