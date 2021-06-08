//
// Created by duncan on 18-02-21.
//

#include <stdlib.h>
#include <board.h>


struct eval_multi evaluation_multipliers = {
        .movement = 7.28f,
        .queen = 0.25f,
        .used_tiles = 9.5f
};

float unused_tiles(struct node* node) {
    int value = 0;
    struct player* p = &node->board->players[1];
    value += p->queens_left + p->spiders_left + p->ants_left + p->grasshoppers_left + p->beetles_left;
    p = &node->board->players[0];
    value -= p->queens_left + p->spiders_left + p->ants_left + p->grasshoppers_left + p->beetles_left;
    return (float)value;
}


float relative_tile_value(unsigned char tile) {
    float value = 1.f;
    if ((tile & TILE_MASK) == L_ANT) {
        value = 2.f;
    }
    if ((tile & TILE_MASK) == L_QUEEN) {
        value = 3.f;
    }
    return value;
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
                float inc = relative_tile_value(tile);
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
        float inc = relative_tile_value(tile);
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
                float inc = relative_tile_value(tile);
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
        float inc = relative_tile_value(tile);
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
                queen_count--;
        }
    }

    if (node->board->dark_queen_position != -1) {
        int x2 = node->board->dark_queen_position % BOARD_SIZE;
        int y2 = node->board->dark_queen_position / BOARD_SIZE;
        int* points = get_points_around(y2, x2);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 2, the value increases by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                queen_count++;
        }
    }
    value += queen_count * evaluation_multipliers.queen;

    data->mm_value = value;
    return false;
}