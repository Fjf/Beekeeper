//
// Created by duncan on 18-02-21.
//

#ifndef HIVE_EVALUATION_H
#define HIVE_EVALUATION_H

#define EVAL_MOVEMENT 2
#define EVAL_QUEEN 1

#include <stdbool.h>
#include "../engine/node.h"

float unused_tiles(struct node* node);
bool mm_evaluate_expqueen(struct node* node);
bool mm_evaluate_movement(struct node* node);
bool mm_evaluate_noblock(struct node* node);

bool (*mm_evaluate)(struct node*);

#endif //HIVE_EVALUATION_H
