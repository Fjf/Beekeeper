
#ifndef BEEKEEPER_CONSTANTS_H
#define BEEKEEPER_CONSTANTS_H


#define N_ANTS 3
#define N_GRASSHOPPERS 3
#define N_BEETLES 2
#define N_SPIDERS 2
#define N_QUEENS 1
#define N_UNIQUE_TILES 5
#define N_TILES (N_ANTS+N_GRASSHOPPERS+N_BEETLES+N_SPIDERS+N_QUEENS)

#define COLOR_SHIFT 5
#define NUMBER_SHIFT 6
#define LIGHT (0 << COLOR_SHIFT)
#define DARK (1 << COLOR_SHIFT)
#define COLOR_MASK (1 << COLOR_SHIFT)
#define TILE_MASK ((1 << COLOR_SHIFT) - 1)
#define NUMBER_MASK (3 << NUMBER_SHIFT)

#define EMPTY 0
#define L_ANT 1
#define L_GRASSHOPPER 2
#define L_BEETLE 3
#define L_SPIDER 4
#define L_QUEEN 5
#define D_ANT (1 | DARK)
#define D_GRASSHOPPER (2 | DARK)
#define D_BEETLE (3 | DARK)
#define D_SPIDER (4 | DARK)
#define D_QUEEN (5 | DARK)

#define TILE_STACK_SIZE (N_BEETLES * 2)
// Add a padding around the board to simplify edge conditions
#define BOARD_PADDING 2
#define BOARD_SIZE int8_t((N_TILES * 2) + BOARD_PADDING * 2)

#ifndef MAX_TURNS
#define MAX_TURNS 1000
#endif

#define UNDECIDED 0
#define LIGHT_WON 1
#define DARK_WON 2
#define DRAW 3


#define to_usec(timespec) ((((timespec).tv_sec * 1e9) + (timespec).tv_nsec) / 1e3)

#define ERR_NOMOVES 1
#define ERR_NOTIME 2
#define ERR_NOMEM 3

#endif //BEEKEEPER_CONSTANTS_H
