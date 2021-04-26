//
// Created by duncan on 30-03-21.
//

#ifndef HIVE_UTILS_H
#define HIVE_UTILS_H

#include "node.h"


#define ALG_MM 0
#define ALG_MCTS 1
#define ALG_RANDOM 2
#define ALG_MANUAL 2
#define alg_to_str(in) ((in) == ALG_MM ? "Minimax" : \
                       ((in) == ALG_MCTS ? "MCTS" :  \
                       ((in) == ALG_RANDOM ? "Random" : \
                       ((in) == ALG_MANUAL ? "Manual" : "Unknown"))))

struct player_arguments {
    int algorithm;
    double mcts_constant;
    int time_to_move;
    bool verbose;
};
struct arguments {
    struct player_arguments p1;
    struct player_arguments p2;
};

void parse_args(int argc, char *const *argv, struct arguments *arguments);

void print_args(struct arguments *arguments);

int performance_testing(struct node *tree, int depth);

#endif //HIVE_UTILS_H
