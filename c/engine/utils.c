//
// Created by duncan on 30-03-21.
//

#include <limits.h>
#include "utils.h"
#include "moves.h"

int performance_testing(struct node* tree, int depth) {
    if (depth == 0) return 1;
    generate_moves(tree);

    struct list *head, *temp;
    int ret = 1;
    node_foreach_safe(tree, head, temp) {
        struct node* child = container_of(head, struct node, node);
        ret += performance_testing(child, depth-1);
        node_free(child);
    }
    return ret;
}