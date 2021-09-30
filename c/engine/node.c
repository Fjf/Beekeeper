//
// Created by duncan on 12-02-21.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "node.h"
#include "board.h"
#include "tt.h"

#define MAX_MEMORY (4ull * GB)

unsigned long long max_nodes = MAX_MEMORY / (sizeof(struct board) + sizeof(struct node) + sizeof(struct mm_data));
unsigned long long n_nodes = 0;

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


void node_free_children(struct node* root) {
    struct list* head = root->children.next;
    while (head != root->children.head) {
        struct node* child = container_of(head, struct node, node);
        struct list* temp = head->next;

        list_remove(head);
        node_free(child);

        head = temp;
    }
}

void node_free(struct node* root) {
    // Free children
    node_free_children(root);

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
    dest->board->n_children = 0;
}

struct node* game_pass(struct node* root) {
    struct node* copy = mm_init();
    copy->board = malloc(sizeof(struct board));
    memcpy(copy->board, root->board, sizeof(struct board));
    copy->board->turn++;
    return copy;
}


// http://www.concentric.net/~Ttwang/tech/inthash.htm
unsigned long mix(unsigned long a, unsigned long b, unsigned long c) {
    a=a-b;  a=a-c;  a=a^(c >> 13);
    b=b-c;  b=b-a;  b=b^(a << 8);
    c=c-a;  c=c-b;  c=c^(b >> 13);
    a=a-b;  a=a-c;  a=a^(c >> 12);
    b=b-c;  b=b-a;  b=b^(a << 16);
    c=c-a;  c=c-b;  c=c^(b >> 5);
    a=a-b;  a=a-c;  a=a^(c >> 3);
    b=b-c;  b=b-a;  b=b^(a << 10);
    c=c-a;  c=c-b;  c=c^(b >> 15);
    return c;
}

unsigned long seed;
struct node* game_init() {

    // Initialize zobrist hashing table
    if (zobrist_table == NULL)
        zobrist_init();
    if (tt_table == NULL) {
        // Initialize transposition table (set flag to -1 to know if its empty)
        tt_init();

        seed = mix(clock(), time(NULL), getpid());
        // Precompute points around all indices
        initialize_points_around();

        // Randomized seed
        srand(seed);
    }

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

    if (node->move.direction == 5) {
        move[i++] = '\\';
    } else if (node->move.direction == 3) {
        move[i++] = '-';
    } else if (node->move.direction == 1) {
        move[i++] = '/';
    }
    string_tile(node->move.next_to, move, &i);

    if (node->move.direction == 4) {
        move[i++] = '/';
    } else if (node->move.direction == 2) {
        move[i++] = '-';
    } else if (node->move.direction == 0) {
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