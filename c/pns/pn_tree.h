//
// Created by duncan on 04-02-21.
//

#ifndef HIVE_PN_TREE_H
#define HIVE_PN_TREE_H

#include "../engine/board.h"
#include "../engine/list.h"
#include "../engine/node.h"

#define PN_TYPE_AND 0
#define PN_TYPE_OR 1
#define PN_INF (1 << 31)

struct pn_data {
    unsigned int to_prove;
    unsigned int to_disprove;
    int node_type;
    bool expanded;
};

void pn_init(struct node* root, int type);
void pn_free_children(struct node* root);
struct node* pn_add_child(struct node* node, struct board* board);
void pn_print(struct node* root);

#endif //HIVE_PN_TREE_H
