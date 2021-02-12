//
// Created by duncan on 12-02-21.
//

#ifndef HIVE_PNS_H
#define HIVE_PNS_H

#include <time.h>
#include "pn_tree.h"
#include "list.h"
#include "moves.h"


struct pn_node *select_most_proving_node(struct pn_node *root);
void PNS(struct pn_node *root, int original_player_bit, time_t end_time);
void do_pn_tree_move(struct pn_node **proot);
void do_pn_random_move(struct pn_node **proot);
void set_proof_numbers(struct pn_node *root, int original_player_bit);
int initialize_node(struct pn_node *root, int original_player_bit);

#endif //HIVE_PNS_H
