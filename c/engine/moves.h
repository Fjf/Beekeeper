//
// Created by duncan on 01-02-21.
//

#ifndef THEHIVE_MOVES_H
#define THEHIVE_MOVES_H

#include "board.h"
#include "node.h"
#include "../mm/mm.h"
#include <time.h>

#define MOVE_NO_ANTS 1 << 0

#define to_usec(timespec) ((((timespec).tv_sec * 1e9) + (timespec).tv_nsec) / 1e3)

int sum_hive_tiles(struct board *board);
int* get_points_around(int y, int x);
void initialize_points_around();
void add_child(struct node *node, int location, int type, int previous_location);
void generate_placing_moves(struct node *node, int type);
void generate_free_moves(struct node *node, int player_bit, int flags);
void generate_moves(struct node *node, int flags);

void find_articulation(struct board *board, int idx, int parent);
void articulation(struct board* board, int index);

bool can_move(struct board* board, int x, int y);
void full_update(struct board *board);
void update_can_move(struct board *board, int location, int previous_location);

struct node* default_add_child(struct node* node, struct board* board);
struct node* default_init();

struct node *(*dedicated_add_child)(struct node *node, struct board *board);
struct node *(*dedicated_init)();

#endif //THEHIVE_MOVES_H
