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


// --------------------------------------------------------
// Changed evaluation function to be better, so total time is not comparable to above
// Baseline 11089.0ms (808knodes)

// Min-max computation optimization: 9859.5ms (943knodes)

// Early return in can_move computation: 8862.81748ms (1026knodes)

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tt.h>
#include <assert.h>
#include <omp.h>
#include "mm.h"
#include "evaluation.h"


void sort(struct node *node, bool max) {
    struct list *temp, *head, *next;
    // Go over all children.
    node_foreach_safe(node, head, temp) {
        struct node *child = container_of(head, struct node, node);
        struct mm_data *data = child->data;

        list_remove(&child->node);
        // Do insertion sort in another list.
        list_foreach(&node->children, next) {
            struct node *sorted_child = container_of(next, struct node, node);
            struct mm_data *sorted_data = sorted_child->data;

            if (max && data->mm_value >= sorted_data->mm_value
                || !max && data->mm_value <= sorted_data->mm_value)
                break;
        }

        list_insert_after(next->prev, &child->node);
    }
}

int leaf_nodes, n_created, n_evaluated, n_table_returns;
int root_player;

float mm(struct node *node, int player, float alpha, float beta, int depth, int initial_depth, time_t end_time) {
    struct mm_data *data = node->data;

    // Lookup in table, and set values if value is found.
    bool to_replace;
    float best = -INFINITY;
#pragma omp critical (transposition)
    {
        struct tt_entry *entry = tt_retrieve(node, root_player);
        to_replace = (entry == NULL || entry->depth <= depth);
        if (!to_replace) {
            if (entry->flag == TT_FLAG_LOWER) {
                alpha = MAX(alpha, entry->score);
            } else if (entry->flag == TT_FLAG_UPPER) {
                beta = MIN(beta, entry->score);
            } else { // TT_FLAG_TRUE
                #pragma omp atomic
                n_table_returns++;
                best = entry->score;
            }

            if (alpha >= beta) {
                #pragma omp atomic
                n_table_returns++;
                best = entry->score;
            }
        }
    }

    // Terminate from critical section
    if (best != -INFINITY) return best;

    // Store for storing in table later.
    float orig_alpha = alpha;
    float orig_beta = beta;

    bool done = mm_evaluate(node);
    #pragma omp atomic
    n_evaluated++;
    if (depth == 0 || done) {
        // If the game is finished or no more depth to evaluate.
        #pragma omp atomic
        leaf_nodes++;
        best = data->mm_value;
    } else {
        generate_children(node, end_time);
        struct list *head, *hold;
        if (node->board->move_location_tracker == 0 || list_empty(&node->children)) {
            best = data->mm_value;
        } else if (player == 0) { // Player 0 maximizes
            // Generate children for this child then compute values.
            best = -INFINITY;
            node_foreach_safe(node, head, hold) {
                struct node *child = container_of(head, struct node, node);

                float value = mm(child, !player, alpha, beta, depth - 1, initial_depth, end_time);
                best = MAX(best, value);
                alpha = MAX(best, alpha);
                if (beta <= alpha) break;
            }
        } else { // Player 1 minimizes
            best = INFINITY;
            node_foreach_safe(node, head, hold) {
                struct node *child = container_of(head, struct node, node);

                float value = mm(child, !player, alpha, beta, depth - 1, initial_depth, end_time);

                best = MIN(best, value);
                beta = MIN(best, beta);
                if (beta <= alpha) break;
            }
        }



        // Cleanup children.
        node_foreach_safe(node, head, hold) {
            struct node *child = container_of(head, struct node, node);
            node->board->move_location_tracker--;
            node_free(child);
        }
    }

    char flag = TT_FLAG_TRUE;
    if (best <= orig_alpha) {
        flag = TT_FLAG_UPPER;
    } else if (best >= orig_beta) {
        flag = TT_FLAG_LOWER;
    }
    if (to_replace) {
    #pragma omp critical (transposition)
    {
        tt_store(node, best, flag, depth, root_player);
    }}

    data->mm_value = best;

    return best;
}


float mm_par(struct node *node, int player, float alpha, float beta, int depth, int initial_depth, time_t end_time) {
    struct mm_data *data = node->data;

    // Lookup in table, and set values if value is found.
    bool to_replace = false;
    float best;
    struct tt_entry *entry = tt_retrieve(node, root_player);
    to_replace = (entry == NULL || entry->depth <= depth);
    if (!to_replace) {
        if (entry->flag == TT_FLAG_LOWER) {
            alpha = MAX(alpha, entry->score);
        } else if (entry->flag == TT_FLAG_UPPER) {
            beta = MIN(beta, entry->score);
        } else { // TT_FLAG_TRUE
            #pragma omp atomic
            n_table_returns++;
            return entry->score;
        }

        if (alpha >= beta) {
            #pragma omp atomic
            n_table_returns++;
            return entry->score;
        }
    }


    float orig_alpha = alpha;
    float orig_beta = beta;
    generate_children(node, end_time);
    omp_set_num_threads(2);
    #pragma omp parallel
    {
    #pragma omp single
    {
    generate_children(node, end_time);
    if (node->board->move_location_tracker == 0 || list_empty(&node->children)) {
        fprintf(stderr, "Root node must have children.");
        exit(1);
    }
    if (player == 0) { // Player 0 maximizes
        // Generate children for this child then compute values.
        best = -INFINITY;
        struct list *head;
        node_foreach(node, head) {
        #pragma omp task firstprivate(head)
        {

            struct node *child = container_of(head, struct node, node);
            float value = mm(child, !player, alpha, beta, depth - 1, initial_depth, end_time);
            #pragma omp critical (setting)
            {
                best = MAX(best, value);
            }
        }}
    } else { // Player 1 minimizes
        best = INFINITY;
        struct list *head;
        node_foreach(node, head) {
        #pragma omp task firstprivate(head)
        {
            struct node *child = container_of(head, struct node, node);

            float value = mm(child, !player, alpha, beta, depth - 1, initial_depth, end_time);
            #pragma omp critical (setting)
            {
                best = MIN(best, value);
            }
        }}
    }}}

    char flag = TT_FLAG_TRUE;
    if (best <= orig_alpha) {
        flag = TT_FLAG_UPPER;
    } else if (best >= orig_beta) {
        flag = TT_FLAG_LOWER;
    }
    if (to_replace) {
        tt_store(node, best, flag, depth, root_player);
    }

    data->mm_value = best;

    return best;
}


struct node *mm_init() {
    struct node *root = malloc(sizeof(struct node));
    struct mm_data *data = malloc(sizeof(struct mm_data));

    data->mm_evaluated = false;
    data->mm_value = 0.42f;

    node_init(root, (void *) data);
    return root;
}


struct node *mm_add_child(struct node *node, struct board *board) {
    struct node *child = mm_init();
    child->board = board;

    #pragma omp atomic
    n_created++;

    node_add_child(node, child);
    return child;
}


bool generate_children(struct node *root, time_t end_time) {
    /*
     * Returns false if no more children should be generated after this.
     * E.g., memory is full, time is spent, or max move depth is reached.
     */

    // Ensure timely finishing
    if (time(NULL) > end_time) return false;

    // Dont continue generating children if there is no more memory.
    if (max_nodes - n_nodes < 1000) {
        fprintf(stderr, "Not enough memory to hold amount of required nodes (%lld/%lld).\n", n_nodes, max_nodes);
        exit(1);
    }

    // Only generate more nodes if you have no nodes yet
    if (list_empty(&root->children)) {
        generate_moves(root);

        if (list_empty(&root->children)) {
            add_child(root, -1, 0, -1);
        }
    }
    return !list_empty(&root->children);
}

void minimax(struct node **proot) {
    struct node *root = *proot;

    // Create struct for root node to store data.
    root->data = malloc(sizeof(struct mm_data));

    int player = root->board->turn % 2;

    // Set the local add child function
    dedicated_add_child = mm_add_child;
    dedicated_init = mm_init;

    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

#ifdef TESTING
    unsigned int time_to_move = 10000000;
    int depth = 2;
#else
    int depth = 2;
    unsigned int time_to_move = 5;
#endif

    time_t end_time = time(NULL) + time_to_move;

    printf("At turn %d\n", root->board->turn);

    time_t cur_time;
    int n_total_evaluated = 0;
    root_player = player;
    while (true) {
        leaf_nodes = n_evaluated = n_created = n_table_returns = 0;
        cur_time = time(NULL);
        if (cur_time > end_time) break;

        printf("Evaluating depth %d...", depth);
        fflush(stdout);
        float value = mm_par(root, player, -INFINITY, INFINITY, depth, depth, end_time);
        printf("(%d nodes, %d leaf, %d evaluated, %d table hits)\n", n_created, leaf_nodes, n_evaluated,
               n_table_returns);

        // Sorting the list by MM-value increases likelihood of finding better moves earlier.
        // In turn, this improves alpha-beta pruning worse subtrees earlier.
        sort(root, player == 0);

        n_total_evaluated += n_evaluated;

        // Break on forced terminal state.
        depth += 1;
        if (value > MM_INFINITY || value < -MM_INFINITY) continue;
        if (value > MM_INFINITY - 200 || value < -MM_INFINITY + 200) break;
#ifdef TESTING
        if (depth == 6) break;
#endif
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    double sec = (double) (to_usec(end) - to_usec(start)) / 1e6;
    printf("Evaluated %d nodes (%.5f knodes/s)\n", n_total_evaluated, (n_total_evaluated / sec) / 1000);

    struct list *head;
    float best_value = player == 0 ? -INFINITY : INFINITY;
    struct node *best = NULL;
    node_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);
        struct mm_data *data = child->data;

        printf("Value %.2f for ", data->mm_value);
        print_move(child);

        if ((player == 0 && best_value < data->mm_value)
            || (player == 1 && best_value > data->mm_value)) {
            best_value = data->mm_value;
            best = child;
        }
    }
    printf("Evaluated %d children and best is %.5f\n", root->board->move_location_tracker, best_value);

    if (best == NULL) {
        root->board->turn++;
    } else {
        list_remove(&best->node);
        node_free(root);

        print_move(best);

        *proot = best;
    }
}