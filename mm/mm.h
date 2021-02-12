//
// Created by duncan on 12-02-21.
//

#ifndef HIVE_MM_H
#define HIVE_MM_H

#include "../node.h"
#include "../moves.h"

#define MM_TYPE_MIN 0
#define MM_TYPE_MAX 1

struct mm_data {
    double mm_value;
    bool mm_evaluated;
};

void minimax(struct node** proot);
void mm_init(struct node* root);
struct node* mm_add_child(struct node* node, struct board* board);

#endif //HIVE_MM_H
