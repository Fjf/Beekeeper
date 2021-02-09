#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "board.h"
#include "moves.h"
#include "pn_tree.h"
#include "list.h"

struct tile_stack* find_tile_in_stack(struct board* board, int location) {
    struct tile_stack* highest_tile = NULL;
    int highest_idx = -1;
    for (int i = 0; i < TILE_STACK_SIZE; i++) {
        if (board->stack[i].location == location
        && highest_idx < board->stack[i].z) {
            highest_idx = board->stack[i].z;
            highest_tile = &board->stack[i];
        }
    }
    return highest_tile;
}

void do_turn(struct board *board, int move_location_index, struct player *player) {
    int idx = board->move_locations[move_location_index].location;
    int type = board->move_locations[move_location_index].move;
    int previous_location = board->move_locations[move_location_index].previous_location;
    int masked_type = type & TILE_MASK;


    // Reduce the amount of available tiles per player
    // Only do this if the tile was not moved but instead newly placed
    if (previous_location == -1) {
        if (masked_type == L_QUEEN)
            player->queens_left--;
        else if (masked_type == L_BEETLE)
            player->beetles_left--;
        else if (masked_type == L_GRASSHOPPER)
            player->grasshoppers_left--;
        else if (masked_type == L_ANT)
            player->ants_left--;
        else if (masked_type == L_SPIDER)
            player->spiders_left--;
    } else {
        // Remove piece from previous location
        board->tiles[previous_location].type = NONE;

        // Check if there is a tile in the stack
        // If this move is a beetle, check the tile stack to know if the beetle
        //  was on top of another piece
        struct tile_stack* stacked_tile = find_tile_in_stack(board, previous_location);
        if (stacked_tile != NULL) {
            board->tiles[previous_location].type = stacked_tile->type;
            stacked_tile->location = -1; // Clear data
            // This is to track all the stacked tiles in a simple list to help cloning.
            board->n_stacked--;
        }
    }

    // If this move is on top of an existing tile, store this tile in a trackable struct
    if (board->tiles[idx].type != NONE) {
        for (int i = 0; i < TILE_STACK_SIZE; i++) {
            if (board->stack[i].location == -1) {
                struct tile_stack* highest = find_tile_in_stack(board, idx);
                int z = 0;
                if (highest != NULL)
                    z = highest->z;

                board->stack[i].location = idx;
                board->stack[i].type = board->tiles[idx].type;
                board->stack[i].z = z + 1;
                break;
            }
        }

        // This is to track all the stacked tiles in a simple list to help cloning.
        board->n_stacked++;
    }

    // Do this for easier board-finished state checking (dont have to take into account beetles).
    if (type == L_QUEEN) {
        board->queen1_position = idx;
    } else if (type == D_QUEEN) {
        board->queen2_position = idx;
    }

    // Do this move.
    board->tiles[idx].type = type;
    board->move_location_tracker = -1; // Reset to initial value
    board->turn++;

    translate_board(board);
}

void generate_moves(struct board *board, struct player *player, int player_bit) {
    board->move_location_tracker = 0;
    // By move 4 for each player, the queen has to be placed.
    if (board->turn > 5 && player->queens_left == 1) {
        generate_placing_moves(board, L_QUEEN | player_bit);
        return;
    }
    if (player->spiders_left > 0)
        generate_placing_moves(board, L_SPIDER | player_bit);
    if (player->beetles_left > 0)
        generate_placing_moves(board, L_BEETLE | player_bit);
    if (player->grasshoppers_left > 0)
        generate_placing_moves(board, L_GRASSHOPPER | player_bit);
    if (player->ants_left > 0)
        generate_placing_moves(board, L_ANT | player_bit);
    if (player->queens_left > 0)
        generate_placing_moves(board, L_QUEEN | player_bit);

    generate_free_moves(board, player_bit);
}

void print_player(struct player* player) {
    printf("Beetles: %d\n", player->beetles_left);
    printf("Spiders: %d\n", player->spiders_left);
    printf("Grasshoppers: %d\n", player->grasshoppers_left);
    printf("Queens: %d\n", player->queens_left);
    printf("Ants: %d\n", player->ants_left);
}


void test_beetle(struct board *board) {
    add_move(board, BOARD_SIZE + 1, L_BEETLE, -1);
    do_turn(board, 0, &board->player1);
    add_move(board, BOARD_SIZE + 2, D_BEETLE, -1);
    do_turn(board, 0, &board->player2);
    add_move(board, BOARD_SIZE + 3, L_BEETLE, -1);
    do_turn(board, 0, &board->player1);
    add_move(board, (2 * BOARD_SIZE) + 2, D_BEETLE, -1);
    do_turn(board, 0, &board->player2);
    print_board(board);

    add_move(board, BOARD_SIZE + 2, L_BEETLE, BOARD_SIZE + 3);
    do_turn(board, 0, &board->player1);

    print_board(board);

    add_move(board, (2 * BOARD_SIZE) + 3, D_BEETLE, (2 * BOARD_SIZE) + 2);
    do_turn(board, 0, &board->player2);

    print_board(board);

    add_move(board, BOARD_SIZE + 3, L_BEETLE, BOARD_SIZE + 2);
    do_turn(board, 0, &board->player1);

    print_board(board);
}

void do_random_move(struct board *board, struct player* player, int player_bit) {
    generate_moves(board, player, player_bit);

    if (board->move_location_tracker == 0) {
        board->turn++;
        return;
    }
    int selected = rand() % board->move_location_tracker;

    do_turn(board, selected, player);
}

void do_pn_random_move(struct pn_node **proot, struct player* player, int player_bit) {
    struct pn_node* root = *proot;

    struct board* board = root->board;
    if (board->move_location_tracker == -1) {
        printf("Error\n");
        exit(1);
    }

    if (board->move_location_tracker == 0) {
        board->turn++;
        return;
    }
    int selected = rand() % board->move_location_tracker;

    struct list* head = root->children.next;
    for (int i = 0; i < selected; i++) {
        head = head->next;
    }

    list_remove(head);
    pn_free(root);

    struct pn_node* new = container_of(head, struct pn_node, node);
    *proot = new;
}

void pn_generate_children(struct pn_node *root, struct player *original_player, int player_bit, int depth) {
    if (depth == 0) return;
    struct board* clone;
    struct board* board = root->board;

    // Get the color based on what player the passed player is
    int temp_pb = (original_player == &board->player2) << COLOR_SHIFT;
    long player_offset = ((void*)original_player) - ((void*)board);

    if (board->move_location_tracker != -1) {
        // This board was already initialized
        struct list* head = root->children.next;
        while (head != root->children.head) {
            struct pn_node* node = container_of(head, struct pn_node, node);
            struct player* player = (struct player*)(((void*) node->board) + player_offset);
            if (player == &node->board->player1) {
                pn_generate_children(node, &node->board->player2, player_bit, depth-1);
            } else {
                pn_generate_children(node, &node->board->player1, player_bit, depth-1);
            }

            head = head->next;
        }
        return;
    }

    generate_moves(board, original_player, temp_pb);

    // If there are no moves available, this tree branch is no good.
    if (board->move_location_tracker == 0) {
        root->to_disprove = 1 << 30;
        root->to_prove = 1 << 30;
        return;
    }
    if (root->node_type == PN_TYPE_OR) {
        root->to_prove = 1;
        root->to_disprove = board->move_location_tracker;
    } else {
        root->to_prove = board->move_location_tracker;
        root->to_disprove = 1;
    }


    for (int i = 0; i < board->move_location_tracker; i++) {
        // Copy board
        clone = malloc(sizeof(struct board));
        memcpy(clone, board, sizeof(struct board));

        struct pn_node* child = node_add_child(root, clone);

        // Extract player from struct based on offset in original struct.
        struct player* player = (struct player*)(((void*) clone) + player_offset);
        do_turn(clone, i, player);

        int won = finished_board(clone);
        if (won) {
            if ((won == 1 && player_bit == LIGHT)
                || (won == 2 && player_bit == DARK)) {
                // Positive terminal state
                root->to_prove--;
                child->to_prove = 0;
            } else {
                // Negative terminal state
                root->to_disprove--;
                child->to_disprove = 0;
            }
        } else {
            if (player == &clone->player1) {
                pn_generate_children(child, &clone->player2, player_bit, depth-1);
            } else {
                pn_generate_children(child, &clone->player1, player_bit, depth-1);
            }

            // If a child is fully proven or disproven, the root can add this to the tracker.
            root->to_prove -= (child->to_prove == 0);
            root->to_disprove -= (child->to_disprove == 0);
        }

        // If after winning, or children winning, proof number is given, return.
        if (root->to_prove == 0) {
            root->to_disprove = 1 << 30;
            return;
        }

        if (root->to_disprove == 0) {
            root->to_prove = 1 << 30;
            return;
        }
    }
}

void do_pn_tree_move(struct pn_node** proot, struct player* player, int player_bit) {
    int depth = 3;

    struct pn_node* root = *proot;

    pn_generate_children(root, player, player_bit, depth);
    if (root->board->move_location_tracker == 0) {
        root->board->turn++;
        return;
    }

    int index = rand() % root->board->move_location_tracker;

    struct list* head = root->children.next;
    int best_to_prove = 1 << 30;
    int iteration = 0;
    struct pn_node* best = NULL;
    struct pn_node* random = NULL;
    while (head != root->children.head) {
        struct pn_node* data = container_of(head, struct pn_node, node);
        if (data->to_prove == 0) {
            best = data;
            break;
        }

        if (index-- == 0) {
            random = data;
        }

        // Iterate list and track index
        iteration++;
        head = head->next;
    }

    // If there is no best solution, pick a random one
    if (best == NULL) {
        best = random;
    }

    // Remove this node from the root, and free the root.
    list_remove(&best->node);

    pn_free(root);

    // Overwrite the node.
    *proot = best;
}

void one_move_win_test(struct board* board) {
    // 1 move win for black setup
    board->tiles[(4 * BOARD_SIZE) + 4].type = L_QUEEN;
    board->queen1_position = (4 * BOARD_SIZE) + 4;
    board->tiles[(4 * BOARD_SIZE) + 3].type = L_BEETLE;
    board->tiles[(4 * BOARD_SIZE) + 5].type = L_BEETLE;
    board->tiles[(3 * BOARD_SIZE) + 4].type = L_BEETLE;
    board->tiles[(3 * BOARD_SIZE) + 5].type = D_ANT;
    board->tiles[(5 * BOARD_SIZE) + 4].type = L_BEETLE;
    board->tiles[(5 * BOARD_SIZE) + 5].type = L_BEETLE;
    board->player2.queens_left -= 1;
    board->player1.queens_left -= 1;
    board->player1.beetles_left -= 5;
    board->turn = 9;
    translate_board(board);
}

int main() {
    srand(time(NULL));

    struct board *board = init_board();

    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    int n_moves = 100;

    struct pn_node* tree = malloc(sizeof(struct pn_node));
    node_init(tree, PN_TYPE_OR);
    tree->board = board;

    for (int i = 0; i < n_moves; i++) {
        printf("At move %d\n", tree->board->turn);
        if (tree->board->turn % 2 == 0) {
            // Player 1
//            do_random_move(board, &board->player1, LIGHT);
            do_pn_tree_move(&tree, &tree->board->player1, LIGHT);
        } else {
            // Player 2
//            do_random_move(board, &board->player1, LIGHT);
            do_pn_random_move(&tree, &tree->board->player2, DARK);
        }

        int won = finished_board(tree->board);
        if (won) {
            printf("Player %d won in move %d\n", won, tree->board->turn);
            break;
        }
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    printf("msec: %.5f\n", (to_usec(end) - to_usec(start)) / 1e3);

    print_board(tree->board);

    free(tree->board);
    return 0;
}
