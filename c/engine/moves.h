//
// Created by duncan on 01-02-21.
//

#ifndef THEHIVE_MOVES_H
#define THEHIVE_MOVES_H

#include "board.h"
#include "node.h"
#include "../mm/mm.h"
#include <time.h>

#define to_usec(timespec) ((((timespec).tv_sec * 1e9) + (timespec).tv_nsec) / 1e3)

int sum_hive_tiles(struct board *board);
void add_child(struct node *node, int location, int type, int previous_location);
void get_points_around(int y, int x, int* points);
void generate_placing_moves(struct node *node, int type);
void generate_free_moves(struct node* node, int player_bit);
void generate_moves(struct node *node);
bool can_move(struct board* board, int x, int y);

#endif //THEHIVE_MOVES_H
