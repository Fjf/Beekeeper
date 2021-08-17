//
// Created by duncan on 18-02-21.
//

#include <stdlib.h>
#include <board.h>
#include <stdio.h>
#include <math.h>

#ifndef MOVEMENT_REST
#define MOVEMENT_REST 7.28f
#endif
#ifndef QUEEN_REST
#define QUEEN_REST 3.25f
#endif
#ifndef USED_TILES_REST
#define USED_TILES_REST 9.5f
#endif

// Global defines.
struct eval_multi evaluation_multipliers = {
        .movement = 7.28f,
        .queen = 3.25f,
        .used_tiles = 9.5f,
        .distance_to_queen = 1.0f
};
bool (*mm_evaluate)(struct node*);

struct eval_multi dtq_multipliers = {
        .movement = 0.25f,
        .queen = 1.35f,
        .used_tiles = 1.49f,
        .distance_to_queen = 5.32f
};


float unused_tiles(struct node* node) {
    float value = 0;
    struct player* p = &node->board->players[1];
    value += (float) (p->ants_left + p->queens_left + p->spiders_left + p->grasshoppers_left + p->beetles_left);

    p = &node->board->players[0];
    value -= (float) (p->ants_left + p->queens_left + p->spiders_left + p->grasshoppers_left + p->beetles_left);
    return value;
}


float mvt(unsigned char tile) {
    float v = 1.f;
    if ((tile & TILE_MASK) == L_ANT) {
        v = 2.f;
    }
    if ((tile & TILE_MASK) == L_QUEEN) {
        v = 3.f;
    }
    return v;
}


bool mm_evaluate_expqueen(struct node* node) {
    struct mm_data* data = node->data;

    // We want to have this information.
    update_can_move(node->board, node->move.location, node->move.previous_location);


#ifdef TESTING
    float value = 0.;
#else
    float value = 0.f + (float)(rand() % 100) / 1000.f;
#endif

    int won = finished_board(node->board);
    if (won == 1) {
        data->mm_value = MM_INFINITY - (float)node->board->turn;
        return true;
    } if (won == 2) {
        data->mm_value = -MM_INFINITY + (float)node->board->turn;
        return true;
    } if (won == 3) {
        data->mm_value = 0;
        return true;
    }

    value += unused_tiles(node) * 8.87622930462319f;

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


    float free_counter = 0.f;
    for (int x = lx; x < hx; x++) {
        if (n_encountered == to_encounter) break;
        for (int y = ly; y < hy; y++) {
            if (n_encountered == to_encounter) break;
            unsigned char tile = node->board->tiles[y * BOARD_SIZE + x].type;
            if (tile == EMPTY) continue;
            n_encountered++;

            if (!node->board->tiles[y * BOARD_SIZE + x].free) {
                float inc = mvt(tile);
                if ((tile & COLOR_MASK) == LIGHT) {
                    free_counter -= inc;
                } else {
                    free_counter += inc;
                }
            }
        }
    }

    // Count the tiles on the stack too.
    for (int i = 0; i < node->board->n_stacked; i++) {
        struct tile_stack* ts = &node->board->stack[i];
        unsigned char tile = ts->type;
        float inc = mvt(tile);
        if ((tile & COLOR_MASK) == LIGHT) {
            free_counter -= inc;
        } else {
            free_counter += inc;
        }
    }

    float movement_multiplier = 8.493793292308318f;
    value += free_counter * movement_multiplier;

    if (node->board->light_queen_position != -1) {
        int x1 = node->board->light_queen_position % BOARD_SIZE;
        int y1 = node->board->light_queen_position / BOARD_SIZE;
        int* points = get_points_around(y1, x1);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 1, the value drops by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                value -=  0.05295699310799809f;
        }
    }

    if (node->board->dark_queen_position != -1) {
        int x2 = node->board->dark_queen_position % BOARD_SIZE;
        int y2 = node->board->dark_queen_position / BOARD_SIZE;
        int* points = get_points_around(y2, x2);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 2, the value increases by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                value +=  0.05295699310799809f;
        }
    }

    data->mm_value = value;
    return false;
}

bool mm_evaluate_variable(struct node* node) {
    struct mm_data* data = node->data;

    // We want to have this information.
    update_can_move(node->board, node->move.location, node->move.previous_location);

#ifdef TESTING
    float value = 0.;
#else
    float value = 0.f + (float)(rand() % 100) / 1000.f;
#endif

    int won = finished_board(node->board);
    if (won == 1) {
        data->mm_value = MM_INFINITY - (float)node->board->turn;
        return true;
    }
    if (won == 2) {
        data->mm_value = -MM_INFINITY + (float)node->board->turn;
        return true;
    }
    if (won == 3) {
        data->mm_value = 0;
        return true;
    }


    value += unused_tiles(node) * evaluation_multipliers.used_tiles;

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

    float free_counter = 0.f;
    for (int x = lx; x < hx; x++) {
        if (n_encountered == to_encounter) break;
        for (int y = ly; y < hy; y++) {
            if (n_encountered == to_encounter) break;
            unsigned char tile = node->board->tiles[y * BOARD_SIZE + x].type;
            if (tile == EMPTY) continue;
            n_encountered++;

            if (!node->board->tiles[y * BOARD_SIZE + x].free) {
                float inc = 1;
                if ((tile & COLOR_MASK) == LIGHT) {
                    free_counter -= inc;
                } else {
                    free_counter += inc;
                }
            }
        }
    }

    // Count the tiles on the stack too.
    for (int i = 0; i < node->board->n_stacked; i++) {
        struct tile_stack* ts = &node->board->stack[i];
        unsigned char tile = ts->type;
        float inc = 1;
        if ((tile & COLOR_MASK) == LIGHT) {
            free_counter -= inc;
        } else {
            free_counter += inc;
        }
    }

    value += free_counter * evaluation_multipliers.movement;

    float queen_count = 0;
    if (node->board->light_queen_position != -1) {
        int x1 = node->board->light_queen_position % BOARD_SIZE;
        int y1 = node->board->light_queen_position / BOARD_SIZE;
        int* points = get_points_around(y1, x1);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 1, the value drops by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                queen_count -= 1;
        }
    }

    if (node->board->dark_queen_position != -1) {
        int x2 = node->board->dark_queen_position % BOARD_SIZE;
        int y2 = node->board->dark_queen_position / BOARD_SIZE;
        int* points = get_points_around(y2, x2);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 2, the value increases by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                queen_count += 1;
        }
    }
    value += queen_count * evaluation_multipliers.queen;

    data->mm_value = value;
    return false;
}


float distance_to_queen(struct board *board, int position, int color) {
    float dtq = 0.f;
    int ax = position % BOARD_SIZE;
    int ay = position / BOARD_SIZE;
    for (int y = board->min_y; y < board->max_y; y++) {
        for (int x = board->min_x; x < board->max_x; x++) {
            int pos = y * BOARD_SIZE + x;
            int tile_type = board->tiles[pos].type;
            if (tile_type == EMPTY || (tile_type & COLOR_MASK) != color) continue;

            int dx = ax - x;
            int dy = ay - y;
            dtq += (float)(dx * dx + dy * dy);
        }
    }
    return dtq;
}

bool mm_evaluate_distance(struct node* node) {
    struct mm_data* data = node->data;

    // We want to have this information.
    update_can_move(node->board, node->move.location, node->move.previous_location);

#ifdef TESTING
    float value = 0.;
#else
    float value = 0.f + (float)(rand() % 100) / 1000.f;
#endif

    int won = finished_board(node->board);
    if (won == 1) {
        data->mm_value = MM_INFINITY - (float)node->board->turn;
        return true;
    }
    if (won == 2) {
        data->mm_value = -MM_INFINITY + (float)node->board->turn;
        return true;
    }
    if (won == 3) {
        data->mm_value = 0;
        return true;
    }


    value += unused_tiles(node) * dtq_multipliers.used_tiles;

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

    float free_counter = 0.f;
    for (int x = lx; x < hx; x++) {
        if (n_encountered == to_encounter) break;
        for (int y = ly; y < hy; y++) {
            if (n_encountered == to_encounter) break;
            unsigned char tile = node->board->tiles[y * BOARD_SIZE + x].type;
            if (tile == EMPTY) continue;
            n_encountered++;

            if (!node->board->tiles[y * BOARD_SIZE + x].free) {
                float inc = 1;
                if ((tile & COLOR_MASK) == LIGHT) {
                    free_counter -= inc;
                } else {
                    free_counter += inc;
                }
            }
        }
    }

    // Count the tiles on the stack too.
    for (int i = 0; i < node->board->n_stacked; i++) {
        struct tile_stack* ts = &node->board->stack[i];
        unsigned char tile = ts->type;
        float inc = 1;
        if ((tile & COLOR_MASK) == LIGHT) {
            free_counter -= inc;
        } else {
            free_counter += inc;
        }
    }

    value += free_counter * dtq_multipliers.movement;

    float queen_count = 0;
    if (node->board->light_queen_position != -1) {
        int x1 = node->board->light_queen_position % BOARD_SIZE;
        int y1 = node->board->light_queen_position / BOARD_SIZE;
        int* points = get_points_around(y1, x1);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 1, the value drops by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                queen_count -= 1;
        }
    }

    if (node->board->dark_queen_position != -1) {
        int x2 = node->board->dark_queen_position % BOARD_SIZE;
        int y2 = node->board->dark_queen_position / BOARD_SIZE;
        int* points = get_points_around(y2, x2);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 2, the value increases by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                queen_count += 1;
        }
    }
    value += queen_count * dtq_multipliers.queen;

    float dtql = distance_to_queen(node->board, node->board->light_queen_position, DARK);
    float dtqd = distance_to_queen(node->board, node->board->dark_queen_position, LIGHT);
    value += (dtql - dtqd) * dtq_multipliers.distance_to_queen;

    data->mm_value = value;
    return false;
}