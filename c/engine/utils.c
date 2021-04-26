//
// Created by duncan on 30-03-21.
//

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "moves.h"
#include <unistd.h>
#include <string.h>

int performance_testing(struct node* tree, int depth) {
    if (depth == 0) return 1;
    generate_moves(tree, 0);

    struct list *head, *temp;
    int ret = 1;
    node_foreach_safe(tree, head, temp) {
        struct node* child = container_of(head, struct node, node);
        ret += performance_testing(child, depth-1);
        node_free(child);
    }
    return ret;
}



void parse_args(int argc, char* const* argv, struct arguments* arguments) {
    int c;
    int errflg = 0;
    struct player_arguments* pa;
    while ((c = getopt(argc, argv, ":P:p:c:t:v")) != -1) {
        switch(c) {
            case 'v':
                arguments->p1.verbose = true;
                arguments->p2.verbose = true;
                break;
            case 'p':
            case 'P':
                if (c == 'p')
                    pa = &arguments->p2;
                else
                    pa = &arguments->p1;
                if (strcmp(optarg, "mcts") == 0) {
                    pa->algorithm = ALG_MCTS;
                } else if (strcmp(optarg, "mm") == 0) {
                    pa->algorithm = ALG_MM;
                } else if (strcmp(optarg, "manual") == 0) {
                    pa->algorithm = ALG_MANUAL;
                } else if (strcmp(optarg, "random") == 0) {
                    pa->algorithm = ALG_RANDOM;
                } else {
                    fprintf(stderr, "Unknown algorithm '%s'.\n", optarg);
                    errflg++;
                }
                break;
            case 'c':
                arguments->p1.mcts_constant = atoi(optarg);
                arguments->p2.mcts_constant = atoi(optarg);
                break;
            case 't':
                arguments->p1.time_to_move = atoi(optarg);
                arguments->p2.time_to_move = atoi(optarg);
                break;
            case ':':       /* -f or -o without operand */
                fprintf(stderr,
                        "Option -%c requires an operand\n", optopt);
                errflg++;
                break;
            case '?':
                fprintf(stderr,
                        "Unrecognized option: '-%c'\n", optopt);
                errflg++;
        }
    }

    if (errflg > 0) {
        fprintf(stderr, "Invalid arguments found, exiting.\n");
        exit(1);
    }
}

void print_args(struct arguments* arguments) {
    struct player_arguments* pa = &arguments->p1;
    char* algo = alg_to_str(pa->algorithm);
    printf("Player 1:\n"
           "\tAlgorithm: %s\n"
           "\tMCTS Constant: %.5f\n"
           "\tTime-to-move: %d\n"
            , algo, pa->mcts_constant, pa->time_to_move);

    pa = &arguments->p2;
    algo = alg_to_str(pa->algorithm);
    printf("Player 2:\n"
           "\tAlgorithm: %s\n"
           "\tMCTS Constant: %.5f\n"
           "\tTime-to-move: %d\n"
            , algo, pa->mcts_constant, pa->time_to_move);
}
