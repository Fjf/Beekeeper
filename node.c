//
// Created by duncan on 12-02-21.
//

#include <stdlib.h>
#include "node.h"


void node_init(struct node* node, void* data) {
    list_init(&node->children);
    list_init(&node->node);
    node->data = data;
}


void node_add_child(struct node* node, struct node* child) {
    list_add(&node->children, &child->node);
}
