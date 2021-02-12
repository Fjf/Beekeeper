#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "board.h"
#include "moves.h"
#include "pns/pn_tree.h"
#include "mm/mm.h"
#include "pns/pns.h"
#include "list.h"


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

int main() {
    srand(time(NULL));

    struct board *board = init_board();

    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    int n_moves = 100;

    struct node *tree = malloc(sizeof(struct node));
    pn_init(tree, PN_TYPE_OR);
    tree->board = board;

    for (int i = 0; i < n_moves; i++) {
        printf("At move %d\n", tree->board->turn);
        int player = tree->board->turn % 2;
        if (player == 0) {
            // Player 1
            minimax(&tree);
        } else {
            // Player 2
            do_pn_random_move(&tree);
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

    print_board(tree->board);

    free(tree->board);
    return 0;
}
