//
// Created by duncan on 12-02-21.
//

#ifndef HIVE_NODE_H
#define HIVE_NODE_H


#define KB (1024ull)
#define MB (1024ull * KB)
#define GB (1024ull * MB)


#include "list.h"

unsigned long long max_nodes;
unsigned long long n_nodes;

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
char* string_move(struct node* node);
void print_move(struct node* node);
struct node* list_get_node(struct list* list);

#endif //HIVE_NODE_H
