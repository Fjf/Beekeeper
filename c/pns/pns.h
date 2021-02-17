//
// Created by duncan on 12-02-21.
//

#ifndef HIVE_PNS_H
#define HIVE_PNS_H

#include <time.h>
#include "pn_tree.h"
#include "../engine/list.h"
#include "../engine/moves.h"


struct node *select_most_proving_node(struct node *root);
void PNS(struct node *root, int original_player_bit, time_t end_time);
void do_pn_tree_move(struct node **proot);
void do_pn_random_move(struct node **proot);
void set_proof_numbers(struct node *root, int original_player_bit);
int initialize_node(struct node *root, int original_player_bit);

#endif //HIVE_PNS_H
