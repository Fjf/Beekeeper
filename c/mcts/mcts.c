//
// Created by duncan on 25-03-21.
//

#include <time.h>
#include <moves.h>
#include <stdlib.h>
#include <stdio.h>
#include <omp.h>
#include "mcts.h"


/*
 *    10   20   50  100  200  500  1000 2000
 *    3/5  4/5  3/5 5/5  5/5  3/5  5/5   4/5
 */



struct node *mcts_init() {
    struct node *root = malloc(sizeof(struct node));
    struct mcts_data *data = malloc(sizeof(struct mcts_data));

    data->draw = data->p0 = data->p1 = 0;

    node_init(root, (void *) data);
    return root;
}

struct node *mcts_add_child(struct node *node, struct board *board) {
    struct node *child = mcts_init();
    child->board = board;

    node_add_child(node, child);
    return child;
}

int mcts_playout(struct node *root, time_t end_time) {
    struct node *node = root;

    while (true) {
        int won = finished_board(node->board);
        if (won > 0) return won;

        // Draw if no children could be generated due to time constraints
        if (!generate_children(node, end_time)) return 3;

        int random_choice = rand() % node->board->move_location_tracker;

        struct list *head, *temp;
        int n = 0;
        node_foreach_safe(node, head, temp) {
            if (random_choice == n++) {
                struct node *child = container_of(head, struct node, node);
                list_remove(&child->node);

                // Dont delete the root node
                if (root != node) {
                    // Done using the previous node completely
                    node_free(node);
                }

                node = child;
                break;
            }
        }
    }
}

void mcts(struct node **tree, int n_playouts) {
    struct node *root = *tree;

    // Create struct to store data.
    root->data = malloc(sizeof(struct mcts_data));

    // Register mcts node add function
    dedicated_add_child = mcts_add_child;
    dedicated_init = mcts_init;

    unsigned int time_to_move = 1000000;

    time_t end_time = time(NULL) + time_to_move;

    generate_children(root, end_time);

    struct list *head;

    bool do_fixed_iterations = n_playouts != -1;

    omp_set_num_threads(1);
    #pragma omp parallel
    {
        #pragma omp single
        {
            // Generate random branches until finish
            while ((do_fixed_iterations && n_playouts > 0) ||
                   (!do_fixed_iterations && end_time > time(NULL))) {
                node_foreach(root, head) {
                    n_playouts--;

                    if (n_playouts == 0) break;
                    #pragma omp task firstprivate(head)
                    {
                        struct node *child = container_of(head, struct node, node);
                        struct mcts_data *data = child->data;
                        int win = mcts_playout(child, end_time);


                        if (win == 1) data->p0++;
                        else if (win == 2) data->p1++;
                        else data->draw++;
                    }
                }
            }
        }
    }

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
