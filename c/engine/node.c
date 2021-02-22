//
// Created by duncan on 12-02-21.
//

#include <stdlib.h>
#include <stdio.h>
#include "node.h"
#include "board.h"


void node_init(struct node* node, void* data) {
    list_init(&node->children);
    list_init(&node->node);
    node->data = data;

    n_nodes++;
}


void node_add_child(struct node* node, struct node* child) {
    list_add(&node->children, &child->node);
}


void node_free(struct node* root) {
    // Free children
    struct list* head = root->children.next;
    while (head != root->children.head) {
        struct node* child = container_of(head, struct node, node);
        struct list* temp = head->next;

        list_remove(head);
        node_free(child);

        head = temp;
    }

    if (root->data != NULL)
        free(root->data);

    free(root->board);
    list_remove(&root->node);
    free(root);

    n_nodes--;
}


struct node* game_init() {
    struct board *board = init_board();
    struct node* tree = mm_init();
    tree->board = board;
    return tree;
}

void string_tile(unsigned char tile, char* move, int* i) {
    unsigned char color = tile & COLOR_MASK;
    if (color == LIGHT) {
        move[(*i)++] = 'w';
    } else {
        move[(*i)++] = 'b';
    }

    unsigned char type = tile & TILE_MASK;
    if (type == L_ANT) {
        move[(*i)++] = 'A';
    } else if (type == L_BEETLE) {
        move[(*i)++] = 'B';
    } else if (type == L_QUEEN) {
        move[(*i)++] = 'Q';
    } else if (type == L_GRASSHOPPER) {
        move[(*i)++] = 'G';
    } else if (type == L_SPIDER) {
        move[(*i)++] = 'S';
    }

    unsigned char number = (tile & NUMBER_MASK) >> NUMBER_SHIFT;
    move[(*i)++] = number + '0';
}

void string_move(struct node* node, char* move) {
    int i = 0;
    string_tile(node->move.tile, move, &i);
    move[i++] = ' ';

    if (node->move.direction == 4) {
        move[i++] = '\\';
    } else if (node->move.direction == 2) {
        move[i++] = '-';
    } else if (node->move.direction == 0) {
        move[i++] = '/';
    }

    string_tile(node->move.next_to, move, &i);

    if (node->move.direction == 5) {
        move[i++] = '/';
    } else if (node->move.direction == 3) {
        move[i++] = '-';
    } else if (node->move.direction == 1) {
        move[i++] = '\\';
    }
    move[i++] = '\n';
    move[i++] = '\0';
}

void print_move(struct node* node) {
    char str[20];
    string_move(node, str);
    printf("%s\n", str);
}