//
// Created by duncan on 01-02-21.
//

#include "board.h"
#include "moves.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Initialize the board.
 */
struct board *init_board() {
    struct board *board = calloc(1, sizeof(struct board));
    board->turn = 0;
    board->move_location_tracker = 0;

    board->player1.ants_left = N_ANTS;
    board->player1.beetles_left = N_BEETLES;
    board->player1.queens_left = N_QUEENS;
    board->player1.grasshoppers_left = N_GRASSHOPPERS;
    board->player1.spiders_left = N_SPIDERS;

    memcpy(&board->player2, &board->player1, sizeof(struct player));

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
            if (tile.type == NONE) continue;

            // Set min_y to the first non-empty tile row
            if (min_y == BOARD_SIZE) min_y = y;

            if (min_x > x) min_x = x;
        }
    }

    int offset = min_y * BOARD_SIZE + min_x;
    int size = (BOARD_SIZE * BOARD_SIZE - offset);

    // Copy data into temp array
    void *temp = calloc(BOARD_SIZE * BOARD_SIZE, sizeof(struct tile));

    memcpy(temp + (2 * BOARD_SIZE + 2) * sizeof(struct tile),
           ((void*)&board->tiles) + offset * sizeof(struct tile),
           (size - (2 * BOARD_SIZE + 2)) * sizeof(struct tile)
    );

    memset(&board->tiles, 0, BOARD_SIZE * BOARD_SIZE * sizeof(struct tile));
    // Copy data back into main array after clearing data.
    memcpy(&board->tiles, temp, BOARD_SIZE * BOARD_SIZE * sizeof(struct tile));

    free(temp);
}


/*
 * Checks if the board is in a finished position
 * Meaning; either one of the two queens is completely surrounded.
 */
int finished_board(struct board *board) {
    int n_encountered_queens = 0;
    int p;

    // Iterate the board, and check if any queen is surrounded.
    // There is a maximum of N_QUEENS * 2 total queens.
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            // If the tile is a queen, check if all the tiles around this are not empty.
            if (board->tiles[(y * BOARD_SIZE) + x].type == W_QUEEN
                || board->tiles[(y * BOARD_SIZE) + x].type == B_QUEEN) {
                n_encountered_queens++;
                int *points = get_points_around(y, x);
                for (p = 0; p < 6; p++) {
                    if (board->tiles[points[p]].type == NONE) {
                        break;
                    }
                }
                free(points);
                // If it has gone through all tiles, it is completely surrounded.
                if (p == 6) return 1;
            }

            if (n_encountered_queens == N_QUEENS * 2) {
                return 0;
            }
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
        printf(" ");
    }
    for (int x = 0; x < BOARD_SIZE; x++) {
        printf("--");
    }
    printf("\n");

    for (int y = 0; y < BOARD_SIZE; y++) {
        // Add spaces padding for the display
        for (int x = 0; x < BOARD_SIZE - y - 1; x++) {
            printf(" ");
        }
        printf("/");

        for (int x = 0; x < BOARD_SIZE; x++) {
            printf(" ");

            struct tile tile = board->tiles[y * BOARD_SIZE + x];

            char pre_move = 0;
            if (board->move_location_tracker != 0) {
                for (int n = 0; n < board->move_location_tracker; n++) {
                    if (board->move_locations[n].location == y * BOARD_SIZE + x) {
                        pre_move = 1;
                        break;
                    }
                }
            }

            if (pre_move) {
                printf(".");
            } else if (tile.type == NONE) {
                printf(" ");
            } else if (tile.type == W_ANT) {
                printf("A");
            } else if (tile.type == W_GRASSHOPPER) {
                printf("G");
            } else if (tile.type == W_BEETLE) {
                printf("B");
            } else if (tile.type == W_SPIDER) {
                printf("S");
            } else if (tile.type == W_QUEEN) {
                printf("Q");
            } else if (tile.type == B_ANT) {
                printf("a");
            } else if (tile.type == B_GRASSHOPPER) {
                printf("g");
            } else if (tile.type == B_BEETLE) {
                printf("b");
            } else if (tile.type == B_SPIDER) {
                printf("s");
            } else if (tile.type == B_QUEEN) {
                printf("q");
            }
        }

        printf("/\n");
    }
    printf(" ");
    for (int x = 0; x < BOARD_SIZE; x++) {
        printf("--");
    }
    printf("\n");
}

