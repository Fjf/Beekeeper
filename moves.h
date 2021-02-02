//
// Created by duncan on 01-02-21.
//

#ifndef THEHIVE_MOVES_H
#define THEHIVE_MOVES_H

#include "board.h"

int *get_points_around(int y, int x);
void generate_placing_moves(struct board *board, int type);
void generate_free_moves(struct board* board);

#endif //THEHIVE_MOVES_H
