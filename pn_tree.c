//
// Created by duncan on 04-02-21.
//

#include <stdlib.h>
#include <stdio.h>
#include "pn_tree.h"
#include "list.h"


void node_init(struct pn_node* node, int type) {
    list_init(&node->children);
    list_init(&node->node);
    node->node_type = type;
    node->to_disprove = PN_INF;
    node->to_prove = PN_INF;
    node->expanded = false;
}


struct pn_node* node_add_child(struct pn_node* node, struct board* board) {
    struct pn_node* child = malloc(sizeof(struct pn_node));
    node_init(child, node->node_type ^ 1);
    list_add(&node->children, &child->node);
    child->board = board;
    return child;
}

void _pn_print(struct pn_node* root, int depth) {
    struct list* head = root->children.next;

    for (int i = 0; i < depth; i++) {
        printf("- ");
    }
    printf("%p (%u, %u)\n", root, root->to_prove, root->to_disprove);
    while (head != root->children.head) {
        struct pn_node* child = container_of(head, struct pn_node, node);
        _pn_print(child, depth+1);

        head = head->next;
    }
}
void pn_print(struct pn_node* root) {
    _pn_print(root, 0);
}


void pn_free(struct pn_node* root) {
    // Free children
    struct list* head = root->children.next;
    while (head != root->children.head) {
        struct pn_node* child = container_of(head, struct pn_node, node);
        struct list* temp = head->next;

        list_remove(head);
        pn_free(child);

        head = temp;
    }

    free(root->board);
    list_remove(&root->node);
    free(root);
}
