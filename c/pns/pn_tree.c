//
// Created by duncan on 04-02-21.
//

#include <stdlib.h>
#include <stdio.h>
#include "pn_tree.h"

void pn_init(struct node* root, int type) {
    struct pn_data* data = malloc(sizeof(struct pn_data));

    data->node_type = type;
    data->to_disprove = PN_INF;
    data->to_prove = PN_INF;
    data->expanded = false;

    node_init(root, (void*)data);
}

struct node* pn_add_child(struct node* node, struct board* board) {
    struct pn_data* data = node->data;

    // Initialize child
    struct node* child = malloc(sizeof(struct node));
    pn_init(child, data->node_type ^ 1);
    child->board = board;

    node_add_child(node, child);
    return child;
}



void pn_print_2(struct node* root, int depth) {
    struct list* head = root->children.next;
    struct pn_data* data = root->data;

    for (int i = 0; i < depth; i++) {
        printf("- ");
    }
    printf("%p (%u, %u)\n", root, data->to_prove, data->to_disprove);
    while (head != root->children.head) {
        struct node* child = container_of(head, struct node, node);
        pn_print_2(child, depth+1);

        head = head->next;
    }
}
void pn_print(struct node* root) {
    pn_print_2(root, 0);
}



