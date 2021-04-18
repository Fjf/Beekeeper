//
// Created by duncan on 12-02-21.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pns.h"


void do_pn_random_move(struct node **proot) {
    /*
     * Selects a random node from the PN tree, and replaces the root with the child.
     */
    struct node *root = *proot;

    struct board *board = root->board;
    if (board->move_location_tracker == -1) {
        generate_moves(root, 0);

        if (board->move_location_tracker == -1) {
            fprintf(stderr, "No moves could be generated\n");
            exit(1);
        }
    }

    if (board->move_location_tracker == 0) {
        board->turn++;
        return;
    }
    int selected = rand() % board->move_location_tracker;



    struct list *head = root->children.next;
    for (int i = 0; i < selected; i++) {
        head = head->next;
    }

    list_remove(head);

    node_free(root);
    struct node *new = container_of(head, struct node, node);

    *proot = new;
}


void set_proof_numbers(struct node *root, int original_player_bit) {
    // This node has children
    struct pn_data* data = root->data;
    if (data->expanded) {
        // Update values as a combination of its children
        if (data->node_type == PN_TYPE_AND) {
            data->to_prove = 0;
            data->to_disprove = PN_INF;

            for (struct list *head = root->children.next; head != root->children.head; head = head->next) {
                struct node *child = container_of(head, struct node, node);
                struct pn_data* child_data = child->data;
                data->to_prove += child_data->to_prove;
                data->to_disprove = MIN(data->to_disprove, child_data->to_disprove);
            }
        } else {
            data->to_prove = PN_INF;
            data->to_disprove = 0;

            for (struct list *head = root->children.next; head != root->children.head; head = head->next) {
                struct node *child = container_of(head, struct node, node);
                struct pn_data* child_data = child->data;
                data->to_disprove += child_data->to_disprove;
                data->to_prove = MIN(data->to_prove, child_data->to_prove);
            }
        }
    } else {
        // Leaf in current tree structure
        int won = finished_board(root->board);
        if (won) {
            // Terminal state, so alter the PN values to be terminal.
            if ((won == 1 && original_player_bit == LIGHT)
                || (won == 2 && original_player_bit == DARK)) {
                // Positive terminal state
                data->to_prove = 0;
                data->to_disprove = PN_INF;
            } else {
                // Negative terminal state
                data->to_prove = PN_INF;
                data->to_disprove = 0;
            }
        } else {
            // This board isn't finished yet, so set it to 1 - 1
            data->to_prove = 1;
            data->to_disprove = 1;
        }
    }
}

int initialize_node(struct node *root, int original_player_bit) {
    generate_moves(root, 0);

    struct pn_data* data = root->data;
    data->expanded = true;

    // If there are no moves available, increment turn and pass on to next player
    if (root->board->move_location_tracker == 0) {
        root->board->turn++;
        root->board->move_location_tracker = 1;

        // Clone board and add as child (no moves equals this node results in the same board)
        struct board *board = malloc(sizeof(struct board));
        if (!board) return -1;
        memcpy(board, root->board, sizeof(struct board));

        pn_add_child(root, board);
    }


    for (struct list *head = root->children.next; head != root->children.head; head = head->next) {
        struct node *child = container_of(head, struct node, node);
        struct pn_data* child_data = child->data;
        set_proof_numbers(child, original_player_bit);

        // Break if this occurs
        if ((data->node_type == PN_TYPE_OR && child_data->to_prove == 0)
            || (data->node_type == PN_TYPE_AND && child_data->to_disprove == 0)) {
            break;
        }
    }
    return 0;
}

struct node *update_ancestors(struct node *node, struct node *root, int original_player_bit) {
    while (true) {
        struct pn_data* data = node->data;
        unsigned int original_prove = data->to_prove;
        unsigned int original_disprove = data->to_disprove;

        set_proof_numbers(node, original_player_bit);

        if (node == root) {
            return node;
        }
        if (data->to_prove == original_prove && data->to_disprove == original_disprove) {
            return node;
        }
        // Select parent of this node.
        struct node *parent = container_of(node->node.head, struct node, children);
        node = parent;
    }
}

struct node *select_most_proving_node(struct node *root) {
    struct node *MPN = NULL;
    unsigned int best = PN_INF;

    struct pn_data* data = root->data;
    while (data->expanded) {
        best = PN_INF;
        for (struct list *head = root->children.next; head != root->children.head; head = head->next) {
            struct node *child = container_of(head, struct node, node);
            struct pn_data* child_data = child->data;

            // Select the best node based on OR or AND type node.
            unsigned int c_value = (data->node_type == PN_TYPE_OR) ? child_data->to_prove : child_data->to_disprove;
            if (c_value <= best) {
                best = child_data->to_prove;
                MPN = child;
            }
        }
        root = MPN;
        data = root->data;
    }
    return MPN;
}

void PNS(struct node *root, int original_player_bit, time_t end_time) {
    // Initialize root
    initialize_node(root, original_player_bit);
    set_proof_numbers(root, original_player_bit);

    // Stop when you exceed max depth or max time
    struct node *current = root;
    struct node *mpn;
    struct pn_data* data = root->data;
    while (data->to_prove != 0 && data->to_disprove != 0) {
        time_t cur_time = time(NULL);
        if (cur_time > end_time) {
            return;
        }

        mpn = select_most_proving_node(current);
        initialize_node(mpn, original_player_bit);
        current = update_ancestors(mpn, root, original_player_bit);
    }
}


void do_pn_tree_move(struct node **proot) {
    struct node *root = *proot;
    int player_idx = root->board->turn % 2;
    int original_player_bit = player_idx << COLOR_SHIFT;

    time_t end_time = time(NULL) + 5; // 5 seconds per move max
    PNS(root, original_player_bit, end_time);

    print_board(root->board);

    printf("Done with PNS\n");

    if (root->board->move_location_tracker == 0) {
        root->board->turn++;
        return;
    }

    struct list *head = root->children.next;
    struct node *best = NULL;
    unsigned int best_prove = PN_INF;
    while (head != root->children.head) {
        struct node *child = container_of(head, struct node, node);
        struct pn_data* data = child->data;
        if (data->to_prove < best_prove) {
            best_prove = data->to_prove;
            best = child;
            printf("Found %d\n", data->to_prove);
            if (best_prove == 0) break;
        }

        // Iterate list and track index
        head = head->next;
    }

    // Remove this node from the root, and free the root.
    list_remove(&best->node);

    node_free(root);

    // Overwrite the node.
    *proot = best;
}