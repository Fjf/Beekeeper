//
// Created by duncan on 12-02-21.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pns.h"


void do_pn_random_move(struct pn_node **proot) {
    /*
     * Selects a random node from the PN tree, and replaces the root with the child.
     */
    struct pn_node *root = *proot;

    struct board *board = root->board;
    if (board->move_location_tracker == -1) {
        generate_moves(root, root->board->turn % 2);

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

    pn_free(root);
    struct pn_node *new = container_of(head, struct pn_node, node);

    *proot = new;
}


void set_proof_numbers(struct pn_node *root, int original_player_bit) {
    // This node has children
    if (root->expanded) {
        // Update values as a combination of its children
        if (root->node_type == PN_TYPE_AND) {
            root->to_prove = 0;
            root->to_disprove = PN_INF;

            for (struct list *head = root->children.next; head != root->children.head; head = head->next) {
                struct pn_node *child = container_of(head, struct pn_node, node);
                root->to_prove += child->to_prove;
                root->to_disprove = MIN(root->to_disprove, child->to_disprove);
            }
        } else {
            root->to_prove = PN_INF;
            root->to_disprove = 0;

            for (struct list *head = root->children.next; head != root->children.head; head = head->next) {
                struct pn_node *child = container_of(head, struct pn_node, node);
                root->to_disprove += child->to_disprove;
                root->to_prove = MIN(root->to_prove, child->to_prove);
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
                root->to_prove = 0;
                root->to_disprove = PN_INF;
            } else {
                // Negative terminal state
                root->to_prove = PN_INF;
                root->to_disprove = 0;
            }
        } else {
            // This board isn't finished yet, so set it to 1 - 1
            root->to_prove = 1;
            root->to_disprove = 1;
        }
    }
}

int initialize_node(struct pn_node *root, int original_player_bit) {
    // Get the color based on what player the passed player is
    int player_idx = (root->board->turn % 2);
    generate_moves(root, player_idx);
    root->expanded = true;

    // If there are no moves available, increment turn and pass on to next player
    if (root->board->move_location_tracker == 0) {
        root->board->turn++;
        root->board->move_location_tracker = 1;

        // Clone board and add as child (no moves equals this node results in the same board)
        struct board *board = malloc(sizeof(struct board));
        if (!board) return -1;
        memcpy(board, root->board, sizeof(struct board));
        node_add_child(root, board);
    }


    for (struct list *head = root->children.next; head != root->children.head; head = head->next) {
        struct pn_node *child = container_of(head, struct pn_node, node);
        set_proof_numbers(child, original_player_bit);

        // Break if this occurs
        if ((root->node_type == PN_TYPE_OR && child->to_prove == 0)
            || (root->node_type == PN_TYPE_AND && child->to_disprove == 0)) {
            break;
        }
    }
    return 0;
}

struct pn_node *update_ancestors(struct pn_node *node, struct pn_node *root, int original_player_bit) {
    while (true) {
        unsigned int original_prove = node->to_prove;
        unsigned int original_disprove = node->to_disprove;

        set_proof_numbers(node, original_player_bit);

        if (node == root) {
            return node;
        }
        if (node->to_prove == original_prove && node->to_disprove == original_disprove) {
            return node;
        }
        // Select parent of this node.
        struct pn_node *parent = container_of(node->node.head, struct pn_node, children);
        node = parent;
    }
}

struct pn_node *select_most_proving_node(struct pn_node *root) {
    struct pn_node *MPN = NULL;
    unsigned int best = PN_INF;

    while (root->expanded) {
        best = PN_INF;
        for (struct list *head = root->children.next; head != root->children.head; head = head->next) {
            struct pn_node *child = container_of(head, struct pn_node, node);

            // Select the best node based on OR or AND type node.
            unsigned int c_value = (root->node_type == PN_TYPE_OR) ? child->to_prove : child->to_disprove;
            if (c_value <= best) {
                best = child->to_prove;
                MPN = child;
            }
        }
        root = MPN;
    }
    return MPN;
}

void PNS(struct pn_node *root, int original_player_bit, time_t end_time) {
    // Initialize root
    initialize_node(root, original_player_bit);
    set_proof_numbers(root, original_player_bit);

    // Stop when you exceed max depth or max time
    struct pn_node *current = root;
    struct pn_node *mpn;
    while (root->to_prove != 0 && root->to_disprove != 0) {
        time_t cur_time = time(NULL);
        if (cur_time > end_time) {
            return;
        }

        mpn = select_most_proving_node(current);
        initialize_node(mpn, original_player_bit);
        current = update_ancestors(mpn, root, original_player_bit);
    }
}


void do_pn_tree_move(struct pn_node **proot) {
    struct pn_node *root = *proot;
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
    struct pn_node *best = NULL;
    unsigned int best_prove = PN_INF;
    while (head != root->children.head) {
        struct pn_node *data = container_of(head, struct pn_node, node);
        if (data->to_prove < best_prove) {
            best_prove = data->to_prove;
            best = data;
            printf("Found %d\n", data->to_prove);
            if (best_prove == 0) break;
        }

        // Iterate list and track index
        head = head->next;
    }

    // Remove this node from the root, and free the root.
    list_remove(&best->node);

    pn_free(root);

    // Overwrite the node.
    *proot = best;
}