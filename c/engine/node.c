//
// Created by duncan on 12-02-21.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "node.h"
#include "board.h"
#include "tt.h"


void node_init(struct node* node, void* data) {
    list_init(&node->children);
    list_init(&node->node);
    node->data = data;

    #pragma omp atomic
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

    #pragma omp atomic
    n_nodes--;
}

void node_copy(struct node* dest, struct node* src) {
    memcpy(&dest->move, &src->move, sizeof(struct move));
    dest->board = init_board();
    memcpy(dest->board, src->board, sizeof(struct board));
    dest->board->move_location_tracker = 0;
}


struct node* game_init() {
    // Initialize zobrist hashing table
    zobrist_init();
    // Initialize transposition table (set flag to -1 to know if its empty)
    tt_init();
    // Precompute points around all indices
    initialize_points_around();

    struct board *board = init_board();
    struct node* tree = dedicated_init();
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

char* string_move(struct node* node) {
    char* move = malloc(10 * sizeof(char));
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

    return move;
}

void print_move(struct node* node) {
    char* str = string_move(node);
    printf("%s", str);
    free(str);
}

struct node* list_get_node(struct list* list) {
    return container_of(list, struct node, node);
}