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
#include <limits.h>
#include "mcts.h"
#include "../mm/evaluation.h"



float prioritization(struct node* node) {
    struct board* board = node->board;
    int player = board->turn % 2;

    float count = 0;
    int lqp = board->light_queen_position;
    if (lqp != -1) {
        int x = lqp % BOARD_SIZE;
        int y = lqp / BOARD_SIZE;

        int* points = get_points_around(y, x);
        for (int i = 0; i < 6; i++) {
            int point = points[i];
            count += (float)(board->tiles[point] != EMPTY);
        }
    }

    int dqp = board->dark_queen_position;
    if (dqp != -1) {
        int x = dqp % BOARD_SIZE;
        int y = dqp / BOARD_SIZE;

        int* points = get_points_around(y, x);
        for (int i = 0; i < 6; i++) {
            int point = points[i];
            count -= (float)(board->tiles[point] != EMPTY);
        }
    }

    float result;
    // Positive count means there are more tiles around the queen of player 0
    if (count == 0) {
        result = 1;
    } else if (count < 0 && player == 1 || count > 0 && player == 0) {
        // Bad result
        result =  1.f / (1 + fabsf(count));
    } else {
        result = 1 + fabsf(count);
    }

    return powf(result, 0);
}


float expensive_prioritization(struct node *node) {
    // We want to have this information.
    update_can_move(node->board, node->move.location, node->move.previous_location);


#ifdef TESTING
    float value = 0.;
#else
    float value = 0.f + (float) (rand() % 100) / 1000.f;
#endif

    int won = finished_board(node->board);
    if (won == 1) {
        value = MM_INFINITY - (float) node->board->turn;
    } else if (won == 2) {
        value = -MM_INFINITY + (float) node->board->turn;
    } else if (won == 3) {
        value = 0;
    } else {
        value += unused_tiles(node) * 0.3f;

        int n_encountered = 0;
        int to_encounter = sum_hive_tiles(node->board);

#ifdef CENTERED
        struct board *board = node->board;
        int ly = board->min_y, hy = board->max_y + 1;
        int lx = board->min_x, hx = board->max_x + 1;
#else
        int ly = 0, hy = BOARD_SIZE;
        int lx = 0, hx = BOARD_SIZE;
#endif

        for (int x = lx; x < hx; x++) {
            if (n_encountered == to_encounter) break;
            for (int y = ly; y < hy; y++) {
                if (n_encountered == to_encounter) break;
                unsigned char tile = node->board->tiles[y * BOARD_SIZE + x];
                if (tile == EMPTY) continue;
                n_encountered++;

                if (!node->board->free[y * BOARD_SIZE + x]) {
                    float inc = 1.f;
                    if ((tile & TILE_MASK) == L_ANT) {
                        inc = 2.f;
                    }
                    if ((tile & COLOR_MASK) == LIGHT) {
                        value -= inc;
                    } else {
                        value += inc;
                    }
                }
            }
        }

        if (node->board->light_queen_position != -1) {
            int x1 = node->board->light_queen_position % BOARD_SIZE;
            int y1 = node->board->light_queen_position / BOARD_SIZE;
            int *points = get_points_around(y1, x1);
            for (int i = 0; i < 6; i++) {
                // If there is a tile around the queen of player 1, the value drops by 1
                if (node->board->tiles[points[i]] != EMPTY)
                    value -= 10.0f;
            }
        }

        if (node->board->dark_queen_position != -1) {
            int x2 = node->board->dark_queen_position % BOARD_SIZE;
            int y2 = node->board->dark_queen_position / BOARD_SIZE;
            int *points = get_points_around(y2, x2);
            for (int i = 0; i < 6; i++) {
                // If there is a tile around the queen of player 2, the value increases by 1
                if (node->board->tiles[points[i]] != EMPTY)
                    value += 10.0f;
            }
        }
    }

    int player = node->board->turn % 2;
    float result;
    // Positive count means player 0 has an advantage
    if (value == 0) {
        result = 1;
    } else if (value < 0 && player == 0 || value > 0 && player == 1) {
        // Bad result
        result = 1.f / fabsf(value);
    } else {
        result = 1 + fabsf(value);
    }

    return powf(result, 2);
}


struct node *mcts_init() {
    struct node *root = malloc(sizeof(struct node));
    struct mcts_data *data = malloc(sizeof(struct mcts_data));

    data->value = 0.;
    data->n_sims = 0;
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

int mcts_playout(struct node *root, double end_time) {
    struct node *node = root;
    while (true) {
        struct mcts_data *parent_data = node->data;

        int won = finished_board(node->board);
        if (won > 0) {
            if (!parent_data->keep) {
                // Done using the previous node completely
                node_free(node);
            }
            return won;
        }

        // Draw if no children could be generated due to time/mem constraints
        int generated_children = generate_children(node, end_time, 0);
        if (generated_children != 0) {
            if (!parent_data->keep) {
                // Done using the previous node completely
                node_free(node);
            }
            if (generated_children == ERR_NOMEM) {
                return 5;
            }
            return 4;

            // This is to force draws to not exist anymore.
            int dc = count_tiles_around(node->board, node->board->dark_queen_position);
            int lc = count_tiles_around(node->board, node->board->light_queen_position);

            if (dc > lc) {
                return 1;
            } else {
                return 2;
            }
        }

        struct list *head, *temp;
        // Select random move to play MC(TS).
        int random_choice = rand() % node->board->n_children;

        int n = 0;
        node_foreach_safe(node, head, temp) {
            if (random_choice == n++) {
                // Randomly selected child.
                struct node *child = container_of(head, struct node, node);

                // Dont delete the nodes whose parents have keep flag on.
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

int mcts_playout_prio(struct node *root, double end_time) {
    struct node *node = root;
    while (true) {
        int won = finished_board(node->board);
        if (won > 0) return won;

        // Draw if no children could be generated due to time constraints
        int generated_children = generate_children(node, end_time, 0);
        if (generated_children != 0) {
            if (generated_children == ERR_NOMEM) return 4;
            return 3;
        }

        struct list *head, *temp;
        float prio_sum = 0;
        node_foreach_safe(node, head, temp) {
            struct node *child = container_of(head, struct node, node);
            struct mcts_data *child_data = child->data;
            child_data->prio = prioritization(child);
            prio_sum += child_data->prio;
        }
        float random_choice = ((float) rand() / INT_MAX) * prio_sum;
        struct mcts_data *parent_data = node->data;
        node_foreach_safe(node, head, temp) {
            struct node *child = container_of(head, struct node, node);
            struct mcts_data *child_data = child->data;
            random_choice -= child_data->prio;
            if (random_choice <= 0) {
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


struct node* mcts_select_leaf(struct node* root, struct player_arguments* args) {
    struct node* mcts_leaf = root;
    struct list* head;

    while (!list_empty(&mcts_leaf->children)) {
        struct node *best = NULL;
        double best_value = -INFINITY;

        // If all nodes have no simulations, use first play urgency priority.
        bool first_play_urgency_active = false;
        struct mcts_data* parent_data = mcts_leaf->data;
        if (args->first_play_urgency) {
            if (parent_data->n_sims == 0) {
                first_play_urgency_active = true;
            }
        }

        assert(mcts_leaf->board->n_children != 0);

        node_foreach(mcts_leaf, head) {
            struct node *child = container_of(head, struct node, node);
            struct mcts_data *data = child->data;

            // First play urgency only when all nodes have no simulations done on them.
            if (first_play_urgency_active) {
                double value = expensive_prioritization(child);
                if (best_value < value) {
                    best_value = value;
                    best = child;
                }
                continue;
            }

            // If there is no first-play urgency, ensure every child has at least one simulation.
            if (data->n_sims == 0) {
                best = child;
                break;
            }

            // Player two has a goodness of  v - n
            double value = root->board->turn % 2 == 0 ? data->value : data->n_sims - data->value;

            // Default case uses UCB1 formula.
            double c = args->mcts_constant;

            value = value / data->n_sims + c * sqrt(log(parent_data->n_sims) / data->n_sims);
            if (best_value < value) {
                best_value = value;
                best = child;
            }
        }

        if (best == NULL) {
            node_foreach(mcts_leaf, head) {
                struct node *child = container_of(head, struct node, node);
                struct mcts_data *data = child->data;
                printf("%d %.5f\n", data->n_sims, data->value);
            }
            assert(best != NULL);
        }
        mcts_leaf = best;
    }

    return mcts_leaf;
}


void mcts_cascade_result(struct node* root, struct node* leaf, double value) {
    struct node* node = leaf;

    while (1) {
        struct mcts_data* data = node->data;
        if (node->board->turn % 2 == 1) {
            data->value += value;
        } else {
            data->value += 1 - value;
        }
        data->n_sims += 1;

        if (node == root) return;

        // Get parent of this node.
        node = container_of(node->node.head, struct node, children);
    }
}


void mcts_prepare(struct node* root, struct player_arguments* args) {
    if (args->verbose)
        printf("Initializing data structures for root node.\n");
    struct mcts_data *parent_data;
    if (root->data == NULL) {
        // Initialize counters to 0.
        parent_data = malloc(sizeof(struct mcts_data));
        parent_data->value = 0;
        parent_data->n_sims = 0;
        parent_data->keep = true;
        root->data = parent_data;
    } else {
        parent_data = root->data;
        parent_data->keep = true;
    }

    // Register mcts node add function
    dedicated_add_child = mcts_add_child;
    dedicated_init = mcts_init;
}

struct node* mcts(struct node *root, struct player_arguments *args) {
    // Create struct to store data if it doesnt exist
    mcts_prepare(root, args);

    struct list *head;

    struct timespec cur_time;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cur_time);
    double end_time = (to_usec(cur_time) / 1e6) + args->time_to_move;

    if (args->verbose)
        printf("Generating initial children.\n");

    generate_children(root, end_time, 0);

    int n_iterations = 0;

    // Generate random branches until time runs out
    while (true) {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cur_time);
        double now = (to_usec(cur_time) / 1e6);
        if (now > end_time) break;

        n_iterations++;

        // Select a leaf based on MCTS rules.
        struct node* mcts_leaf = mcts_select_leaf(root, args);
        struct mcts_data* data = mcts_leaf->data;

        // Keep the node until data cascaded
        data->keep = true;

        int win;
        // Argument for prioritization
        if (args->prioritization) {
            win = mcts_playout_prio(mcts_leaf, end_time);
        } else {
            win = mcts_playout(mcts_leaf, end_time);
        }
        if (win == 5) {
            printf("Early memory termination.\n");
            break;
        }

        double value;
        if (win == 1) value = 1;
        else if (win == 2) value = 0;
        else value = .5;

        mcts_cascade_result(root, mcts_leaf, value);

        // Free the new node if there is not enough memory for this node.
        if (max_nodes - n_nodes < 2000) {
            struct node* parent = container_of(mcts_leaf->node.head, struct node, children);
            node_free_children(parent);
        }
    }
    if (args->verbose)
        printf("Generated samples/s: %.2f\n", (n_iterations / args->time_to_move));


#ifdef DEBUG
    printf("Final stats:\n");
#endif

    if (args->verbose)
        printf("Selecting best child.\n");

    double best_ratio = 0.0;
    struct node *best = NULL;

    node_foreach(root, head) {
        struct node *child = container_of(head, struct node, node);
        struct mcts_data *data = child->data;

        double value = root->board->turn % 2 == 0 ? data->value : data->n_sims - data->value;
        double ratio = value / data->n_sims;
        char* mve = string_move(child);
        printf("%.2f/%d = %.2f for %s\n", data->value, data->n_sims, ratio, mve);
        free(mve);
        if (ratio > best_ratio) {
#ifdef DEBUG
            printf("%d/%d/%d\n", data->p0, data->p1, data->draw);
#endif

            best_ratio = ratio;
            best = child;
        }
    }

    if (args->verbose)
        printf("Selected best child.\n");


    if (best == NULL) {
        best = game_pass(root);
    }

    node_free_children(best);

    return best;
}

void print_mcts_data(struct mcts_data *pData) {
    printf("v: %.5f, sims: %d (keep: %d)\n", pData->value, pData->n_sims, pData->keep);
}
