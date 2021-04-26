//
// Created by duncan on 25-03-21.
//

#ifndef HIVE_MCTS_H
#define HIVE_MCTS_H

#include <time.h>
#include <stdbool.h>
#include "utils.h"

struct mcts_data {
    unsigned int p0;
    unsigned int p1;
    unsigned int draw;
    bool keep;
};

struct node *mcts_init();

struct node *mcts_add_child(struct node *node, struct board *board);


void mcts(struct node **tree, struct player_arguments *args);
int mcts_playout(struct node *root, time_t end_time);

#endif //HIVE_MCTS_H
