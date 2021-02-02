//
// Created by duncan on 01-02-21.
//

#include <stdlib.h>
#include <stdio.h>
#include "moves.h"
#include "board.h"

/*
 * Adds a potential move to the tracker inside the board struct.
 */
void add_move(struct board *board, int location, int type, int previous_location) {
    board->move_locations[board->move_location_tracker].move = type;
    board->move_locations[board->move_location_tracker].previous_location = previous_location;
    board->move_locations[board->move_location_tracker].location = location;
    board->move_location_tracker++;
}

/*
 * Returns an array containing array indices around a given x,y coordinate.
 */
int *get_points_around(int y, int x) {
    int *points = malloc(6 * sizeof(int));
    points[0] = (y - 1) * BOARD_SIZE + (x + 0);
    points[1] = (y - 1) * BOARD_SIZE + (x - 1);
    points[2] = (y + 0) * BOARD_SIZE + (x - 1);
    points[3] = (y + 0) * BOARD_SIZE + (x + 1);
    points[4] = (y + 1) * BOARD_SIZE + (x + 0);
    points[5] = (y + 1) * BOARD_SIZE + (x + 1);
    return points;
}

/*
 * Generates the placing moves, only allowed to place next to allied tiles.
 */
void generate_placing_moves(struct board *board, int type) {
    int color = type & COLOR_MASK;

    if (board->turn == 0) {
        return add_move(board, BOARD_SIZE + 1, type, 0);
    }
    if (board->turn == 1) {
        return add_move(board, BOARD_SIZE + 2, type, 0);
    }

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            // Skip empty tiles
            if (board->tiles[y * BOARD_SIZE + x].type == NONE) continue;

            // Get points around this point
            int *points = get_points_around(y, x);
            for (int p = 0; p < 6; p++) {
                int index = points[p];
                // Check if its empty
                if (board->tiles[index].type != NONE) continue;

                // Check for all neighbours of this point if its the same colour as the colour of the
                //  passed tile.
                int invalid = 0;
                int yy = index / BOARD_SIZE;
                int xx = index % BOARD_SIZE;
                int *neighbour_points = get_points_around(yy, xx);
                for (int np = 0; np < 6; np++) {
                    int np_index = neighbour_points[np];
                    if (board->tiles[np_index].type == NONE) continue;

                    // Check if every tile around it has the same colour as the passed tile colour.
                    if ((board->tiles[np_index].type & COLOR_MASK) != color) {
                        invalid = 1;
                        break;
                    }
                }
                free(neighbour_points);

                // If any neighbour of this point is another colour, check another point.
                if (invalid) continue;

                // Check if this move is already tracked.
                int tracked = 0;
                for (int n = 0; n < board->move_location_tracker; n++) {
                    if (board->move_locations[n].location == index) {
                        tracked = 1;
                        break;
                    }
                }
                if (!tracked) {
                    add_move(board, index, type, 0);
                }
            }
            free(points);
        }
    }
}

int add_if_unique(int *array, int n, int value) {
    for (int i = 0; i < n; i++) {
        if (array[i] == value) return 0;
    }
    array[n] = value;
    return 1;
}


int count_connected(struct board *board, int index) {
    int *connected = calloc(BOARD_SIZE * BOARD_SIZE, sizeof(int));
    int n_connected = 0;
    int *frontier = calloc(BOARD_SIZE * BOARD_SIZE, sizeof(int));
    int frontier_p = 0; // Frontier pointer.

    connected[n_connected++] = index;
    frontier[frontier_p++] = index;

    while (frontier_p != 0) {
        int i = frontier[--frontier_p];
        int y = i / BOARD_SIZE;
        int x = i % BOARD_SIZE;

        int *points = get_points_around(y, x);

        for (int n = 0; n < 6; n++) {
            // Skip this tile if theres nothing on it
            if (board->tiles[points[n]].type == NONE) continue;

            // If a unique tile is added, increment the tracker.
            int added = add_if_unique(connected, n_connected, points[n]);
            n_connected += added;

            if (added) {
                frontier[frontier_p++] = points[n];
            }
        }

        free(points);
    }
    return n_connected;
}

int sum_hive_tiles(struct board *board) {
    return N_TILES * 2 - (
            board->player1.queens_left +
            board->player1.grasshoppers_left +
            board->player1.beetles_left +
            board->player1.ants_left +
            board->player1.spiders_left +
            board->player2.spiders_left +
            board->player2.ants_left +
            board->player2.queens_left +
            board->player2.grasshoppers_left +
            board->player2.beetles_left);
}

void generate_free_moves(struct board *board) {
    int n_hive_tiles = sum_hive_tiles(board) - 1;

    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            struct tile *tile = &board->tiles[y * BOARD_SIZE + x];
            if (tile->type == NONE) continue;

            // Store the tile type for later use
            int tile_type = tile->type;
            tile->type = NONE;

            // Check if the points around this point consist of a single spanning tree.
            int valid = 1;
            int *points = get_points_around(y, x);
            for (int p = 1; p < 6; p++) {
                int index = points[p];

                if (board->tiles[index].type == NONE) continue;

                // Count how many tiles are connected.
                int connect_count = count_connected(board, index);
                if (connect_count != n_hive_tiles) {
                    valid = 0;
                    break;
                }
            }

            // Restore original tile value.
            tile->type = tile_type;

            if (valid) {
//                printf("%d %d is valid\n", x, y);
                // If this tile can be removed without breaking the hive, add it to the valid moves list.
                // TODO: Actually generate moves.
                add_move(board, y * BOARD_SIZE + x, tile->type, y * BOARD_SIZE + x);
            }

        }
    }
}
