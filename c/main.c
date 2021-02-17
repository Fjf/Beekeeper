#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "engine/board.h"
#include "engine/moves.h"
#include "pns/pn_tree.h"
#include "mm/mm.h"
#include "pns/pns.h"
#include "engine/list.h"
#include "timing/timing.h"


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

    generate_children(root, 1, time(NULL) + 10);

    char move[20], cmove[20];

    while (true) {
        printf("Type a move:\n");
        fgets(move, 20, stdin);

        printf("%s", move);

        list_foreach(root, head) {
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
        list_foreach(root, head) {
            struct node *child = container_of(head, struct node, node);
            string_move(child, cmove);
            printf("%s", cmove);
        }
    }
}

int main() {
//    srand(time(NULL));

    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    initialize_timer("out.txt");
    int n_moves = 10;

    struct node* tree = game_init();

    for (int i = 0; i < n_moves; i++) {
        int player = tree->board->turn % 2;
        if (player == 0) {
            // Player 1
//            manual(&tree);
            minimax(&tree);
        } else {
            // Player 2
//            manual(&tree);
            minimax(&tree);
        }

        print_board(tree->board);

        int won = finished_board(tree->board);
        if (won) {
            printf("Player %d won in move %d\n", won, tree->board->turn);
            break;
        }
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    printf("msec: %.5f\n", (to_usec(end) - to_usec(start)) / 1e3);

    finalize_timer();

    free(tree->board);
    return 0;
}
