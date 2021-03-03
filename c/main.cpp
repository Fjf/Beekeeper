#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>

#include <iostream>

#include "engine/board.hpp"
#include "engine/moves.hpp"
#include "engine/list.hpp"
#include "mm/mm.hpp"
#include "mm/evaluation.hpp"

#define KB (1024ull)
#define MB (1024ull * KB)
#define GB (1024ull * MB)
#define MAX_MEMORY (500ull * MB)
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

void manual(struct node **proot) {
    struct node *root = *proot;
    struct list *head;

    generate_children(root, time(NULL) + 10);

    char move[20], cmove[20];

    while (true) {
        printf("Type a move:\n");
        fgets(move, 20, stdin);

        printf("%s", move);

        node_foreach(root, head) {
            struct node *child = container_of(head, struct node, node);
            string_move(child, cmove);

            int res = strcmp(move, cmove);
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
            string_move(child, cmove);
            printf("%s", cmove);
        }
    }
}

int main() {
#ifdef TESTING
    srand(0);
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

    // Initialize board history list to identify repeats.
    list_init(&board_history);
    initialize_points_around();

    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

#ifdef TESTING
    int n_moves = 10;
#else
    int n_moves = 100;
#endif

    struct node *tree = game_init();

    for (int i = 0; i < n_moves; i++) {
        int player = tree->board->turn % 2;
        if (player == 0) {
            // Player 1
//            manual(&tree);
            mm_evaluate = mm_evaluate_expqueen;
            minimax(&tree);
        } else {
            // Player 2
//            manual(&tree);
            mm_evaluate = mm_evaluate_linqueen;
            minimax(&tree);
        }

        print_board(tree->board);

        int won = finished_board(tree->board);
        if (won) {
            printf("Player %d won in move %d\n", won, tree->board->turn);
            break;
        }

        // Copy node except for children
        // Doing this forces recomputation of tree every iteration.
        struct node *copy = mm_init();
        memcpy(&copy->move, &tree->move, sizeof(struct move));
        copy->board = init_board();
        memcpy(copy->board, tree->board, sizeof(struct board));
        memcpy(copy->data, tree->data, sizeof(struct mm_data));

        node_free(tree);
        tree = copy;
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    printf("msec: %.5f\n", (to_usec(end) - to_usec(start)) / 1e3);

    free(tree->board);
    return 0;
}
