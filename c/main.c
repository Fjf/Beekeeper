#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <utils.h>
#include <assert.h>
#include <omp.h>
#include "engine/tt.h"
#include "engine/board.h"
#include "engine/moves.h"
#include "pns/pn_tree.h"
#include "mm/mm.h"
#include "pns/pns.h"
#include "engine/list.h"
#include "mcts/mcts.h"

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

struct node *manual(struct node *root) {
    struct list *head;

    generate_children(root, time(NULL) + 1000000000, 0);

    char move[20];
    while (true) {
        printf("Type a move:\n");
        char *fgetres = fgets(move, 20, stdin);
        if (fgetres == NULL) continue; // NOTE: Maybe not very good

        printf("%s", move);

        node_foreach(root, head) {
            struct node *child = container_of(head, struct node, node);
            char *cmove = string_move(child);
            int res = strcmp(move, cmove);
            free(cmove);
            if (res == 0) {
                return child;
            }
        }
        printf("That is not a valid move!\nPick one from:\n");
        node_foreach(root, head) {
            struct node *child = container_of(head, struct node, node);
            print_move(child);
        }
    }
}

void ptest(struct node *tree) {
    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    int nodes = performance_testing(tree, 8);

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    double sec = (to_usec(end) - to_usec(start)) / 1e6;

    printf("%.2f knodes/s\n", (nodes / 1000.) / sec);
}

/*
 *          (1000 samples)
 *  -------------------------------------------------------
 * |  Move limit  |    w/l/d    | total time |  playout/s  |
 * |--------------|-------------|------------|-------------|
 * | N_TURNS 50   | 20/24/956   | (5616ms)   |    178      |
 * | N_TURNS 100  | 49/39/912   | (14188ms)  |    70       |
 * | N_TURNS 200  | 79/83/838   | (30740ms)  |    32       |
 * | N_TURNS 500  | 104/127/769 | (78722ms)  |    12       |
 * | N_TURNS 1000 | 195/198/607 | (150591ms) |    6        |
 * | N_TURNS 2000 | 288/301/411 | (278767ms) |    3        |
 *  -------------------------------------------------------
 *  Every sample is a full playout starting from an empty
 *   (initial) board state.
 *

 https://www.aatbio.com/tools/linear-logarithmic-semi-log-regression-online-calculator

 x,    y
 50,   44
 100,  88
 200,  162
 500,  231
 1000, 393
 2000, 589

 linear log regression gives; (datapoints look logarithmic)
 y = 3.7 * x^(0.676)

  50,5616
 100,14188
 200,30470
 500,78722
1000,150591
2000,278767

 lin regr gives;
 y = 140.18*x + 3109.94

 *
 */
void mcts_test(struct node *tree) {
    struct mcts_data *data = tree->data;
    data->keep = true;

    time_t t = time(NULL) + 100000;

    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

    int w = 0, l = 0, d = 0;
    for (int i = 0; i < 1000; i++) {
        int result = mcts_playout_prio(tree, t);
        if (result == 1) w++;
        else if (result == 2) l++;
        else if (result == 3) d++;
    }
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    printf("msec: %.5f\n", (to_usec(end) - to_usec(start)) / 1e3);
    printf("playouts p sec: %.5f\n", 1000. / ((to_usec(end) - to_usec(start)) / 1e6));
    printf("%d & %d & %d\n", w, l, d);

}


int main(int argc, char **argv) {
    struct arguments arguments = {0};
    arguments.p1.time_to_move = arguments.p2.time_to_move = 0.1;
    parse_args(argc, argv, &arguments);
    print_args(&arguments);

    // Compute max amount of nodes available in memory.
    printf("Can hold %llu nodes in memory.\n", max_nodes);

    // Register mcts node add function
    dedicated_add_child = mcts_add_child;
    dedicated_init = mcts_init;

#ifdef TESTING
    int n_children = 1;
#else
    int n_moves = MAX_TURNS - 1;
#endif

    struct node *tree = game_init();

    assert(tree->board != NULL);

//    setup_puzzle(&tree, 4);
//    print_board(tree->board);
//    exit(0);

//    print_board(tree->board);

//    for (int i = 0; i < 30; i++) {
//        print_board(tree->board);
//
//        random_moves(&tree, 1);
//    }

//    mcts_test(tree);
//
//    exit(1);

//    print_board(tree->board);

    omp_set_num_threads(1);

    struct timespec start, end;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
    for (int i = 0; i < n_moves; i++) {
        int player = tree->board->turn % 2;

        struct player_arguments *pa = (player == 0 ? &arguments.p1 : &arguments.p2);

        struct node *child;
        if (pa->algorithm == ALG_MCTS) {
            child = mcts(tree, pa);
        } else if (pa->algorithm == ALG_MM) {
            child = minimax(tree, pa);
        } else if (pa->algorithm == ALG_RANDOM) {
            child = random_moves(tree, 1);
        } else if (pa->algorithm == ALG_MANUAL) {
            child = manual(tree);
        } else {
            fprintf(stderr, "Invalid algorithm passed, exiting.\n");
            exit(1);
        }

        // Clean up nodes
        list_remove(&child->node);
        node_free(tree);
        tree = child;

        if (pa->verbose)
            print_board(tree->board);

        int won = finished_board(tree->board);
        if (won) {
            print_board(tree->board);
            printf("Player %d won in move %d\n", won, tree->board->turn);
            break;
        }
    }

    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
    printf("msec: %.5f\n", (to_usec(end) - to_usec(start)) / 1e3);

    free(tree->board);
    return 0;
}
