//
// Created by duncan on 23-05-21.
//

#include <utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <moves.h>
#include <omp.h>

int main(int argc, char** argv) {
    int max_depth = 4;
    if (argc > 1) {
        max_depth = atoi(argv[1]);
    }
    omp_set_num_threads(4);

    printf("Running perft with depth %d on %d threads.\n", max_depth, omp_get_max_threads());

    struct node* tree = game_init();

    random_moves(&tree, 10);

    int last = 0;
    struct timespec start, end;
    printf("Depth    | Time (s)        | Nodes           | Knodes/sec    \n");
    printf("---------|-----------------|-----------------|--------------\n");
    for (int depth = 0; depth < max_depth; depth++) {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        int n = performance_testing(tree, depth);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);

        int nodes = n - last;
        last = n;
        double time = (to_usec(end) - to_usec(start)) / 1e6;
        printf("%8d | %15.4f | %15d | (%.2f)\n", depth, time, nodes, (n/time) / 1000);
    }
}