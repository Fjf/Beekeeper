//
// Created by duncan on 01-02-21.
//

#ifndef THEHIVE_BOARD_H
#define THEHIVE_BOARD_H

// Amount of tiles available per player.
#define N_ANTS 3
#define N_GRASSHOPPERS 3
#define N_BEETLES 2
#define N_SPIDERS 2
#define N_QUEENS 1
#define N_TILES (N_ANTS+N_GRASSHOPPERS+N_BEETLES+N_SPIDERS+N_QUEENS)

#define BLACK (1 << 5)
#define COLOR_MASK (1 << 5)
#define TILE_MASK ((1 << 5) - 1)

#define NONE 0
#define W_ANT 1
#define W_GRASSHOPPER 2
#define W_BEETLE 3
#define W_SPIDER 4
#define W_QUEEN 5
#define B_ANT (1 | BLACK)
#define B_GRASSHOPPER (2 | BLACK)
#define B_BEETLE (3 | BLACK)
#define B_SPIDER (4 | BLACK)
#define B_QUEEN (5 | BLACK)


struct move_location {
    int move;
    int location;
    int previous_location;
};

struct tile_stack {
    int type;
    int location;
    int z;
};

struct tile {
    int type;
    struct tile* next;
};

struct player {
    int beetles_left;
    int grasshoppers_left;
    int queens_left;
    int ants_left;
    int spiders_left;
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
#define BOARD_SIZE ((N_TILES * 2) + 4)
struct board {
    struct tile tiles[BOARD_SIZE * BOARD_SIZE * sizeof(struct tile)];
    char turn;  // Use this to derive whose turn it is
    char winner;

    struct player player1;
    struct player player2;

    int n_stacked;

    int move_location_tracker;
    struct move_location move_locations[BOARD_SIZE * BOARD_SIZE * sizeof(struct move_location)];
};

void print_board(struct board* board);
struct board* init_board();
void translate_board(struct board* board);
int finished_board(struct board* board);

#endif //THEHIVE_BOARD_H
