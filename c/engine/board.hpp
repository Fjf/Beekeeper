//
// Created by duncan on 01-02-21.
//

#ifndef THEHIVE_BOARD_H
#define THEHIVE_BOARD_H

// Amount of tiles available per player.
#include "list.hpp"
#include "moves.hpp"

#define N_ANTS 3
#define N_GRASSHOPPERS 3
#define N_BEETLES 2
#define N_SPIDERS 2
#define N_QUEENS 1
#define N_TILES (N_ANTS+N_GRASSHOPPERS+N_BEETLES+N_SPIDERS+N_QUEENS)



/*
 * Tile format (8 bits);
 * [ NN C TTTTT ]
 *
 * N = Number
 * C = Colour
 * T = Tile
 *
 * Number is 1-3, to define what the iteration is of the tile.
 * Colour is 0-1, specifying light or dark.
 * Tile is one of the 5 possible tiles as specified above.
 */

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

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))


struct tile_stack {
    unsigned char type;
    int location;
    unsigned char z;
};

struct tile {
    bool free;
    unsigned char type;
};

struct player {
    unsigned char beetles_left;
    unsigned char grasshoppers_left;
    unsigned char queens_left;
    unsigned char ants_left;
    unsigned char spiders_left;
};

/*
 *  All the tiles fit in a diagonal square
 *
 *          X X X X
 *         X X   X
 *        X   X X
 *       X X X X
 */

// Add a padding around the board to simplify edge conditions
#define BOARD_PADDING 2
#define BOARD_SIZE ((N_TILES * 2) + BOARD_PADDING * 2)

#include <Eigen/Dense>

typedef Eigen::Matrix<float, 22, 22> LapMatrix;
typedef Eigen::Matrix<std::complex<float>, 22, 1> LapEigen;
struct board {
    struct tile tiles[BOARD_SIZE * BOARD_SIZE];
    int turn;  // Use this to derive whose turn it is

    struct player players[2];

    int queen1_position;
    int queen2_position;

    int min_x, min_y;
    int max_x, max_y;

    char n_stacked;
    struct tile_stack stack[TILE_STACK_SIZE];

    LapMatrix laplacian_matrix;

    int move_location_tracker;
    bool done;
};

struct board_history_entry {
    struct tile tiles[BOARD_SIZE * BOARD_SIZE];
    struct tile_stack stack[TILE_STACK_SIZE];
    char repeats;
    struct list node;
};

extern struct list board_history;

extern void print_board(struct board* board);
extern struct board* init_board();

extern void get_min_x_y(struct board* board, int* min_x, int* min_y);
extern void get_max_x_y(struct board* board, int* max_x, int* max_y);
extern void translate_board(struct board* board);
extern void translate_board_22(struct board* board);
extern int finished_board(struct board* board);
extern void print_adj_matrix(struct board* board);

#endif //THEHIVE_BOARD_H
