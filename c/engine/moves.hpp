//
// Created by duncan on 01-02-21.
//

#ifndef THEHIVE_MOVES_H
#define THEHIVE_MOVES_H

#include "board.hpp"
#include "node.hpp"
#include "../mm/mm.hpp"
#include <time.h>

#define to_usec(timespec) ((((timespec).tv_sec * 1e9) + (timespec).tv_nsec) / 1e3)
#define is_ok(code) { \
    int err = code;   \
    if (err != 0) return err; \
}

extern int sum_hive_tiles(struct board *board);
extern int* get_points_around(int y, int x);
extern void initialize_points_around();
extern void add_child(struct node *node, int location, int type, int previous_location);
extern void generate_placing_moves(struct node *node, int type);
extern void generate_free_moves(struct node* node, int player_bit);
extern void generate_moves(struct node *node);
extern bool can_move(struct board* board, int x, int y);

#endif //THEHIVE_MOVES_H
