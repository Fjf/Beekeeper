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
#define MM_INFINITY 100000

struct mm_data {
    double mm_value;
    bool mm_evaluated;
};

void minimax(struct node** proot);
struct node* mm_init();
struct node* mm_add_child(struct node* node, struct board* board);
bool generate_children(struct node* root, time_t end_time);
#endif //HIVE_MM_H