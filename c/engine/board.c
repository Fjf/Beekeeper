//
// Created by duncan on 01-02-21.
//

#include "board.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


unsigned int pboardsize = BOARD_SIZE;
unsigned int ptilestacksize = TILE_STACK_SIZE;

/*
 * Initialize the board.
 */
struct board *init_board() {
    struct board *board = calloc(1, sizeof(struct board));
    board->turn = 0;
    board->move_location_tracker = -1;
    board->n_stacked = 0;
    board->queen1_position = -1;
    board->queen2_position = -1;

    for (int i = 0; i < 2; i++) {
        board->players[i].ants_left = N_ANTS;
        board->players[i].beetles_left = N_BEETLES;
        board->players[i].queens_left = N_QUEENS;
        board->players[i].grasshoppers_left = N_GRASSHOPPERS;
        board->players[i].spiders_left = N_SPIDERS;
    }

    memset(&board->stack, -1, TILE_STACK_SIZE * sizeof(struct tile_stack));

    return board;
}

/*
 * Translates the board to 2,2 coordinate space
 */
void translate_board(struct board *board) {
    int min_x = BOARD_SIZE;
    int min_y = BOARD_SIZE;
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < min_x; x++) {
            struct tile tile = board->tiles[y * BOARD_SIZE + x];
            if (tile.type == EMPTY) continue;

            // Set min_y to the first non-empty tile row
            if (min_y == BOARD_SIZE) min_y = y;

            if (min_x > x) min_x = x;
        }
    }

    int offset = min_y * BOARD_SIZE + min_x;
    int size = (BOARD_SIZE * BOARD_SIZE - offset);

    // Move all the tile tracking structs the same amount as the rest of the board.
    int translate_offset = (2 * BOARD_SIZE + 2) - offset;
    if (board->queen1_position != -1)
        board->queen1_position += translate_offset;
    if (board->queen2_position != -1)
        board->queen2_position += translate_offset;
    for (int i = 0; i < TILE_STACK_SIZE; i++) {
        if (board->stack[i].location != -1) {
            board->stack[i].location += translate_offset;
        }
    }

    // Copy data into temp array
    struct tile t[BOARD_SIZE * BOARD_SIZE] = {0};
    void* temp = &t;
//    void *temp = calloc(BOARD_SIZE * BOARD_SIZE, sizeof(struct tile));

    memcpy(temp + (2 * BOARD_SIZE + 2) * sizeof(struct tile),
           ((void*)&board->tiles) + offset * sizeof(struct tile),
           (size - (2 * BOARD_SIZE + 2)) * sizeof(struct tile)
    );

    memset(&board->tiles, 0, BOARD_SIZE * BOARD_SIZE * sizeof(struct tile));
    // Copy data back into main array after clearing data.
    memcpy(&board->tiles, temp, BOARD_SIZE * BOARD_SIZE * sizeof(struct tile));

//    free(temp);
}

bool is_surrounded(struct board* board, int y, int x) {
    int p;
    int points[6];
    get_points_around(y, x, points);
    for (p = 0; p < 6; p++) {
        if (board->tiles[points[p]].type == EMPTY) {
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
 */
int finished_board(struct board *board) {
    int x, y;
    if (board->queen1_position != -1) {
        // Check queen 1
        x = board->queen1_position % BOARD_SIZE;
        y = board->queen1_position / BOARD_SIZE;
        if (is_surrounded(board, y, x)) {
            return 2;
        }
    }

    if (board->queen2_position != -1) {
        // Check queen 2
        x = board->queen2_position % BOARD_SIZE;
        y = board->queen2_position / BOARD_SIZE;
        if (is_surrounded(board, y, x)) {
            return 1;
        }
    }
    return 0;
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

            struct tile tile = board->tiles[y * BOARD_SIZE + x];
            unsigned char type = tile.type & (COLOR_MASK | TILE_MASK);
            unsigned char n = (tile.type & NUMBER_MASK) >> NUMBER_SHIFT;
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

