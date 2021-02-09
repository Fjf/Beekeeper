//
// Created by duncan on 04-02-21.
//

#ifndef HIVE_PN_TREE_H
#define HIVE_PN_TREE_H

#include "board.h"

#define PN_NODE_UNDECIDED 0
#define PN_NODE_PROVED 1
#define PN_NODE_DISPROVED -1

#define PN_TYPE_AND 0
#define PN_TYPE_OR 1

struct pn_node {
    struct list children;
    struct list node;
    int proven;
    int to_prove;
    int to_disprove;
    int node_type;
    struct board* board;
};


void node_init(struct pn_node* node, int type);
struct pn_node* node_add_child(struct pn_node* node, struct board* board);
void pn_free_children(struct pn_node* root);
void pn_free(struct pn_node* root);
void pn_print(struct pn_node* root);

#endif //HIVE_PN_TREE_H
