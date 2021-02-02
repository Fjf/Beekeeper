#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "board.h"
#include "moves.h"


void do_turn(struct board *board, int move_location_index, struct player *player) {
    int idx = board->move_locations[move_location_index].location;
    int type = board->move_locations[move_location_index].move;
    board->tiles[idx].type = type;
    board->move_location_tracker = 0;
    board->turn++;

    // Reduce the amount of available tiles per player
    int masked_type = type & TILE_MASK;
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


    translate_board(board);
}

int main() {
    srand(time(NULL));

    struct board *board = init_board();
    struct player *player;
    int player_bit;
    for (int i = 0; i < 22; i++) {
        // Select player based on turn.
        if (board->turn % 2 == 0) {
            player = &board->player1;
            player_bit = 0;
        } else {
            player = &board->player2;
            player_bit = BLACK;
        }

        printf("%d %d %d %d %d\n",
               player->spiders_left,
               player->ants_left,
               player->grasshoppers_left,
               player->beetles_left,
               player->queens_left
        );

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



        int selected = rand() % board->move_location_tracker;
        do_turn(board, selected, player);
        print_board(board);
    }

    generate_free_moves(board);
    print_board(board);
//    do {
//
//    } while (!finished_board(board));


    free(board);
    return 0;
}
