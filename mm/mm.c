//
// Created by duncan on 12-02-21.
//

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "mm.h"
#include "../pns/pn_tree.h"

double mm_evaluate(struct node* node) {
    double value = 0. + (float)(rand() % 100) / 100;
    int points[6];

    int won = finished_board(node->board);
    if (won == 1) return 10000;
    if (won == 2) return -10000;

    if (node->board->queen1_position != -1) {
        int x1 = node->board->queen1_position % BOARD_SIZE;
        int y1 = node->board->queen1_position / BOARD_SIZE;
        get_points_around(y1, x1, points);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 1, the value drops by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                value -= 1.;
        }
    }

    if (node->board->queen2_position != -1) {
        int x2 = node->board->queen2_position % BOARD_SIZE;
        int y2 = node->board->queen2_position / BOARD_SIZE;
        get_points_around(y2, x2, points);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 2, the value increases by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                value += 1;
        }
    }

    return value;
}


double mm(struct node* node, int player, int depth, double alpha, double beta) {
    struct mm_data* data = node->data;
    if (depth == 0 || list_empty(&node->children)) {
        if (!data->mm_evaluated) {
            data->mm_value = mm_evaluate(node);
        }
        return data->mm_value;
    }

    double best;
    if (player == 0) { // Player 0 maximizes
        best = -INFINITY;
        struct list *head;
        list_foreach(node, head) {
            struct node *child = container_of(head, struct node, node);
            double value = mm(child, 1, depth - 1, alpha, beta);
            best = MAX(best, value);
            alpha = MAX(best, alpha);
            if (beta <= alpha) break;
        }
    } else {
        best = INFINITY;
        struct list *head;
        list_foreach(node, head) {
            struct node *child = container_of(head, struct node, node);

            double value = mm(child, 0, depth - 1, alpha, beta);
            best = MIN(best, value);
            beta = MIN(best, beta);
            if (beta <= alpha) break;
        }
    }
    data->mm_value = best;
    return best;
}

void mm_init(struct node* root) {
    struct mm_data* data = malloc(sizeof(struct mm_data));

    data->mm_evaluated = false;
    data->mm_value = 0.;

    node_init(root, (void*)data);
}

struct node* mm_add_child(struct node* node, struct board* board) {
    // Initialize child
    struct node* child = malloc(sizeof(struct node));
    mm_init(child);
    child->board = board;

    node_add_child(node, child);
    return child;
}


void generate_children(struct node* root, int depth, time_t end_time) {
    // Ensure timely finishing
    if (time(NULL) > end_time) return;

    root->data = malloc(sizeof(struct mm_data));
    struct mm_data* d = root->data;
    d->mm_value = 0.;
    d->mm_evaluated = false;

    generate_moves(root);

    if (depth == 0) return;

    struct list* head;
    list_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);

        generate_children(child, depth - 1, end_time);
    }
}


void minimax(struct node** proot) {
    struct node* root = *proot;
    int depth = 2;

    time_t end_time = time(NULL) + 5; // 5 seconds per move max

    time_t cur_time;
    while (true) {
        cur_time = time(NULL);
        if (cur_time > end_time) break;

        generate_children(root, depth, end_time);

        mm(root, root->board->turn % 2, depth, -INFINITY, INFINITY);

        depth += 1;
    }

    struct list* head;
    double best_value = -INFINITY;
    struct node* best = NULL;
    list_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);
        struct mm_data* data = child->data;


        if (best_value < data->mm_value) {
            best_value = data->mm_value;
            best = child;
        }
    }

    list_remove(&best->node);
    pn_free(root);

    *proot = best;
}