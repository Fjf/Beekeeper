//
// Created by duncan on 12-02-21.
//

#ifndef HIVE_MM_H
#define HIVE_MM_H

#include "../engine/node.h"
#include "../engine/moves.h"
#include <time.h>
#include "utils.h"

#define MM_INFINITY 10000000.0f

struct mm_data {
    float mm_value;
};

struct node* minimax(struct node *root, struct player_arguments *args);
struct node* mm_init();
struct node* mm_add_child(struct node* node, struct board* board);

#endif //HIVE_MM_H
