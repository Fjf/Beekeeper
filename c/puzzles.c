//
// Created by duncan on 08-04-21.
//

#include <board.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "puzzles.h"

#define make_tile(tile, number) ((tile) | ((number) << NUMBER_SHIFT))

void set_board_information(struct board* board) {
    // Set correct min x and y
    board->min_x = board->min_y = 0;
    board->max_x = board->max_y = BOARD_SIZE - 1;
    get_min_x_y(board, &board->min_x, &board->min_y);
    get_max_x_y(board, &board->max_x, &board->max_y);

    for (int i = board->min_x; i < board->max_x + 1; i++) {
        for (int j = board->min_y; j < board->max_y + 1; j++) {
            int idx = j * BOARD_SIZE + i;
            if (board->tiles[idx].type == EMPTY)
                continue;
            int color = board->tiles[idx].type & COLOR_MASK;
            int type = board->tiles[idx].type & TILE_MASK;
            int player = color >> COLOR_SHIFT;

            if (type == L_GRASSHOPPER)
                board->players[player].grasshoppers_left--;
            if (type == L_ANT)
                board->players[player].ants_left--;
            if (type == L_BEETLE)
                board->players[player].beetles_left--;
            if (type == L_QUEEN) {
                board->players[player].queens_left--;
                if (player == 0)
                    board->light_queen_position = idx;
                else
                    board->dark_queen_position = idx;
            }
            if (type == L_SPIDER)
                board->players[player].spiders_left--;
        }
    }

    // Count for stacked tiles too
    for (int i = 0; i < board->n_stacked; i++) {
        struct tile_stack ts = board->stack[i];
        int color = ts.type & COLOR_MASK;
        int type = ts.type & TILE_MASK;
        int player = color >> COLOR_SHIFT;

        printf("%d %d is in stack\n", color, type);

        if (type == L_GRASSHOPPER)
            board->players[player].grasshoppers_left--;
        if (type == L_ANT)
            board->players[player].ants_left--;
        if (type == L_BEETLE)
            board->players[player].beetles_left--;
        if (type == L_QUEEN) {
            board->players[player].queens_left--;
            if (player == 0)
                board->light_queen_position = ts.location;
            else
                board->dark_queen_position = ts.location;
        }
        if (type == L_SPIDER)
            board->players[player].spiders_left--;
    }
}

void puzzle_4(struct node* tree) {
    int white_to_move = 1;

    struct board* board = tree->board;

    board->turn = white_to_move + 9;

    board->tiles[0 * BOARD_SIZE + 3].type = make_tile(L_GRASSHOPPER, 1);
    board->tiles[0 * BOARD_SIZE + 4].type = make_tile(L_ANT, 2);

    board->tiles[1 * BOARD_SIZE + 0].type = make_tile(D_ANT, 2);
    board->tiles[1 * BOARD_SIZE + 1].type = make_tile(D_GRASSHOPPER, 2);
    board->tiles[1 * BOARD_SIZE + 3].type = make_tile(L_SPIDER, 2);
    board->tiles[1 * BOARD_SIZE + 4].type = make_tile(D_QUEEN, 1);
    board->tiles[1 * BOARD_SIZE + 5].type = make_tile(L_ANT, 1);

    board->tiles[2 * BOARD_SIZE + 2].type = make_tile(L_ANT, 3);
    board->tiles[2 * BOARD_SIZE + 3].type = make_tile(D_SPIDER, 1);
    board->tiles[2 * BOARD_SIZE + 5].type = make_tile(L_SPIDER, 1);

    board->tiles[3 * BOARD_SIZE + 2].type = make_tile(D_GRASSHOPPER, 1);
    board->tiles[3 * BOARD_SIZE + 3].type = make_tile(D_BEETLE, 2);
    board->tiles[3 * BOARD_SIZE + 5].type = make_tile(D_BEETLE, 1);
    board->stack[0].location = 3 * BOARD_SIZE + 3;
    board->stack[0].z = 0;
    board->stack[0].type = make_tile(L_QUEEN, 1);
    board->stack[1].location = 3 * BOARD_SIZE + 3;
    board->stack[1].z = 1;
    board->stack[1].type = make_tile(L_BEETLE, 1);

    board->tiles[4 * BOARD_SIZE + 3].type = make_tile(D_ANT, 1);

    board->n_stacked = 2;
}

void puzzle_5(struct node* tree) {
    int white_to_move = 0;

    struct board* board = tree->board;

    board->turn = white_to_move + 9;

    board->tiles[0 * BOARD_SIZE + 4].type = make_tile(L_BEETLE, 1);
    board->stack[0].location = 0 * BOARD_SIZE + 4;
    board->stack[0].z = 0;
    board->stack[0].type = make_tile(D_GRASSHOPPER, 2);
    board->tiles[0 * BOARD_SIZE + 5].type = make_tile(D_SPIDER, 2);

    board->tiles[1 * BOARD_SIZE + 0].type = make_tile(D_SPIDER, 1);
    board->tiles[1 * BOARD_SIZE + 1].type = make_tile(D_ANT, 1);
    board->tiles[1 * BOARD_SIZE + 2].type = make_tile(L_ANT, 1);
    board->tiles[1 * BOARD_SIZE + 4].type = make_tile(D_QUEEN, 1);

    board->tiles[2 * BOARD_SIZE + 1].type = make_tile(D_BEETLE, 2);
    board->stack[1].location = 2 * BOARD_SIZE + 1;
    board->stack[1].z = 0;
    board->stack[1].type = make_tile(L_QUEEN, 1);
    board->tiles[2 * BOARD_SIZE + 3].type = make_tile(D_GRASSHOPPER, 1);
    board->tiles[2 * BOARD_SIZE + 4].type = make_tile(L_GRASSHOPPER, 1);

    board->tiles[3 * BOARD_SIZE + 1].type = make_tile(D_ANT, 2);
    board->tiles[4 * BOARD_SIZE + 2].type = make_tile(D_BEETLE, 1);
    board->tiles[5 * BOARD_SIZE + 2].type = make_tile(L_ANT, 3);
    board->tiles[5 * BOARD_SIZE + 3].type = make_tile(L_ANT, 2);

    board->tiles[6 * BOARD_SIZE + 3].type = make_tile(D_ANT, 3);

    board->n_stacked = 2;
}


void puzzle_10(struct node* tree) {
    int white_to_move = 0;

    struct board* board = tree->board;

    board->turn = white_to_move + 9;

    board->tiles[0 * BOARD_SIZE + 1].type = make_tile(L_SPIDER, 1);
    board->tiles[0 * BOARD_SIZE + 3].type = make_tile(L_GRASSHOPPER, 3);
    board->tiles[0 * BOARD_SIZE + 4].type = make_tile(D_ANT, 1);
    board->tiles[0 * BOARD_SIZE + 5].type = make_tile(L_GRASSHOPPER, 2);

    board->tiles[1 * BOARD_SIZE + 0].type = make_tile(L_SPIDER, 2);
    board->tiles[1 * BOARD_SIZE + 2].type = make_tile(D_SPIDER, 1);
    board->tiles[1 * BOARD_SIZE + 3].type = make_tile(D_ANT, 2);
    board->tiles[1 * BOARD_SIZE + 6].type = make_tile(L_GRASSHOPPER, 1);

    board->tiles[2 * BOARD_SIZE + 1].type = make_tile(D_GRASSHOPPER, 2);
    board->tiles[2 * BOARD_SIZE + 2].type = make_tile(L_ANT, 3);
    board->tiles[2 * BOARD_SIZE + 3].type = make_tile(L_QUEEN, 1);
    board->tiles[2 * BOARD_SIZE + 4].type = make_tile(L_BEETLE, 2);
    board->stack[0].location = 2 * BOARD_SIZE + 4;
    board->stack[0].z = 0;
    board->stack[0].type = make_tile(D_ANT, 3);
    board->stack[1].location = 2 * BOARD_SIZE + 4;
    board->stack[1].z = 1;
    board->stack[1].type = make_tile(D_BEETLE, 2);
    board->tiles[2 * BOARD_SIZE + 7].type = make_tile(L_ANT, 2);

    board->tiles[3 * BOARD_SIZE + 3].type = make_tile(L_BEETLE, 1);
    board->stack[2].location = 3 * BOARD_SIZE + 3;
    board->stack[2].z = 0;
    board->stack[2].type = make_tile(D_GRASSHOPPER, 1);
    board->stack[3].location = 3 * BOARD_SIZE + 3;
    board->stack[3].z = 1;
    board->stack[3].type = make_tile(D_BEETLE, 1);
    board->tiles[3 * BOARD_SIZE + 7].type = make_tile(L_ANT, 1);

    board->tiles[4 * BOARD_SIZE + 7].type = make_tile(D_QUEEN, 1);

    board->n_stacked = 4;
}

void puzzle_11(struct node* tree) {
    int white_to_move = 0;

    struct board* board = tree->board;

    board->turn = white_to_move + 9;

    board->tiles[3 * BOARD_SIZE + 3].type = make_tile(L_ANT, 1);
    board->tiles[1 * BOARD_SIZE + 1].type = make_tile(D_SPIDER, 2);
    board->tiles[3 * BOARD_SIZE + 4].type = make_tile(L_BEETLE, 2);
    board->tiles[4 * BOARD_SIZE + 5].type = make_tile(L_ANT, 3);
    board->tiles[5 * BOARD_SIZE + 6].type = make_tile(D_ANT, 1);
    board->tiles[2 * BOARD_SIZE + 3].type = make_tile(D_QUEEN, 1);
    board->tiles[1 * BOARD_SIZE + 2].type = make_tile(L_GRASSHOPPER, 2);
    board->tiles[2 * BOARD_SIZE + 4].type = make_tile(L_BEETLE, 1);
    board->tiles[1 * BOARD_SIZE + 3].type = make_tile(L_SPIDER, 1);
    board->tiles[3 * BOARD_SIZE + 6].type = make_tile(D_GRASSHOPPER, 2);
    board->tiles[2 * BOARD_SIZE + 5].type = make_tile(L_GRASSHOPPER, 3);
    board->tiles[0 * BOARD_SIZE + 3].type = make_tile(D_GRASSHOPPER, 1);
    board->tiles[3 * BOARD_SIZE + 2].type = make_tile(D_BEETLE, 1);
    board->stack[0].location = 3 * BOARD_SIZE + 2;
    board->stack[0].z = 0;
    board->stack[0].type = make_tile(L_GRASSHOPPER, 1);
    board->tiles[2 * BOARD_SIZE + 1].type = make_tile(L_QUEEN, 1);
    board->tiles[1 * BOARD_SIZE + 0].type = make_tile(D_GRASSHOPPER, 3);
    board->tiles[3 * BOARD_SIZE + 1].type = make_tile(D_ANT, 2);
    board->tiles[4 * BOARD_SIZE + 2].type = make_tile(L_ANT, 2);
    board->tiles[5 * BOARD_SIZE + 3].type = make_tile(D_SPIDER, 1);
    board->tiles[2 * BOARD_SIZE + 0].type = make_tile(D_BEETLE, 2);

    board->n_stacked = 1;
}

void puzzle_13(struct node* tree) {
    int white_to_move = 0;

    struct board* board = tree->board;

    board->turn = white_to_move + 9;
    board->tiles[4 * BOARD_SIZE + 4].type = make_tile(D_GRASSHOPPER, 1);
    board->tiles[5 * BOARD_SIZE + 5].type = make_tile(D_QUEEN, 1);
    board->tiles[6 * BOARD_SIZE + 6].type = make_tile(L_SPIDER, 2);
    board->tiles[2 * BOARD_SIZE + 2].type = make_tile(D_ANT, 3);
    board->tiles[1 * BOARD_SIZE + 1].type = make_tile(D_BEETLE, 1);
    board->tiles[0 * BOARD_SIZE + 0].type = make_tile(L_ANT, 3);
    board->tiles[3 * BOARD_SIZE + 4].type = make_tile(L_GRASSHOPPER, 1);
    board->tiles[5 * BOARD_SIZE + 6].type = make_tile(L_BEETLE, 2);
    board->stack[0].location = 5 * BOARD_SIZE + 6;
    board->stack[0].z = 0;
    board->stack[0].type = make_tile(L_BEETLE, 1);
    board->tiles[0 * BOARD_SIZE + 1].type = make_tile(L_ANT, 2);
    board->tiles[3 * BOARD_SIZE + 5].type = make_tile(L_SPIDER, 1);
    board->tiles[2 * BOARD_SIZE + 5].type = make_tile(D_ANT, 2);
    board->tiles[1 * BOARD_SIZE + 4].type = make_tile(D_SPIDER, 1);
    board->tiles[1 * BOARD_SIZE + 5].type = make_tile(L_GRASSHOPPER, 2);
    board->tiles[3 * BOARD_SIZE + 2].type = make_tile(D_BEETLE, 2);
    board->stack[1].location = 3 * BOARD_SIZE + 2;
    board->stack[1].z = 0;
    board->stack[1].type = make_tile(L_QUEEN, 1);
    board->tiles[4 * BOARD_SIZE + 3].type = make_tile(D_ANT, 1);
    board->tiles[2 * BOARD_SIZE + 1].type = make_tile(D_GRASSHOPPER, 2);
    board->tiles[4 * BOARD_SIZE + 2].type = make_tile(L_ANT, 1);
    board->tiles[4 * BOARD_SIZE + 1].type = make_tile(D_GRASSHOPPER, 3);

    board->n_stacked = 2;
}

void setup_puzzle(struct node** tree, int puzzle_number) {
    struct board* board = (*tree)->board;
    memset(board->tiles, 0, sizeof(board->tiles));
    board->zobrist_hash = 0;
    board->move_location_tracker = 0;

    if (puzzle_number == 4) {
        puzzle_4(*tree);
    } else if (puzzle_number == 5) {
        puzzle_5(*tree);
    } else if (puzzle_number == 10) {
        puzzle_10(*tree);
    } else if (puzzle_number == 11) {
        puzzle_11(*tree);
    } else {
        fprintf(stderr, "Invalid puzzle number supplied.");
        exit(1);
    }

    set_board_information(board);
    translate_board(board);
    full_update(board);
}