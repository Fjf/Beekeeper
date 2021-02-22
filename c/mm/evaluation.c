//
// Created by duncan on 18-02-21.
//

#include <stdlib.h>
#include <board.h>
#include <math.h>
#include "evaluation.h"
#include "../timing/timing.h"

bool mm_evaluate_expqueen(struct node* node) {
    timing("mm_evaluate", TIMING_START);
    struct mm_data* data = node->data;
    double value = 0. + (float)(rand() % 100) / 100;
    int points[6];

    int won = finished_board(node->board);
    if (won == 1) {
        data->mm_value = MM_INFINITY;
        data->mm_evaluated = true;
        return true;
    }
    if (won == 2) {
        data->mm_value = -MM_INFINITY;
        data->mm_evaluated = true;
        return true;
    }


    int n_encountered = 0;
    int to_encounter = sum_hive_tiles(node->board);
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            unsigned char tile = node->board->tiles[y * BOARD_SIZE + x].type;
            if (tile == EMPTY) continue;

            if (!node->board->tiles[y * BOARD_SIZE + x].free) {
                float inc = 3.f;
                if ((tile & TILE_MASK) == L_QUEEN) {
                    inc = 6.f;
                }
                if ((tile & COLOR_MASK) == LIGHT) {
                    value -= inc;
                } else {
                    value += inc;
                }
            }
            n_encountered++;
            if (n_encountered == to_encounter) break;
        }
        if (n_encountered == to_encounter) break;
    }

    double queen_sum = -2.;
    if (node->board->queen1_position != -1) {
        int x1 = node->board->queen1_position % BOARD_SIZE;
        int y1 = node->board->queen1_position / BOARD_SIZE;
        get_points_around(y1, x1, points);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 1, the value drops by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                queen_sum *= 2;
        }
    }
    value += queen_sum;

    queen_sum = 2.;
    if (node->board->queen2_position != -1) {
        int x2 = node->board->queen2_position % BOARD_SIZE;
        int y2 = node->board->queen2_position / BOARD_SIZE;
        get_points_around(y2, x2, points);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 2, the value increases by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                queen_sum *= 2;
        }
    }
    value += queen_sum;

    data->mm_value = value;
    data->mm_evaluated = true;
    timing("mm_evaluate", TIMING_END);
    return false;
}

bool mm_evaluate_linqueen(struct node* node) {
    timing("mm_evaluate", TIMING_START);
    struct mm_data* data = node->data;
    double value = 0. + (float)(rand() % 100) / 100;
    int points[6];

    int won = finished_board(node->board);
    if (won == 1) {
        data->mm_value = MM_INFINITY;
        data->mm_evaluated = true;
        return true;
    }
    if (won == 2) {
        data->mm_value = -MM_INFINITY;
        data->mm_evaluated = true;
        return true;
    }


    int n_encountered = 0;
    int to_encounter = sum_hive_tiles(node->board);
    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            unsigned char tile = node->board->tiles[y * BOARD_SIZE + x].type;
            if (tile == EMPTY) continue;

            if (!node->board->tiles[y * BOARD_SIZE + x].free) {
                float inc = 3.f;
                if ((tile & TILE_MASK) == L_BEETLE
                    || (tile & TILE_MASK) == L_SPIDER) {
                    inc = 3.f;
                }
                if ((tile & COLOR_MASK) == LIGHT) {
                    value -= inc;
                } else {
                    value += inc;
                }
            }
            n_encountered++;
            if (n_encountered == to_encounter) break;
        }
        if (n_encountered == to_encounter) break;
    }

    if (node->board->queen1_position != -1) {
        int x1 = node->board->queen1_position % BOARD_SIZE;
        int y1 = node->board->queen1_position / BOARD_SIZE;
        get_points_around(y1, x1, points);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 1, the value drops by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                value -= 20;
        }
    }

    if (node->board->queen2_position != -1) {
        int x2 = node->board->queen2_position % BOARD_SIZE;
        int y2 = node->board->queen2_position / BOARD_SIZE;
        get_points_around(y2, x2, points);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 2, the value increases by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                value += 20;
        }
    }

    data->mm_value = value;
    data->mm_evaluated = true;
    timing("mm_evaluate", TIMING_END);
    return false;
}

bool mm_evaluate_noblock(struct node* node) {
    timing("mm_evaluate", TIMING_START);
    struct mm_data* data = node->data;
    double value = 0. + (float)(rand() % 100) / 100;
    int points[6];

    int won = finished_board(node->board);
    if (won == 1) {
        data->mm_value = MM_INFINITY;
        data->mm_evaluated = true;
        return true;
    }
    if (won == 2) {
        data->mm_value = -MM_INFINITY;
        data->mm_evaluated = true;
        return true;
    }


    double queen_sum = -2.;
    if (node->board->queen1_position != -1) {
        int x1 = node->board->queen1_position % BOARD_SIZE;
        int y1 = node->board->queen1_position / BOARD_SIZE;
        get_points_around(y1, x1, points);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 1, the value drops by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                queen_sum *= 2;
        }
    }
    value += queen_sum;

    queen_sum = 2.;
    if (node->board->queen2_position != -1) {
        int x2 = node->board->queen2_position % BOARD_SIZE;
        int y2 = node->board->queen2_position / BOARD_SIZE;
        get_points_around(y2, x2, points);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 2, the value increases by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                queen_sum *= 2;
        }
    }
    value += queen_sum;

    data->mm_value = value;
    data->mm_evaluated = true;
    timing("mm_evaluate", TIMING_END);
    return false;
}