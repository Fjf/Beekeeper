//
// Created by duncan on 18-02-21.
//

#include <stdlib.h>
#include <board.h>
#include <math.h>
#include "evaluation.h"
#include "../timing/timing.h"





double unused_tiles(struct node* node) {
    double value = 0;
    struct player* p = &node->board->players[1];
    value += p->queens_left + p->spiders_left + p->ants_left + p->grasshoppers_left + p->beetles_left;
    p = &node->board->players[0];
    value -= p->queens_left + p->spiders_left + p->ants_left + p->grasshoppers_left + p->beetles_left;
    return value;
}

bool mm_evaluate_expqueen(struct node* node) {
    timing("mm_evaluate", TIMING_START);
    struct mm_data* data = node->data;

#ifdef TESTING
    double value = 0.;
#else
    double value = 0. + (float)(rand() % 100) / 1000;
#endif

    int won = finished_board(node->board);
    if (won == 1) {
        data->mm_value = MM_INFINITY - node->board->turn;
        data->mm_evaluated = true;
        return true;
    }
    if (won == 2) {
        data->mm_value = -MM_INFINITY + node->board->turn;
        data->mm_evaluated = true;
        return true;
    }
    if (won == 3) {
        data->mm_value = 0;
        data->mm_evaluated = true;
        return true;
    }

    value += unused_tiles(node);

    int n_encountered = 0;
    int to_encounter = sum_hive_tiles(node->board);

#ifdef CENTERED
    struct board* board = node->board;
    int ly = board->min_y, hy = board->max_y + 1;
    int lx = board->min_x, hx = board->max_x + 1;
#else
    int ly = 0, hy = BOARD_SIZE;
    int lx = 0, hx = BOARD_SIZE;
#endif

    for (int x = lx; x < hx; x++) {
        for (int y = ly; y < hy; y++) {
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
        int* points = get_points_around(y1, x1);
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
        int* points = get_points_around(y2, x2);
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

#ifdef TESTING
    double value = 0.;
#else
    double value = 0. + (float)(rand() % 100) / 1000;
#endif

    int won = finished_board(node->board);
    if (won == 1) {
        data->mm_value = MM_INFINITY - node->board->turn;
        data->mm_evaluated = true;
        return true;
    }
    if (won == 2) {
        data->mm_value = -MM_INFINITY + node->board->turn;
        data->mm_evaluated = true;
        return true;
    }
    if (won == 3) {
        data->mm_value = 0;
        data->mm_evaluated = true;
        return true;
    }

    int n_encountered = 0;
    int to_encounter = sum_hive_tiles(node->board);

#ifdef CENTERED
    struct board* board = node->board;
    int ly = board->min_y, hy = board->max_y + 1;
    int lx = board->min_x, hx = board->max_x + 1;
#else
    int ly = 0, hy = BOARD_SIZE;
    int lx = 0, hx = BOARD_SIZE;
#endif

    for (int x = lx; x < hx; x++) {
        for (int y = ly; y < hy; y++) {
            unsigned char tile = node->board->tiles[y * BOARD_SIZE + x].type;
            if (tile == EMPTY) continue;

            if (!node->board->tiles[y * BOARD_SIZE + x].free) {
                float inc = 3.f;
                if ((tile & TILE_MASK) == L_ANT) {
                    inc = 5.f;
                } else if ((tile & TILE_MASK) == L_QUEEN) {
                    inc = 7.f;
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
        int* points = get_points_around(y1, x1);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 1, the value drops by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                value -= 20;
        }
    }

    if (node->board->queen2_position != -1) {
        int x2 = node->board->queen2_position % BOARD_SIZE;
        int y2 = node->board->queen2_position / BOARD_SIZE;
        int* points = get_points_around(y2, x2);
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