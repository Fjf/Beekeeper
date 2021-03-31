#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <utils.h>
#include "engine/tt.h"
#include "engine/board.h"
#include "engine/moves.h"
#include "pns/pn_tree.h"
#include "mm/mm.h"
#include "pns/pns.h"
#include "engine/list.h"
#include "timing/timing.h"
#include "mm/evaluation.h"
#include "mcts/mcts.h"

#define MAX_MEMORY (4ull * GB)


/* winrate: PN vs random (fixed depth PN no disproof)
 *  PN  -  draws  - random
 *   5  -    3    -   2
 */

/*
 * winrate PN  vs random (including non-proof and dynamic depth search)
 *   PN  -  draws - random
 *    5  -    5   -   0
 */

/*
 * Winrate minimax vs random (evaluation takes into account n-tiles surrounding queen
 *                            and whether or not it won)
 *   MM  -  draws - random
 *    10  -    0   -   0
 */

void manual(struct node** proot) {
    struct node* root = *proot;
    struct list* head;

    generate_children(root, time(NULL) + 1000000000);

    char move[20];
    while (true) {
        printf("Type a move:\n");
        char* fgetres = fgets(move, 20, stdin);
        if (fgetres == NULL) continue; // NOTE: Maybe not very good

        printf("%s", move);

        node_foreach(root, head) {
            struct node *child = container_of(head, struct node, node);
            char* cmove = string_move(child);
            int res = strcmp(move, cmove);
            free(cmove);
            if (res == 0) {
                list_remove(&child->node);
                node_free(root);
                *proot = child;
                return;
            }
        }
        printf("That is not a valid move!\nPick one from:\n");
        node_foreach(root, head) {
            struct node *child = container_of(head, struct node, node);
            print_move(child);
        }
    }
}


void random_moves(struct node **tree, int n_moves) {
    for (int i = 0; i < n_moves; i++) {
        struct node* node = *tree;

        generate_children(node, (time_t) INT_MAX);
        int choice = rand() % node->board->move_location_tracker;
        struct list* head;
        int n = 0;
        node_foreach(node, head) {
            if (choice == n++) {
                break;
            }
        }

        struct node* child = container_of(head, struct node, node);
        list_remove(head);
        node_free(node);
        *tree = child;
    }
}

void ptest(struct node *tree) {
    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    int nodes = performance_testing(tree, 3);

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    double sec = (to_usec(end) - to_usec(start)) / 1e6;

    printf("%.2f knodes/s\n", (nodes / 1000.) / sec);
}



int main() {
#ifdef TESTING
    srand(11287501);
    printf("Running test case (forced depth of 3, no randomization)\n");
#else
    srand(time(NULL));
#endif

    // Compute max amount of nodes available in memory.
    max_nodes = MAX_MEMORY / (sizeof(struct board) + sizeof(struct node) + sizeof(struct mm_data));
    n_nodes = 0;
    printf("Can hold %llu nodes in memory.\n", max_nodes);

    // Set the evaluation function
    mm_evaluate = mm_evaluate_expqueen;

    // Set the initial add child function to minimax add child
    dedicated_add_child = mm_add_child;


#ifdef TESTING
    int n_moves = 1;
#else
    int n_moves = 130;
#endif

    struct node* tree = game_init();

    for (int i = 0; i < 30; i++) {
//        ptest(tree);
        print_board(tree->board);

        random_moves(&tree, 1);
    }

    exit(1);

    print_board(tree->board);

    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
    for (int i = 0; i < n_moves; i++) {
        int player = tree->board->turn % 2;
        if (player == 0) {
            // Player 1
//            manual(&tree);
            mcts(&tree);
        } else {
            // Player 2
//            manual(&tree);
            mcts(&tree);
        }

        print_board(tree->board);
        printf("%d %d %d %d\n", tree->board->min_x, tree->board->min_y, tree->board->max_x, tree->board->max_y);


        int won = finished_board(tree->board);
        if (won) {
            printf("Player %d won in move %d\n", won, tree->board->turn);
            break;
        }

        // Copy node except for children
        // Doing this forces re-computation of tree every iteration.
        struct node* copy = dedicated_init();
        memcpy(&copy->move, &tree->move, sizeof(struct move));
        copy->board = init_board();
        memcpy(copy->board, tree->board, sizeof(struct board));
        tree->board->move_location_tracker = 0;
        node_free(tree);
        tree = copy;
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    printf("msec: %.5f\n", (to_usec(end) - to_usec(start)) / 1e3);

    free(tree->board);
    return 0;
}
