//
// Created by duncan on 12-02-21.
//

#ifndef HIVE_NODE_HPP
#define HIVE_NODE_HPP


#include "list.hpp"

extern unsigned long long max_nodes;
extern unsigned long long n_nodes;

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

extern struct node *game_init();
extern void node_init(struct node* node, void* data);
extern void node_add_child(struct node* node, struct node* child);
extern void node_free(struct node* root);
extern void string_move(struct node* node, char* move);
extern void print_move(struct node* node);

#endif //HIVE_NODE_HPP
