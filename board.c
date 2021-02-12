//
// Created by duncan on 01-02-21.
//

#include "board.h"
#include "moves.h"
#include "list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


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
    void *temp = calloc(BOARD_SIZE * BOARD_SIZE, sizeof(struct tile));

    memcpy(temp + (2 * BOARD_SIZE + 2) * sizeof(struct tile),
           ((void*)&board->tiles) + offset * sizeof(struct tile),
           (size - (2 * BOARD_SIZE + 2)) * sizeof(struct tile)
    );

    memset(&board->tiles, 0, BOARD_SIZE * BOARD_SIZE * sizeof(struct tile));
    // Copy data back into main array after clearing data.
    memcpy(&board->tiles, temp, BOARD_SIZE * BOARD_SIZE * sizeof(struct tile));

    free(temp);

//    int min_x = -1, min_y = -1;
//    int max_x = -1, max_y = -1;
//
//    for (int y = 0; y < BOARD_SIZE; y++) {
//        for (int x = 0; x < BOARD_SIZE; x++) {
//            if (board->tiles[y * BOARD_SIZE + x].type != EMPTY) {
//                min_y = y;
//                break;
//            }
//        }
//        if (min_y != -1) break;
//    }
//    for (int y = BOARD_SIZE - 1; y >= 0; y--) {
//        for (int x = 0; x < BOARD_SIZE; x++) {
//            if (board->tiles[y * BOARD_SIZE + x].type != EMPTY) {
//                max_y = y;
//                break;
//            }
//        }
//        if (max_y != -1) break;
//    }
//    for (int x = 0; x < BOARD_SIZE; x++) {
//        for (int y = 0; y < BOARD_SIZE; y++) {
//            if (board->tiles[y * BOARD_SIZE + x].type != EMPTY) {
//                min_x = x;
//                break;
//            }
//        }
//        if (min_x != -1) break;
//    }
//    for (int x = BOARD_SIZE - 1; x >= 0; x--) {
//        for (int y = 0; y < BOARD_SIZE; y++) {
//            if (board->tiles[y * BOARD_SIZE + x].type != EMPTY) {
//                max_x = x;
//                break;
//            }
//        }
//        if (max_x != -1) break;
//    }
//
//    // Check if tiles are close to the bounds
//    if (max_x < BOARD_SIZE - BOARD_PADDING
//     && max_y < BOARD_SIZE - BOARD_PADDING
//     && min_x > BOARD_PADDING
//     && min_y > BOARD_PADDING) {
//        return;
//    }
//
//    int width = max_x - min_x;
//    int height = max_y - min_y;
//
//    int center = BOARD_SIZE / 2;
//    int write_offset = (center - height / 2) * BOARD_SIZE + (center - width / 2);
//    int read_offset = min_y * BOARD_SIZE + min_x;
//    // If desired width is more toward the width, offset is negative
//    int write_size = (BOARD_SIZE * BOARD_SIZE - write_offset);
//    int read_size = (BOARD_SIZE * BOARD_SIZE - read_offset);
//    int size = MIN(write_size, read_size);
//
//    int wx = write_offset % BOARD_SIZE;
//    int wy = write_offset / BOARD_SIZE;
//    int x = wx - min_x;
//    int y = wy - min_y;
//    int offset = y * BOARD_SIZE + x;
//
//    // Move all the tile tracking structs the same amount as the rest of the board.
//    if (board->queen1_position != -1)
//        board->queen1_position += offset;
//    if (board->queen2_position != -1)
//        board->queen2_position += offset;
//    for (int i = 0; i < TILE_STACK_SIZE; i++) {
//        if (board->stack[i].location != -1) {
//            board->stack[i].location += offset;
//        }
//    }
//
//    // Copy data into temp array
//    void *temp = calloc(BOARD_SIZE * BOARD_SIZE, sizeof(struct tile));
//
//    memcpy(temp + write_offset * sizeof(struct tile),
//           ((void*)&board->tiles) + read_offset * sizeof(struct tile),
//           size * sizeof(struct tile)
//    );
//
//    memset(&board->tiles, 0, BOARD_SIZE * BOARD_SIZE * sizeof(struct tile));
//    // Copy data back into main array after clearing data.
//    memcpy(&board->tiles, temp, BOARD_SIZE * BOARD_SIZE * sizeof(struct tile));
//
//    free(temp);
}

bool is_surrounded(struct board* board, int y, int x) {
    int p;
    int* points = malloc(6 * sizeof(int));
    get_points_around(y, x, points);
    for (p = 0; p < 6; p++) {
        if (board->tiles[points[p]].type == EMPTY) {
            break;
        }
    }
    free(points);
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

    if (board->queen1_position != -1) {
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

            if (tile.type == EMPTY) {
                printf(" ");
            } else if (tile.type == L_ANT) {
                printf("A");
            } else if (tile.type == L_GRASSHOPPER) {
                printf("G");
            } else if (tile.type == L_BEETLE) {
                printf("B");
            } else if (tile.type == L_SPIDER) {
                printf("S");
            } else if (tile.type == L_QUEEN) {
                printf("Q");
            } else if (tile.type == D_ANT) {
                printf("a");
            } else if (tile.type == D_GRASSHOPPER) {
                printf("g");
            } else if (tile.type == D_BEETLE) {
                printf("b");
            } else if (tile.type == D_SPIDER) {
                printf("s");
            } else if (tile.type == D_QUEEN) {
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

