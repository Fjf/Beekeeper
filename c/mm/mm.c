//
// Created by duncan on 12-02-21.
//

//#define BEST_FIRST

// Setting n-turns to 10
// --------------------------------------------------------
// Without best first msec: 23203.12500
// With best first msec: 39859.37500

// With CENTERED flag: ~20000 msec

// With beetle movement checking bugfix: msec: 21937.50000

// Small optimizations 17885.90929 (224 knodes)

// Removed duplicate placing logic 4718.48 (274.28 knodes)
// --------------------------------------------------------

// Increasing n-turns to 15:
// --------------------------------------------------------
// Initial version: 17678ms (155knodes)

// Trade-off version: 18000ms (158knodes)
// This is significantly worse early on, but faster later on.

// Recursion for connected components: 15000ms (170knodes)
// No recurion                       : 16500ms (151knodes)

// Vectorization and all optimization flags: 9600ms (270knodes)
// No recursion with flags: ~9000ms (280knodes)

// No double placement && Beetle movement optimization: 7229.5ms (362knodes)

// Min-max computation optimization: 4900.5ms (595knodes)
// --------------------------------------------------------

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mm.h"
#include "../timing/timing.h"
#include "evaluation.h"

double mm(struct node *node, int player, double alpha, double beta, int depth, int initial_depth, time_t end_time) {
    struct mm_data *data = node->data;
    if (depth == 1 || node->board->done || list_empty(&node->children)) {
        return data->mm_value;
    }

    double best;
    if (player == 0) { // Player 0 maximizes
        best = -INFINITY;
        struct list *head, *hold;
        node_foreach_safe(node, head, hold) {
            struct node *child = container_of(head, struct node, node);

            // Generate children for this child then compute value.
            generate_children(child, end_time);
            double value = mm(child, 1, alpha, beta, depth - 1, initial_depth, end_time);

            best = MAX(best, value);
            alpha = MAX(best, alpha);
            if (beta <= alpha) break;
        }
    } else { // Player 1 minimizes
        best = INFINITY;
        struct list *head, *hold;
        node_foreach_safe(node, head, hold) {
            struct node *child = container_of(head, struct node, node);

            generate_children(child, end_time);
            double value = mm(child, 0, alpha, beta, depth - 1, initial_depth, end_time);

            best = MIN(best, value);
            beta = MIN(best, beta);
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

int n_evaluated = 0;
struct node *mm_add_child(struct node *node, struct board *board) {
    struct node *child = mm_init();
    child->board = board;

    bool done = mm_evaluate(child);
    n_evaluated++;
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


bool generate_children(struct node *root, time_t end_time) {
    /*
     * Returns false if no more children should be generated after this.
     * E.g., memory is full, or time is spent.
     */

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

    // Only generate more nodes if you have no nodes yet
    if (list_empty(&root->children)) {
        generate_moves(root);
    }
    return true;
}

int get_n_repeats(struct board_history_entry *bhe) {
    struct list *node;
    size_t size = sizeof(struct tile) * BOARD_SIZE * BOARD_SIZE + sizeof(struct tile_stack) * TILE_STACK_SIZE;
    list_foreach(&board_history, node) {
        struct board_history_entry *entry = container_of(node, struct board_history_entry, node);
        if (memcmp(bhe, entry, size) == 0) {
            printf("Found board match!\n");
            // Match found
            return entry->repeats;
        }
    }

    return 0;
}

int add_bhe(struct board_history_entry *bhe) {
    struct list *node;
    size_t size = sizeof(struct tile) * BOARD_SIZE * BOARD_SIZE + sizeof(struct tile_stack) * TILE_STACK_SIZE;
    list_foreach(&board_history, node) {
        struct board_history_entry *entry = container_of(node, struct board_history_entry, node);

        if (memcmp(bhe, entry, size) == 0) {
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
    int depth = 4;
    int player = root->board->turn % 2;

    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

#ifdef TESTING
    unsigned int time_to_move = 10000000;
#else
    unsigned int time_to_move = 180;
#endif

    time_t end_time = time(NULL) + time_to_move;

    printf("At turn %d\n", root->board->turn);

    n_evaluated = 0;
    time_t cur_time;

    // Generating level 1 children.
    generate_children(root, end_time);

    struct list* node;
    int n = 0;
    node_foreach(root, node) { n++; }

    while (true) {
        cur_time = time(NULL);
        if (cur_time > end_time) break;

        printf("Evaluating depth %d...", depth);
        fflush(stdout);
        double value = mm(root, player, -INFINITY, INFINITY, depth, depth, end_time);
        printf("(%d nodes)\n", n_evaluated);

        // Break on forced terminal state.
        if (value > MM_INFINITY - 200 || value < -MM_INFINITY + 200) break;
        depth += 1;
#ifdef TESTING
        break;
#endif
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    double sec = (double) (to_usec(end) - to_usec(start)) / 1e6;
    printf("Evaluated %d nodes (%.5f knodes/s)\n", n_evaluated, (n_evaluated / sec) / 1000);

    struct list *head;
    double best_value = player == 0 ? -INFINITY : INFINITY;
    struct node *best = NULL;
    struct board_history_entry tmp;
    struct board_history_entry *bhe = malloc(sizeof(struct board_history_entry));
    node_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);
        struct mm_data *data = child->data;

//        printf("Value %.2f for move ", data->mm_value);
//        print_move(child);

        initialize_bhe(&tmp, child->board);
        int rep = get_n_repeats(&tmp);
        if (rep == 1) {
            // Doing this move would result in 2x repeating move, possibly letting your opponent draw.
            data->mm_value = 0;
        }

        if ((player == 0 && best_value < data->mm_value)
            || (player == 1 && best_value > data->mm_value)) {
            best_value = data->mm_value;
            best = child;
            initialize_bhe(bhe, child->board);
        }
    }
    printf("Evaluated %d children and best is %.5f\n", n, best_value);

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


    timing("minimax", TIMING_END);
}