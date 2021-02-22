//
// Created by duncan on 12-02-21.
//

//#define BEST_FIRST

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mm.h"
#include "../timing/timing.h"
#include "evaluation.h"

double mm(struct node *node, int player, double alpha, double beta, int depth, int initial_depth, time_t end_time) {
    struct mm_data *data = node->data;
    if (depth == 0 || node->board->done) {
        return data->mm_value;
    }

    double best;
    if (player == 0) { // Player 0 maximizes
        best = -MM_INFINITY;
        struct list *head, *hold;
        node_foreach_safe(node, head, hold) {
            struct node *child = container_of(head, struct node, node);

            // Generate children for this child then compute value.
            bool cont = generate_children(child, 1, end_time);
            if (cont) {
                double value = mm(child, 1, alpha, beta, depth - 1, initial_depth, end_time);

                best = MAX(best, value);
                alpha = MAX(best, alpha);
            }
            if (beta <= alpha) break;
        }
    } else { // Player 1 minimizes
        best = MM_INFINITY;
        struct list *head, *hold;
        node_foreach_safe(node, head, hold) {
            struct node *child = container_of(head, struct node, node);

            bool cont = generate_children(child, 1, end_time);
            if (cont) {
                double value = mm(child, 0, alpha, beta, depth - 1, initial_depth, end_time);

                best = MIN(best, value);
                beta = MIN(best, beta);
            }

            if (beta <= alpha) break;
        }
    }

    data->mm_value = best;
    // Delete all nodes except for the root node (including its children).
    if (depth < initial_depth - 1)
        node_free(node);

    return best;
}

struct node *mm_init() {
    struct node *root = malloc(sizeof(struct node));
    struct mm_data *data = malloc(sizeof(struct mm_data));

    data->mm_evaluated = false;
    data->mm_value = 0.;

    node_init(root, (void *) data);
    return root;
}

struct node *mm_add_child(struct node *node, struct board *board) {
    struct node *child = mm_init();
    child->board = board;

    bool done = mm_evaluate(child);
    if (done) child->board->done = true;

#ifdef BEST_FIRST
    int player = board->turn % 2;
    // Create an ordered list of entries (best first)
    struct mm_data* child_data = child->data;
    struct list* head;
    node_foreach(node, head) {
        struct node* c = container_of(head, struct node, node);
        struct mm_data* d = c->data;
        if ((player == 0 && d->mm_value < child_data->mm_value)
        ||  (player == 1 && d->mm_value > child_data->mm_value)) {
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

bool generate_children(struct node *root, int depth, time_t end_time) {
    /*
     * Returns false if no more children should be generated after this.
     * E.g., memory is full, or time is spent.
     */
    n_evaluated++;

    // Ensure timely finishing
    if (time(NULL) > end_time) return false;

    // Dont generate children if this node is done.
    if (root->board->done) {
        return true;
    }

    // Dont continue generating children if there is no more memory.
    if (max_nodes - n_nodes < 1000) {
        return false;
    }

    if (depth == 0) return true;

    // Only generate more nodes if you have no nodes yet
    if (list_empty(&root->children)) {
        generate_moves(root);
    }

    struct list *head;
    node_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);

        // Dont continue generating children if there is no more memory.
        if (max_nodes - n_nodes < 1000) {
            return false;
        }

        generate_children(child, depth - 1, end_time);
    }
    return true;
}

int get_n_repeats(struct board_history_entry *bhe) {
    struct list *node;
    list_foreach(&board_history, node) {
        struct board_history_entry *entry = container_of(node, struct board_history_entry, node);

        if (memcmp(bhe, entry, sizeof(struct board_history_entry)) == 0) {
            // Match found
            return entry->repeats;
        }
    }

    return 0;
}

int add_bhe(struct board_history_entry *bhe) {
    struct list *node;
    list_foreach(&board_history, node) {
        struct board_history_entry *entry = container_of(node, struct board_history_entry, node);

        if (memcmp(bhe, entry, sizeof(struct board_history_entry)) == 0) {
            // Match found, early return.
            entry->repeats++;
            return entry->repeats;
        }
    }

    list_init(&bhe->node);
    bhe->repeats = 1;
    list_add(&board_history, &bhe->node);
    return bhe->repeats;
}

void initialize_bhe(struct board_history_entry *bhe, struct board *board) {
    memcpy(bhe->tiles, board->tiles, sizeof(board->tiles));
    memcpy(bhe->stack, board->stack, sizeof(board->stack));
}

void minimax(struct node **proot) {
    timing("minimax", TIMING_START);

    struct node *root = *proot;
    int depth = 2;
    int player = root->board->turn % 2;

    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    time_t end_time = time(NULL) + 5; // 5 seconds per move max

    // Generating level 1 children.

    n_evaluated = 0;
    time_t cur_time;
    while (true) {
        cur_time = time(NULL);
        if (cur_time > end_time) break;

        printf("Generating to depth %d\n", depth);
        generate_children(root, 1, end_time);
        double best = mm(root, player, -INFINITY, INFINITY, depth, depth, end_time);
        printf("Best: %.5f\n", best);
        depth += 1;
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    double sec = (double) (to_usec(end) - to_usec(start)) / 1e6;
    printf("Evaluated %d nodes (%.5f nps)\n", n_evaluated, n_evaluated / sec);

    struct list *head;
    double best_value = player == 0 ? -INFINITY : INFINITY;
    struct node *best = NULL;
    struct board_history_entry *bhe = malloc(sizeof(struct board_history_entry));
    node_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);
        struct mm_data *data = child->data;

        initialize_bhe(bhe, child->board);
        if (get_n_repeats(bhe) == 2) {
            // Doing this move would result in 3x repeating move.
            data->mm_value = 0;
        }

        if ((player == 0 && best_value < data->mm_value)
            || (player == 1 && best_value > data->mm_value)) {
            best_value = data->mm_value;
            best = child;
        }
    }

    printf("MM value of %.5f\n", best_value);
    printf("Using %llu/%llu nodes\n", n_nodes, max_nodes);
    if (best == NULL) {
        root->board->turn++;
    } else {
        int repeats = add_bhe(bhe);
        if (repeats == 3) {
            // TODO: Refactor this to another location
            printf("Draw by repetition\n");
            exit(0);
        }

        list_remove(&best->node);
        node_free(root);

        print_move(best);

        *proot = best;
    }
    printf("Using %llu/%llu nodes\n", n_nodes, max_nodes);


    timing("minimax", TIMING_END);
}