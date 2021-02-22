//
// Created by duncan on 18-02-21.
//

#ifndef HIVE_EVALUATION_H
#define HIVE_EVALUATION_H


#include <stdbool.h>
#include "../engine/node.h"

bool mm_evaluate_expqueen(struct node* node);
bool mm_evaluate_linqueen(struct node* node);
bool mm_evaluate_noblock(struct node* node);

bool (*mm_evaluate)(struct node*);

#endif //HIVE_EVALUATION_H
