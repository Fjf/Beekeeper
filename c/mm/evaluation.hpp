//
// Created by duncan on 18-02-21.
//

#ifndef HIVE_EVALUATION_HPP
#define HIVE_EVALUATION_HPP


#include <stdbool.h>
#include "../engine/node.hpp"

extern bool mm_evaluate_expqueen(struct node* node);
extern bool mm_evaluate_linqueen(struct node* node);
extern bool mm_evaluate_noblock(struct node* node);

extern bool (*mm_evaluate)(struct node*);

#endif //HIVE_EVALUATION_HPP
