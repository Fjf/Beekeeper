//
// Created by duncan on 18-02-21.
//

#ifndef HIVE_EVALUATION_H
#define HIVE_EVALUATION_H

#define EVAL_VARIABLE 2
#define EVAL_QUEEN 1

#include <stdbool.h>
#include "../engine/node.h"

struct eval_multi {
    float queen_ant;
    float queen_rest;
    float movement_ant;
    float movement_rest;
    float used_tiles_ant;
    float used_tiles_rest;
} evaluation_multipliers;

float unused_tiles(struct node* node);
bool mm_evaluate_expqueen(struct node* node);
bool mm_evaluate_variable(struct node* node);

bool (*mm_evaluate)(struct node*);

#endif //HIVE_EVALUATION_H
