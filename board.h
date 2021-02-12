//
// Created by duncan on 01-02-21.
//

#ifndef THEHIVE_BOARD_H
#define THEHIVE_BOARD_H

// Amount of tiles available per player.
#include "list.h"

#define N_ANTS 3
#define N_GRASSHOPPERS 3
#define N_BEETLES 2
#define N_SPIDERS 2
#define N_QUEENS 1
#define N_TILES (N_ANTS+N_GRASSHOPPERS+N_BEETLES+N_SPIDERS+N_QUEENS)

#define COLOR_SHIFT 5
#define LIGHT (0 << COLOR_SHIFT)
#define DARK (1 << COLOR_SHIFT)
#define COLOR_MASK (1 << COLOR_SHIFT)
#define TILE_MASK ((1 << COLOR_SHIFT) - 1)

#define NONE 0
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

struct move_location {
    int move;
    int location;
    int previous_location;
};

struct tile_stack {
    unsigned char type;
    int location;
    unsigned char z;
};

struct tile {
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
struct board {
    struct tile tiles[BOARD_SIZE * BOARD_SIZE * sizeof(struct tile)];
    int turn;  // Use this to derive whose turn it is

    struct player players[2];

    int queen1_position;
    int queen2_position;

    int n_stacked;
    struct tile_stack stack[TILE_STACK_SIZE];

    int move_location_tracker;
};

void print_board(struct board* board);

struct board* init_board();

void translate_board(struct board* board);
int finished_board(struct board* board);

#endif //THEHIVE_BOARD_H
