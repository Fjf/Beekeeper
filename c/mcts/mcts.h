//
// Created by duncan on 25-03-21.
//

#ifndef HIVE_MCTS_H
#define HIVE_MCTS_H

#include <time.h>
#include <stdbool.h>
#include "utils.h"

struct mcts_data {
    double value;
    uint n_sims;
    bool keep;
    float prio;
};

struct node *mcts_init();

struct node *mcts_add_child(struct node *node, struct board *board);


struct node* mcts(struct node *tree, struct player_arguments *args);
int mcts_playout(struct node *root, double end_time);
int mcts_playout_prio(struct node *root, double end_time);

#endif //HIVE_MCTS_H
