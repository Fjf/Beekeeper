//
// Created by duncan on 01-02-21.
//

#include "board.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/*
 * This is used for the python link.
 */
unsigned int pboardsize = BOARD_SIZE;
unsigned int ptilestacksize = TILE_STACK_SIZE;
unsigned int pmaxturns = MAX_TURNS;

/*
 * Initialize the board.
 */
struct board *init_board() {
    struct board *board = calloc(1, sizeof(struct board));
    board->turn = 0;
    board->n_children = 0;
    board->n_stacked = 0;
    board->light_queen_position = -1;
    board->dark_queen_position = -1;

    for (int i = 0; i < 2; i++) {
        board->players[i].ants_left = N_ANTS;
        board->players[i].beetles_left = N_BEETLES;
        board->players[i].queens_left = N_QUEENS;
        board->players[i].grasshoppers_left = N_GRASSHOPPERS;
        board->players[i].spiders_left = N_SPIDERS;
    }

    board->min_x = board->min_y = BOARD_SIZE;
    board->max_x = board->max_y = 0;

    board->zobrist_hash = 0;

    memset(&board->stack, -1, TILE_STACK_SIZE * sizeof(struct tile_stack));

    board->has_updated = 0;

    return board;
}


void get_min_x_y(struct board *board, int *min_x, int *min_y) {
    int lower_x = *min_x;
    int lower_y = *min_y;
    *min_x = *min_y = BOARD_SIZE;
    for (int y = lower_y; y < BOARD_SIZE; y++) {
        for (int x = lower_x; x < *min_x; x++) {
            uchar tile = board->tiles[y * BOARD_SIZE + x];
            if (tile == EMPTY) continue;

            // Set min_y to the first non-empty tile row
            if (*min_y == BOARD_SIZE) *min_y = y;

            if (x < *min_x) *min_x = x;
        }
    }
}

void get_max_x_y(struct board *board, int *max_x, int *max_y) {
    int upper_y = *max_y;
    int upper_x = *max_x;
    *max_x = *max_y = 0;
    for (int y = upper_y; y >= 0; y--) {
        for (int x = upper_x; x >= *max_x; x--) {
            uchar tile = board->tiles[y * BOARD_SIZE + x];
            if (tile == EMPTY) continue;

            // Set min_y to the first non-empty tile row
            if (*max_y == 0) *max_y = y;

            if (x > *max_x) *max_x = x;
        }
    }
}




void force_set_bounds(struct board *board) {
    board->min_y = board->min_x = board->max_x = board->max_y = -1;
    // Set min Y
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board->tiles[y * BOARD_SIZE + x] != EMPTY) {
                board->min_y = y;
                break;
            }
        }
        if (board->min_y != -1) break;
    }

    // Set max Y
    for (int y = BOARD_SIZE - 1; y >= 0; y--) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            if (board->tiles[y * BOARD_SIZE + x] != EMPTY) {
                board->max_y = y;
                break;
            }
        }
        if (board->max_y != -1) break;
    }
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (board->tiles[y * BOARD_SIZE + x] != EMPTY) {
                board->min_x = x;
                break;
            }
        }
        if (board->min_x != -1) break;
    }
    for (int x = BOARD_SIZE - 1; x >= 0; x--) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            if (board->tiles[y * BOARD_SIZE + x] != EMPTY) {
                board->max_x = x;
                break;
            }
        }
        if (board->max_x != -1) break;
    }
}

/*
 * Translates the board to the center coordinate space
 */
void translate_board(struct board *board) {
    int offset = board->min_y * BOARD_SIZE + board->min_x;
    int moffset = board->max_y * BOARD_SIZE + board->max_x;
    int size = (moffset - offset) + 1;

    int to_x = (BOARD_SIZE / 2) - (board->max_x - board->min_x + 1) / 2;
    int to_y = (BOARD_SIZE / 2) - (board->max_y - board->min_y + 1) / 2;

    // Move all the tile tracking structs the same amount as the rest of the board.
    int translate_offset = (to_y * BOARD_SIZE + to_x) - offset;
    if (board->light_queen_position != -1)
        board->light_queen_position += translate_offset;
    if (board->dark_queen_position != -1)
        board->dark_queen_position += translate_offset;
    for (int i = 0; i < TILE_STACK_SIZE; i++) {
        if (board->stack[i].location != -1) {
            board->stack[i].location += translate_offset;
        }
    }

    // Copy data into temp array
    char t[BOARD_SIZE * BOARD_SIZE] = {0};
    char *temp = (void*) &t;

    memcpy(temp + (to_y * BOARD_SIZE + to_x) * sizeof(char),
           ((char *) &board->tiles) + offset * sizeof(char),
           (size) * sizeof(char)
    );

    memset(&board->tiles, 0, BOARD_SIZE * BOARD_SIZE * sizeof(char));
    // Copy data back into main array after clearing data.
    memcpy(&board->tiles, temp, BOARD_SIZE * BOARD_SIZE * sizeof(char));

    int xdiff = to_x - board->min_x;
    int ydiff = to_y - board->min_y;

    board->min_x += xdiff;
    board->max_x += xdiff;
    board->min_y += ydiff;
    board->max_y += ydiff;
}


/*
 * Translates the board to 2,2 coordinate space
 */
void translate_board_22(struct board *board) {
    int min_x = BOARD_SIZE;
    int min_y = BOARD_SIZE;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < min_x; x++) {
            uchar tile = board->tiles[y * BOARD_SIZE + x];
            if (tile == EMPTY) continue;

            // Set min_y to the first non-empty tile row
            if (min_y == BOARD_SIZE) min_y = y;

            if (min_x > x) min_x = x;
        }
    }

    int offset = min_y * BOARD_SIZE + min_x;
    int size = (BOARD_SIZE * BOARD_SIZE - offset);

    // Move all the tile tracking structs the same amount as the rest of the board.
    int translate_offset = (2 * BOARD_SIZE + 2) - offset;
    if (board->light_queen_position != -1)
        board->light_queen_position += translate_offset;
    if (board->dark_queen_position != -1)
        board->dark_queen_position += translate_offset;
    for (int i = 0; i < TILE_STACK_SIZE; i++) {
        if (board->stack[i].location != -1) {
            board->stack[i].location += translate_offset;
        }
    }

    // Copy data into temp array
    char t[BOARD_SIZE * BOARD_SIZE] = {0};
    char *temp = (void*) &t;

    memcpy(temp + (2 * BOARD_SIZE + 2) * sizeof(char),
           ((char *) &board->tiles) + offset * sizeof(char),
           (size - (2 * BOARD_SIZE + 2)) * sizeof(char)
    );

    memset(&board->tiles, 0, BOARD_SIZE * BOARD_SIZE * sizeof(char));
    // Copy data back into main array after clearing data.
    memcpy(&board->tiles, temp, BOARD_SIZE * BOARD_SIZE * sizeof(char));
}


int count_tiles_around(struct board *board, int position) {
    int x = position % BOARD_SIZE;
    int y = position / BOARD_SIZE;
    int count = 0;
    int *points = get_points_around(y, x);
    for (int p = 0; p < 6; p++) {
        if (board->tiles[points[p]] != EMPTY) {
            count++;
        }
    }
    return count;
}


bool is_surrounded(struct board *board, int y, int x) {
    int p;
    int *points = get_points_around(y, x);
    for (p = 0; p < 6; p++) {
        if (board->tiles[points[p]] == EMPTY) {
            break;
        }
    }
    // If it has gone through all tiles, it is completely surrounded.
    if (p == 6) return true;
    return false;
}

/*
 * Checks if the board is in a finished position
 * Meaning; either one of the two queens is completely surrounded.
 * Returns 1 if player 1 won, 2 if player 2 won, 0 if nobody won yet.
 * Or 3 if its a draw by force (both queens surrounded), or 4 by repetition.
 */
int finished_board(struct board *board) {
    int x, y;
    int res = 0;
    if (board->light_queen_position != -1) {
        // Check queen 1
        x = board->light_queen_position % BOARD_SIZE;
        y = board->light_queen_position / BOARD_SIZE;
        if (is_surrounded(board, y, x)) {
            res = 2;
        }
    }

    if (board->dark_queen_position != -1) {
        // Check queen 2
        x = board->dark_queen_position % BOARD_SIZE;
        y = board->dark_queen_position / BOARD_SIZE;
        if (is_surrounded(board, y, x)) {
            if (res == 0)
                res = 1;
            else
                res = 3;
        }
    }

    // Check draw by repetition.
    int sum = 0;
    for (int i = 0; i < board->turn; i++) {
        if (board->hash_history[i] == board->zobrist_hash) sum++;

        if (sum == 3) res = 3;
    }

    // Draw due to turn limit
    if (board->turn >= MAX_TURNS - 2) {
        int l = count_tiles_around(board, board->light_queen_position);
        int d = count_tiles_around(board, board->dark_queen_position);
        return d > l ? 1 : 2;
    }

    return res;
}

/*
 * Prints the given Hive board to standard output.
 * Mainly for testing purposes
 */
void print_board(struct board *board) {
    for (int x = 0; x < BOARD_SIZE; x++) {
        printf("  ");
    }
    for (int x = 0; x < BOARD_SIZE; x++) {
        printf("---");
    }
    printf("\n");

    for (int y = 0; y < BOARD_SIZE; y++) {
        // Add spaces padding for the display
        for (int x = 0; x < BOARD_SIZE - y - 1; x++) {
            printf("  ");
        }
        printf("/");

        for (int x = 0; x < BOARD_SIZE; x++) {
            printf(" ");

            uchar tile = board->tiles[y * BOARD_SIZE + x];
            int type = tile & (COLOR_MASK | TILE_MASK);
            int n = (tile & NUMBER_MASK) >> NUMBER_SHIFT;
            if (type == EMPTY) {
                printf(" ");
            } else if (type == L_ANT) {
                printf("A");
            } else if (type == L_GRASSHOPPER) {
                printf("G");
            } else if (type == L_BEETLE) {
                printf("B");
            } else if (type == L_SPIDER) {
                printf("S");
            } else if (type == L_QUEEN) {
                printf("Q");
            } else if (type == D_ANT) {
                printf("a");
            } else if (type == D_GRASSHOPPER) {
                printf("g");
            } else if (type == D_BEETLE) {
                printf("b");
            } else if (type == D_SPIDER) {
                printf("s");
            } else if (type == D_QUEEN) {
                printf("q");
            }

            // Add number after tile
            if (n > 0) {
//                n = board->tiles[y * BOARD_SIZE + x].free;
                printf("%d", n);
            } else {
                printf(" ");
            }
        }

        printf("/\n");
    }
    printf(" ");
    for (int x = 0; x < BOARD_SIZE; x++) {
        printf("---");
    }
    printf("\n");
}

void print_matrix(struct board* board) {
    printf("---------\n");
    for (int i = 0; i < BOARD_SIZE - 0; i++) {
        for (int j = 0; j < BOARD_SIZE - 0; j++) {
            char t = board->tiles[i * BOARD_SIZE + j];
            printf("%d ", t & TILE_MASK);
        }
        printf("\n");
    }
}