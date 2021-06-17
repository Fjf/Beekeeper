//
// Created by duncan on 30-03-21.
//

#ifndef HIVE_UTILS_H
#define HIVE_UTILS_H

#include "node.h"
#include "../mm/evaluation.h"

#define ALG_MM 0
#define ALG_MCTS 1
#define ALG_RANDOM 2
#define ALG_MANUAL 3
#define alg_to_str(in) ((in) == ALG_MM ? "Minimax" : \
                       ((in) == ALG_MCTS ? "MCTS" :  \
                       ((in) == ALG_RANDOM ? "Random" : \
                       ((in) == ALG_MANUAL ? "Manual" : "Unknown"))))
#define eval_to_str(in)((in) == EVAL_QUEEN ? "Queen" : \
                       ((in) == EVAL_DISTANCE ? "Distance" : \
                       ((in) == EVAL_VARIABLE ? "Variable" : "Unknown")))

struct player_arguments {
    int algorithm;
    double mcts_constant;
    double time_to_move;
    bool prioritization;
    bool first_play_urgency;
    bool verbose;
    int evaluation_function;
};
struct arguments {
    struct player_arguments p1;
    struct player_arguments p2;
};

void parse_args(int argc, char *const *argv, struct arguments *arguments);

void print_args(struct arguments *arguments);

int performance_testing(struct node *tree, int depth);
int performance_testing_parallel(struct node* tree, int depth, int par_depth);
void random_moves(struct node **tree, int n_moves);

#endif //HIVE_UTILS_H
