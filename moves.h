//
// Created by duncan on 01-02-21.
//

#ifndef THEHIVE_MOVES_H
#define THEHIVE_MOVES_H

#include "board.h"
#include "pn_tree.h"
#include <time.h>

#define to_usec(timespec) ((((timespec).tv_sec * 1e9) + (timespec).tv_nsec) / 1e3)

void add_child(struct pn_node *node, int location, int type, int previous_location);
void get_points_around(int y, int x, int* points);
void generate_placing_moves(struct pn_node *node, int type);
void generate_free_moves(struct pn_node* node, int player_bit);
void generate_moves(struct pn_node *node, int player_idx);

#endif //THEHIVE_MOVES_H
