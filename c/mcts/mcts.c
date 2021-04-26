//
// Created by duncan on 25-03-21.
//

#include <time.h>
#include <moves.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include <math.h>
#include <assert.h>
#include <string.h>
#include "mcts.h"


/*
 *    10   20   50  100  200  500  1000 2000
 *    3/5  4/5  3/5 5/5  5/5  3/5  5/5   4/5
 */



struct node *mcts_init() {
    struct node *root = malloc(sizeof(struct node));
    struct mcts_data *data = malloc(sizeof(struct mcts_data));

    data->draw = data->p0 = data->p1 = 0;
    data->keep = false;

    node_init(root, (void *) data);
    return root;
}

struct node *mcts_add_child(struct node *node, struct board *board) {
    struct node *child = mcts_init();
    child->board = board;

    node_add_child(node, child);
    return child;
}

void print_mcts_data(struct mcts_data *pData);

int mcts_playout(struct node *root, time_t end_time) {
    struct node *node = root;

    while (true) {
        int won = finished_board(node->board);
        if (won > 0) return won;

        // Draw if no children could be generated due to time constraints
        bool generated_children = generate_children(node, end_time, 0);
        if (!generated_children) return 3;

        int random_choice = rand() % node->board->move_location_tracker;

        struct list *head, *temp;
        int n = 0;
        struct mcts_data* parent_data = node->data;
        node_foreach_safe(node, head, temp) {
            if (random_choice == n++) {
                struct node *child = container_of(head, struct node, node);

                // Dont delete the nodes with the keep flag on
                if (!parent_data->keep) {
                    list_remove(&child->node);
                    // Done using the previous node completely
                    node_free(node);
                }

                node = child;
                break;
            }
        }
    }
}

void mcts(struct node **tree, struct player_arguments *args) {
    struct node *root = *tree;

    // Create struct to store data if it doesnt exist
    struct mcts_data* parent_data;
    if (root->data == NULL) {
        // Initialize counters to 0.
        parent_data = malloc(sizeof(struct mcts_data));
        parent_data->draw = parent_data->p0 = parent_data->p1 = 0;
        parent_data->keep = true;
        root->data = parent_data;
    } else {
        parent_data = root->data;
        parent_data->keep = true;
    }


    // Register mcts node add function
    dedicated_add_child = mcts_add_child;
    dedicated_init = mcts_init;

    unsigned int time_to_move = (unsigned int) args->time_to_move;

    time_t end_time = time(NULL) + time_to_move;

    generate_children(root, end_time, 0);

    struct list *head;

    bool do_fixed_iterations = false;
    int n_iterations = -1;


    // Generate random branches until finish
    #pragma omp parallel
    while ((do_fixed_iterations && n_iterations > 0) ||
           (!do_fixed_iterations && end_time > time(NULL))) {

        n_iterations--;
        if (n_iterations == 0) break;

        struct node* best = NULL;
        double best_value = -INFINITY;
        node_foreach(root, head) {
            struct node* child = container_of(head, struct node, node);
            struct mcts_data* data = child->data;

            double n_wins = (double) (root->board->turn % 2 == 0 ? data->p0 : data->p1);
            double n_simulations = (double) (data->p0 + data->p1 + data->draw);
            double parent_simulations = (double) (parent_data->p0 + parent_data->p1 + parent_data->draw);
            if (n_simulations == 0) {
                // Ensure each child has at least one simulation.
                best = child;
                break;
            }

            double c = args->mcts_constant;
            double value = n_wins / n_simulations + c * sqrt(log(parent_simulations) / n_simulations);
            if (best_value < value) {
                best_value = value;
                best = child;
            }
        }

        // best cannot be NULL here.
        assert(best != NULL);

        // TODO: Parallelism
        struct mcts_data *data = best->data;
        // Selected node should not be freed.
        data->keep = true;

        #pragma omp parallel for
        for (int n = 0; n < omp_get_num_threads(); n++) {
            struct node* local = mcts_init();
            memcpy(local->data, best->data, sizeof(struct mcts_data));
            node_copy(local, best);
            int win = mcts_playout(local, end_time);
            node_free(local);

            #pragma omp critical (set_mcts_data)
            {
                if (win == 1) {
                    data->p0++;
                    parent_data->p0++;
                } else if (win == 2) {
                    data->p1++;
                    parent_data->p1++;
                } else {
                    data->draw++;
                    parent_data->draw++;
                }
            }

        }
    }
    printf("samples/s: %.2f\n", ((-n_iterations) * omp_get_num_threads() / (double)time_to_move));

#ifdef DEBUG
    printf("Final stats:\n");
#endif
    float best_ratio = 0.0f;
    struct node *best = NULL;

    node_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);
        struct mcts_data *data = child->data;


        float ratio = (float) (data->p0 + 1) / (float) (data->p1 + 1);
        if (root->board->turn % 2 == 1) ratio = 1 / ratio;

        if (ratio > best_ratio) {
#ifdef DEBUG
            printf("%d/%d/%d\n", data->p0, data->p1, data->draw);
#endif
            best_ratio = ratio;
            best = child;
        }
    }

    if (best == NULL) {
        root->board->turn++;
    } else {
        list_remove(&best->node);
        node_free(root);

        *tree = best;
    }
}

void print_mcts_data(struct mcts_data *pData) {
    printf("w: %d, l: %d, d: %d (keep: %d)\n", pData->p0, pData->p1, pData->draw, pData->keep);
}
