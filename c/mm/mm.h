//
// Created by duncan on 12-02-21.
//

#ifndef HIVE_MM_H
#define HIVE_MM_H

#include "../engine/node.h"
#include "../engine/moves.h"
#include <time.h>

#define MM_TYPE_MIN 0
#define MM_TYPE_MAX 1

struct mm_data {
    double mm_value;
    bool mm_evaluated;
};

void minimax(struct node** proot);
void mm_init(struct node* root);
struct node* mm_add_child(struct node* node, struct board* board);
void generate_children(struct node* root, int depth, time_t end_time);
#endif //HIVE_MM_H
