//
// Created by duncan on 12-02-21.
//

#ifndef HIVE_NODE_H
#define HIVE_NODE_H


#include "list.h"

struct node {
    struct list children;
    struct list node;
    struct board *board;
    void *data;
};

void node_init(struct node* node, void* data);
void node_add_child(struct node* node, struct node* child);


#endif //HIVE_NODE_H
