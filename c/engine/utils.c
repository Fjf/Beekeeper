//
// Created by duncan on 30-03-21.
//

#include <stdlib.h>
#include <stdio.h>
#include "utils.h"
#include "moves.h"
#include <unistd.h>
#include <string.h>
#include <omp.h>

int performance_testing(struct node *tree, int depth) {
    if (depth == 0) return 1;
    generate_moves(tree, 0);

    struct list *head, *temp;
    int ret = 1;
    node_foreach_safe(tree, head, temp) {
        struct node *child = container_of(head, struct node, node);
        ret += performance_testing(child, depth - 1);
        node_free(child);
    }
    return ret;
}

int performance_testing_parallel(struct node *tree, int depth, int par_depth) {
    if (depth == 0) return 1;
    generate_moves(tree, 0);

    int ret = 1;
    struct list *head, *temp;

    if (par_depth == 0) {
#pragma omp parallel
#pragma omp single
        node_foreach_safe(tree, head, temp) {
#pragma omp task firstprivate(head)
            {
                struct node *child = container_of(head, struct node, node);
                int r = performance_testing(child, depth - 1);

                #pragma omp atomic
                ret += r;

                #pragma omp critical (free)
                node_free(child);
            }
        }
    } else {
        node_foreach_safe(tree, head, temp) {
            struct node *child = container_of(head, struct node, node);
            ret += performance_testing_parallel(child, depth - 1, par_depth - 1);
            node_free(child);
        }
    }
    return ret;
}


void parse_args(int argc, char *const *argv, struct arguments *arguments) {
    int c;
    int errflg = 0;
    struct player_arguments *pa;
    while ((c = getopt(argc, argv, ":A:a:C:c:t:T:e:E:PpFfv")) != -1) {
        if (c >= 97) {
            pa = &arguments->p2;
            c -= 32;
        } else {
            pa = &arguments->p1;
        }
        switch (c) {
            case 'V':
                arguments->p1.verbose = true;
                arguments->p2.verbose = true;
                break;
            case 'E':
                pa->evaluation_function = atoi(optarg);
                break;
            case 'A':
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
            case 'C':
                pa->mcts_constant = atof(optarg);
                break;
            case 'T':
                pa->time_to_move = atof(optarg);
                break;
            case 'P':
                pa->prioritization = true;
                break;
            case 'F':
                pa->first_play_urgency = true;
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

void print_args(struct arguments *arguments) {
    for (int i = 0; i < 2; i++) {
        struct player_arguments* pa = &arguments->p1;
        if (i == 1) {
            pa = &arguments->p2;
        }

        char *algo = alg_to_str(pa->algorithm);
        char *eval = eval_to_str(pa->evaluation_function);
        printf("Player %d:\n"
               "\tAlgorithm: %s\n"
               "\tMinimax Eval: %s\n"
               "\tMCTS Constant: %.2f\n"
               "\tTime-to-move: %.2f\n"
               "\tMCTS-Prioritization: %d\n"
               "\tMCTS-FirstPlayUrgency: %d\n", i+1, algo, eval, pa->mcts_constant, pa->time_to_move, pa->prioritization,
               pa->first_play_urgency);
    }
}
