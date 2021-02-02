#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "board.h"
#include "moves.h"




void do_turn(struct board *board, int move_location_index, struct player *player) {
    int idx = board->move_locations[move_location_index].location;
    int type = board->move_locations[move_location_index].move;
    int previous_location = board->move_locations[move_location_index].previous_location;
    int masked_type = type & TILE_MASK;


    // Reduce the amount of available tiles per player
    // Only do this if the tile was not moved but instead newly placed
    if (previous_location == -1) {
        if (masked_type == W_QUEEN)
            player->queens_left--;
        else if (masked_type == W_BEETLE)
            player->beetles_left--;
        else if (masked_type == W_GRASSHOPPER)
            player->grasshoppers_left--;
        else if (masked_type == W_ANT)
            player->ants_left--;
        else if (masked_type == W_SPIDER)
            player->spiders_left--;
    } else {
        // Remove piece from previous location
        board->tiles[previous_location].type = NONE;

        // Check if there is a tile in the stack
        // If this move is a beetle, check the tile stack to know if the beetle
        //  was on top of another piece
        if (board->tiles[previous_location].next != NULL) {
            // Copy data into this location.
            struct tile* tile = board->tiles[previous_location].next;
            memcpy(&board->tiles[previous_location], tile, sizeof(struct tile));
            free(tile);
        }
    }


    // If this move is on top of an existing tile, store this tile in a trackable struct
    if (board->tiles[idx].type != NONE) {
        struct tile* tile = malloc(sizeof(struct tile));
        memcpy(tile, &board->tiles[idx], sizeof(struct tile));
        board->tiles[idx].next = tile;
    }

    // Do this move.
    board->tiles[idx].type = type;
    board->move_location_tracker = 0;
    board->turn++;

    translate_board(board);
}

void test_beetle(struct board *board) {
    add_move(board, BOARD_SIZE + 1, W_BEETLE, -1);
    do_turn(board, 0, &board->player1);
    add_move(board, BOARD_SIZE + 2, B_BEETLE, -1);
    do_turn(board, 0, &board->player2);
    add_move(board, BOARD_SIZE + 3, W_BEETLE, -1);
    do_turn(board, 0, &board->player1);
    add_move(board, (2 * BOARD_SIZE) + 2, B_BEETLE, -1);
    do_turn(board, 0, &board->player2);
    print_board(board);

    add_move(board, BOARD_SIZE + 2, W_BEETLE, BOARD_SIZE + 3);
    do_turn(board, 0, &board->player1);

    print_board(board);

    add_move(board, (2 * BOARD_SIZE) + 3, B_BEETLE, (2 * BOARD_SIZE) + 2);
    do_turn(board, 0, &board->player2);

    print_board(board);

    add_move(board, BOARD_SIZE + 3, W_BEETLE, BOARD_SIZE + 2);
    do_turn(board, 0, &board->player1);

    print_board(board);
}

int main() {
    srand(time(NULL));

    struct board *board = init_board();
    struct player *player;
    int player_bit;
    for (int i = 0; i < 22; i++) {
        printf("Move %d\n", i);
        // Select player based on turn.
        if (board->turn % 2 == 0) {
            player = &board->player1;
            player_bit = 0;
        } else {
            player = &board->player2;
            player_bit = BLACK;
        }

        if (player->spiders_left > 0)
            generate_placing_moves(board, W_SPIDER | player_bit);
        if (player->beetles_left > 0)
            generate_placing_moves(board, W_BEETLE | player_bit);
        if (player->grasshoppers_left > 0)
            generate_placing_moves(board, W_GRASSHOPPER | player_bit);
        if (player->ants_left > 0)
            generate_placing_moves(board, W_ANT | player_bit);
        if (player->queens_left > 0)
            generate_placing_moves(board, W_QUEEN | player_bit);


        printf("Generated placing moves\n");
        int selected = rand() % board->move_location_tracker;
        do_turn(board, selected, player);
        print_board(board);
    }

    generate_free_moves(board);

    print_board(board);
//    test_beetle(board);


    free(board);
    return 0;
}
