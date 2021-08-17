//
// Created by duncan on 12-02-21.
//

#ifndef HIVE_NODE_H
#define HIVE_NODE_H


#define KB (1024ull)
#define MB (1024ull * KB)
#define GB (1024ull * MB)


#include "list.h"

extern unsigned long long int max_nodes;
extern unsigned long long int n_nodes;

struct move {
    unsigned char tile;
    unsigned char next_to;
    unsigned char direction;
    int previous_location;
    int location;
};

struct node {
    struct list children;
    struct list node;
    struct move move;
    struct board *board;
    void *data;
};

struct node *game_init();
struct node* game_pass(struct node* root);

void node_init(struct node* node, void* data);
void node_add_child(struct node* node, struct node* child);
void node_free_children(struct node* root);
void node_free(struct node* root);
void node_copy(struct node* dest, struct node* src);
char* string_move(struct node* node);
void print_move(struct node* node);
struct node* list_get_node(struct list* list);

#endif //HIVE_NODE_H
