//
// Created by duncan on 12-02-21.
//

#ifndef HIVE_NODE_H
#define HIVE_NODE_H


#include "list.h"

struct move {
    unsigned char tile;
    unsigned char next_to;
    unsigned char direction;
};

struct node {
    struct list children;
    struct list node;
    struct move move;
    struct board *board;
    void *data;
};

struct node *game_init();
void node_init(struct node* node, void* data);
void node_add_child(struct node* node, struct node* child);
void node_free(struct node* root);
void string_move(struct node* node, char* move);
void print_move(struct node* node);

#endif //HIVE_NODE_H
