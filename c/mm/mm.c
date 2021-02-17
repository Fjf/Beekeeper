//
// Created by duncan on 12-02-21.
//

#define BEST_FIRST

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "mm.h"
#include "../timing/timing.h"


bool mm_evaluate(struct node* node) {
    timing("mm_evaluate", TIMING_START);
    struct mm_data* data = node->data;
    double value = 0. + (float)(rand() % 100) / 100;
    int points[6];

    int won = finished_board(node->board);
    if (won == 1) {
        data->mm_value = 1000.;
        data->mm_evaluated = true;
        return true;
    }
    if (won == 2) {
        data->mm_value = -1000.;
        data->mm_evaluated = true;
        return true;
    }

    for (int x = 0; x < BOARD_SIZE; x++) {
        for (int y = 0; y < BOARD_SIZE; y++) {
            unsigned char tile = node->board->tiles[y * BOARD_SIZE + x].type;
            if (tile == EMPTY) continue;

            if (node->board->tiles[y * BOARD_SIZE + x].free) {
//            if (can_move(node->board, x, y)) {
                float inc = 5.f;
                if ((tile & TILE_MASK) == L_SPIDER) {
                    inc = 2.f;
                } else if ((tile & TILE_MASK) == L_ANT) {
                    inc = 5.f;
                } else if ((tile & TILE_MASK) == L_GRASSHOPPER) {
                    inc = 4.f;
                } else if ((tile & TILE_MASK) == L_BEETLE) {
                    inc = 2.f;
                }
                if ((tile & COLOR_MASK) == LIGHT) {
                    value += inc;
                } else {
                    value -= inc;
                }
            }
        }
    }

    if (node->board->queen1_position != -1) {
        int x1 = node->board->queen1_position % BOARD_SIZE;
        int y1 = node->board->queen1_position / BOARD_SIZE;
        get_points_around(y1, x1, points);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 1, the value drops by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                value -= 25.;
        }
    }

    if (node->board->queen2_position != -1) {
        int x2 = node->board->queen2_position % BOARD_SIZE;
        int y2 = node->board->queen2_position / BOARD_SIZE;
        get_points_around(y2, x2, points);
        for (int i = 0; i < 6; i++) {
            // If there is a tile around the queen of player 2, the value increases by 1
            if (node->board->tiles[points[i]].type != EMPTY)
                value += 25.;
        }
    }

    data->mm_value = value;
    data->mm_evaluated = true;
    timing("mm_evaluate", TIMING_END);
    return false;
}


double mm(struct node* node, int player, int depth, double alpha, double beta) {
    struct mm_data* data = node->data;
    if (depth == 0 || list_empty(&node->children)) {
        // FIXME: I dont think I need this check anymore (check if can remove)
        if (!data->mm_evaluated) {
            mm_evaluate(node);
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
    } else { // Player 1 minimizes
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

    struct mm_data* child_data = child->data;
    bool done = mm_evaluate(child);
    if (done) child->board->done = true;

#ifdef BEST_FIRST
    // Create an ordered list of entries (best first)
    struct list* head;
    list_foreach(node, head) {
        struct node* c = container_of(head, struct node, node);
        struct mm_data* d = c->data;
        if (d->mm_value < child_data->mm_value) {
            break;
        }
    }
    list_insert_after(head->prev, &child->node);
#else
    node_add_child(node, child);
#endif
    return child;
}

int n_evaluated = 0;
void generate_children(struct node* root, int depth, time_t end_time) {
    // Ensure timely finishing
    if (time(NULL) > end_time) return;

    n_evaluated++;

    // Dont generate children if this node is done.
    if (root->board->done) return;

    generate_moves(root);

    if (depth == 0) return;

    struct list* head;
    list_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);

        generate_children(child, depth - 1, end_time);
    }
}


void minimax(struct node** proot) {
    timing("minimax", TIMING_START);

    struct node* root = *proot;
    int depth = 2;
    int player = root->board->turn % 2;

    time_t end_time = time(NULL) + 50; // 5 seconds per move max

    n_evaluated = 0;
    time_t cur_time;
    while (true) {
        cur_time = time(NULL);
        if (cur_time > end_time) break;

        generate_children(root, depth, end_time);

        mm(root, player, depth, -INFINITY, INFINITY);

        depth += 1;
        break;
    }

    printf("Evaluated %d nodes\n", n_evaluated);

    struct list* head;
    double best_value = player == 0 ? -INFINITY : INFINITY;
    struct node* best = NULL;
    list_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);
        struct mm_data* data = child->data;


        if ((player == 0 && best_value < data->mm_value)
        ||  (player == 1 && best_value > data->mm_value)) {
            best_value = data->mm_value;
            best = child;
        }
    }

    if (best == NULL) {
        root->board->turn++;
    } else {
        list_remove(&best->node);
        node_free(root);

        print_move(best);

        *proot = best;
    }

    timing("minimax", TIMING_END);
}