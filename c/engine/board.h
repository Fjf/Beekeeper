//
// Created by duncan on 01-02-21.
//

#ifndef THEHIVE_BOARD_H
#define THEHIVE_BOARD_H

// Amount of tiles available per player.
#include "list.h"
#include "moves.h"
#include "utils.h"

#define N_ANTS 3
#define N_GRASSHOPPERS 3
#define N_BEETLES 2
#define N_SPIDERS 2
#define N_QUEENS 1
#define N_UNIQUE_TILES 5
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

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

// Tile stack location is -1 if there is no tile in the stack.
#pragma pack(1)
struct tile_stack {
    unsigned char type;
    int location;
    unsigned char z;
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
 *          X X X X X
 *         X X     X
 *        X   X   X
 *       X     X X
 *      X X X X X
 */

// Add a padding around the board to simplify edge conditions
#define BOARD_PADDING 2
#define BOARD_SIZE ((N_TILES * 2) + BOARD_PADDING * 2)

// Default MAX_TURNS of 150
#ifndef MAX_TURNS
#define MAX_TURNS 150
#endif

struct board {
    uchar tiles[BOARD_SIZE * BOARD_SIZE];
    bool free[BOARD_SIZE * BOARD_SIZE];
    int turn;  // Use this to derive whose turn it is

    struct player players[2];

    int light_queen_position;
    int dark_queen_position;

    int min_x, min_y;
    int max_x, max_y;

    char n_stacked;
    struct tile_stack stack[TILE_STACK_SIZE];

    int n_children;

    long long zobrist_hash;
    long long hash_history[MAX_TURNS + 1];

    bool has_updated;
};


void print_board(struct board* board);
void print_matrix(struct board* board);
struct board* init_board();

void get_min_x_y(struct board* board, int* min_x, int* min_y);
void get_max_x_y(struct board* board, int* max_x, int* max_y);
int count_tiles_around(struct board* board, int position);
void translate_board(struct board* board);
void translate_board_22(struct board* board);
int finished_board(struct board* board);

#endif //THEHIVE_BOARD_H
