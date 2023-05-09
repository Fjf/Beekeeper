//
// Created by duncan on 23-05-21.
//

#include <utils.h>
#include <omp.h>
#include "game.h"
#include <tree_impl.cpp>


int performance_testing(DefaultNode &tree, int depth) {
    if (depth == 0) {
        return 1;
    }
    tree.generate_moves();

    int ret = 1;
    for (DefaultNode &child : tree.children) {
        ret += performance_testing(child, depth - 1);
    }

    // Remove data
    tree.children.clear();
    return ret;
}


int main(int argc, char **argv) {
    int max_depth = 8;
    if (argc > 1) {
        max_depth = atoi(argv[1]);
    }

    printf("Running perft with depth %d on %d threads.\n", max_depth, 1);

    Game game = Game<DefaultNode>();
//    for (size_t i = 0; i < 10; i++) {
//        generate_children(game.root, 1e100);
//        game.random_move();
//    }
    srand(0);

    int last = 0;
    struct timespec start, end;
    printf("Depth    | Time (s)        | Nodes           | Knodes/sec    \n");
    printf("---------|-----------------|-----------------|--------------\n");
    for (int depth = 0; depth < max_depth; depth++) {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        int n = performance_testing(game.root, depth);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);

        int nodes = n - last;
        last = n;
        double time = (to_usec(end) - to_usec(start)) / 1e6;
        printf("%8d | %15.4f | %15d | (%.2f)\n", depth, time, nodes, (n / time) / 1000);
    }
}

/*q
 * C version of this code's performance below:
 *
Running perft with depth 8 on 1 threads.
Depth    | Time (s)        | Nodes           | Knodes/sec
---------|-----------------|-----------------|--------------
       0 |          0.0000 |               1 | (1438.85)
       1 |          0.0000 |               4 | (832.78)
       2 |          0.0000 |              16 | (2374.49)
       3 |          0.0001 |             240 | (3715.99)
       4 |          0.0009 |            3600 | (4460.57)
       5 |          0.0193 |           86040 | (4660.98)
       6 |          0.3631 |         2036580 | (5855.74)
       7 |          5.6310 |        30273650 | (5753.85)
 */

/*
 * Initial implementatoin
 *
Running perft with depth 7 on 1 threads.
Depth    | Time (s)        | Nodes           | Knodes/sec
---------|-----------------|-----------------|--------------
       0 |          0.0000 |               1 | (3401.36)
       1 |          0.0000 |               4 | (288.25)
       2 |          0.0000 |              16 | (1019.12)
       3 |          0.0002 |             240 | (1301.19)
       4 |          0.0037 |            3600 | (1038.33)
       5 |          0.0693 |           86040 | (1297.30)
       6 |          1.4433 |         2036518 | (1473.26)
 */

/*
 * Using list as children storage type
 *
Running perft with depth 7 on 1 threads.
Depth    | Time (s)        | Nodes           | Knodes/sec
---------|-----------------|-----------------|--------------
       0 |          0.0000 |               1 | (3984.06)
       1 |          0.0000 |               4 | (262.23)
       2 |          0.0000 |              16 | (1402.71)
       3 |          0.0002 |             240 | (1583.32)
       4 |          0.0021 |            3600 | (1809.24)
       5 |          0.0359 |           86040 | (2506.14)
       6 |          0.7899 |         2036515 | (2692.06)
 */


/*
 * Removed hash history (unused anyways)
 * Set the return type for points vector to be fixed size array.
 *
Running perft with depth 8 on 1 threads.
Depth    | Time (s)        | Nodes           | Knodes/sec
---------|-----------------|-----------------|--------------
       0 |          0.0000 |               1 | (6622.52)
       1 |          0.0000 |               4 | (950.39)
       2 |          0.0000 |              16 | (2199.41)
       3 |          0.0000 |             240 | (5986.10)
       4 |          0.0006 |            3600 | (6434.91)
       5 |          0.0150 |           84600 | (5911.21)
       6 |          0.3649 |         1970428 | (5642.43)
       7 |          5.7316 |        27631300 | (5180.10)

 */

/*
 * Use default board constructor instead of what I had.
 *
 Running perft with depth 8 on 1 threads.
Depth    | Time (s)        | Nodes           | Knodes/sec
---------|-----------------|-----------------|--------------
       0 |          0.0000 |               1 | (6060.61)
       1 |          0.0000 |               4 | (946.43)
       2 |          0.0000 |              16 | (2135.45)
       3 |          0.0000 |             240 | (6195.40)
       4 |          0.0006 |            3600 | (6831.74)
       5 |          0.0142 |           84600 | (6232.21)
       6 |          0.3471 |         1970428 | (5932.06)
       7 |          5.4063 |        27631300 | (5491.79)

 */

/*
 *
 *
Running perft with depth 8 on 1 threads.
Depth    | Time (s)        | Nodes           | Knodes/sec
---------|-----------------|-----------------|--------------
       0 |          0.0000 |               1 | (844.59)
       1 |          0.0000 |               4 | (150.01)
       2 |          0.0001 |              16 | (346.47)
       3 |          0.0002 |             240 | (1136.80)
       4 |          0.0033 |            3600 | (1186.35)
       5 |          0.0407 |           84600 | (2174.48)
       6 |          0.3140 |         1970428 | (6557.73)
       7 |          4.8454 |        27638820 | (6129.04)

 Running perft with depth 8 on 1 threads.
Depth    | Time (s)        | Nodes           | Knodes/sec
---------|-----------------|-----------------|--------------
       0 |          0.0000 |               1 | (4237.29)
       1 |          0.0000 |               4 | (933.01)
       2 |          0.0000 |              16 | (11000.52)
       3 |          0.0000 |             240 | (8616.42)
       4 |          0.0003 |            3600 | (12357.73)
       5 |          0.0079 |           86040 | (11330.25)
       6 |          0.2020 |         2036580 | (10525.36)
       7 |          3.9714 |        30273650 | (8158.38)


 */