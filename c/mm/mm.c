//
// Created by duncan on 12-02-21.
//


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tt.h>
#include <assert.h>
#include <omp.h>
#include <sys/time.h>
#include <limits.h>
#include "mm.h"
#include "evaluation.h"
#include "../mcts/mcts.h"


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

bool mm(struct node *node, int player, float alpha, float beta, int depth, double end_time) {
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
    if (best != -INFINITY) {
        data->mm_value = best;
        return true;
    }

    // Store for storing in table later.
    float orig_alpha = alpha;
    float orig_beta = beta;

    bool next_sibling = true;
    bool done = finished_board(node->board) != 0;

#pragma omp atomic
    n_evaluated++;
    if (depth == 0 || done) {
        // If the game is finished or no more depth to evaluate.
#pragma omp atomic
        leaf_nodes++;
        mm_evaluate(node);
        return true;
    } else {
        int err = generate_children(node, end_time, 0);
        if (err) return false;

        struct list *head, *hold;
        if (node->board->move_location_tracker == 0 || list_empty(&node->children)) {
            mm_evaluate(node);
            best = data->mm_value;
        } else if (player == 0) { // Player 0 maximizes
            // Generate children for this child then compute values.
            best = -INFINITY;
            node_foreach(node, head) {
                struct node *child = container_of(head, struct node, node);
                struct mm_data* child_data = child->data;

                bool cont = mm(child, !player, alpha, beta, depth - 1, end_time);
                if (!cont) {
                    next_sibling = false;
                    break;
                }

                float value = child_data->mm_value;
                best = MAX(best, value);
                alpha = MAX(best, alpha);
                if (beta <= alpha) break;
            }
        } else { // Player 1 minimizes
            best = INFINITY;
            node_foreach(node, head) {
                struct node *child = container_of(head, struct node, node);
                struct mm_data* child_data = child->data;

                bool cont = mm(child, !player, alpha, beta, depth - 1, end_time);
                if (!cont) {
                    next_sibling = false;
                    break;
                }

                float value = child_data->mm_value;

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


    if (next_sibling) {
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
            }
        }
        data->mm_value = best;
        return true;
    }
    return false;
}


float mm_par(struct node *node, int player, float alpha, float beta, int depth, double end_time) {
    struct mm_data *data = node->data;
    struct list *head;

    int err = generate_children(node, end_time, 0);
    if (err) {
        return 0;
    }
    if (node->board->move_location_tracker == 0 || list_empty(&node->children)) {
        fprintf(stderr, "Root node must have children, current: %d\n", node->board->move_location_tracker);
        exit(1);
    }

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
    if (best != -INFINITY) {
        data->mm_value = best;
        return true;
    }

    // Store for storing in table later.
    float orig_alpha = alpha;
    float orig_beta = beta;

    bool cont = true;
    if (player == 0) { // Player 0 maximizes
        // Generate children for this child then compute values.
        best = -INFINITY;
        node_foreach(node, head) {
            struct node *child = container_of(head, struct node, node);
            struct mm_data* child_data = child->data;

            if (cont) {
                cont = mm(child, !player, alpha, beta, depth - 1, end_time);
            }
            best = MAX(best, child_data->mm_value);
        }
    } else { // Player 1 minimizes
        best = INFINITY;
        node_foreach(node, head) {
            struct node *child = container_of(head, struct node, node);
            struct mm_data* child_data = child->data;

            if (cont) {
                cont = mm(child, !player, alpha, beta, depth - 1, end_time);
            }

            best = MIN(best, child_data->mm_value);
        }
    }

    // If cont is true, it did not terminate due to time constraints, so this best value is valid.
    if (cont) {
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
            }
        }
    }

    return best;
}


struct node *mm_init() {
    struct node *root = malloc(sizeof(struct node));
    struct mm_data *data = malloc(sizeof(struct mm_data));

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


void minimax(struct node **proot, struct player_arguments *args) {
    struct node *root = *proot;

    if (root->board->move_location_tracker > 0) {
        // Reallocate child data structs to ensure there is no old data here.
        struct list* node;
        node_foreach(root, node) {
            struct node* child = container_of(node, struct node, node);
            if (child->data != NULL) {
                free(child->data);
                child->data = NULL;
            }

            child->data = malloc(sizeof(struct mm_data));
        }
    }

    // Create struct for root node to store data.
    root->data = malloc(sizeof(struct mm_data));
    if (root->data == NULL) {
        fprintf(stderr, "No memory to allocate data structure.\n");
        exit(ERR_NOMEM);
    }

    int player = root->board->turn % 2;
    root_player = player;

    // Set the local add child function
    dedicated_add_child = mm_add_child;
    dedicated_init = mm_init;

    mm_evaluate = mm_evaluate_variable;
    if (args->evaluation_function == EVAL_QUEEN) {
        // Set the evaluation function
        mm_evaluate = mm_evaluate_expqueen;
    } else if (args->evaluation_function == EVAL_VARIABLE) {
        mm_evaluate = mm_evaluate_variable;
    } else if (args->evaluation_function == EVAL_DISTANCE) {
        mm_evaluate = mm_evaluate_distance;
    }

#ifdef TESTING
    int depth = 2;
#else
    int depth = 2;
#endif

    struct timespec cur_time;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cur_time);
    double end_time = (to_usec(cur_time) / 1e6) + args->time_to_move;

    int n_total_evaluated = 0;

    while (true) {
        leaf_nodes = n_evaluated = n_created = n_table_returns = 0;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cur_time);
        if (to_usec(cur_time) / 1e6 > end_time) break;

        if (args->verbose) {
            printf("Evaluating depth %d...", depth);
            fflush(stdout);
        }

        float value = mm_par(root, player, -INFINITY, INFINITY, depth, end_time);


        if (args->verbose)
            printf("(%d nodes, %d leaf, %d evaluated, %d table hits)\n", n_created, leaf_nodes, n_evaluated, n_table_returns);

        // Sorting the list by MM-value increases likelihood of finding better moves earlier.
        // In turn, this improves alpha-beta pruning worse subtrees earlier.
        sort(root, player == 0);

        n_total_evaluated += n_evaluated;

        // Break on forced terminal state.
        depth += 1;
        if (value > MM_INFINITY || value < -MM_INFINITY) {
            fprintf(stderr, "Minimax returned no value.");
            exit(1);
        };
        if (value > MM_INFINITY - MAX_TURNS || value < -MM_INFINITY + MAX_TURNS) {
            break;
        };
        if (root->board->turn + depth >= MAX_TURNS) break;
#ifdef TESTING
        if (depth == 6) break;
#endif
    }

    if (args->verbose)
        printf("Evaluated %d nodes (%.5f knodes/s)\n", n_total_evaluated, (n_total_evaluated / args->time_to_move) / 1000);

    struct list *head;
    float best_value = player == 0 ? -INFINITY : INFINITY;
    struct node *best = NULL;
    node_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);
        struct mm_data *data = child->data;

        if ((player == 0 && best_value < data->mm_value)
            || (player == 1 && best_value > data->mm_value)) {
            best_value = data->mm_value;
            best = child;
        }
    }

    if (args->verbose) {
        // Forced win
        if (best_value > MM_INFINITY - MAX_TURNS || best_value < -MM_INFINITY + MAX_TURNS) {
            int t = (int)(MM_INFINITY - fabsf(best_value)) - root->board->turn;
            printf("Evaluated %d children and best is #%d\n", root->board->move_location_tracker, t);
        } else {
            printf("Evaluated %d children and best is %.5f\n", root->board->move_location_tracker, best_value);
        }
    }

    if (best == NULL) {
        root->board->turn++;
    } else {
        list_remove(&best->node);
        node_free(root);

        *proot = best;
    }
}