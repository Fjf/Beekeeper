//
// Created by duncan on 09-03-21.
//

#ifndef HIVE_TT_H
#define HIVE_TT_H


#include "node.h"
#include "board.h"

#define TT_FLAG_UPPER 0
#define TT_FLAG_LOWER 1
#define TT_FLAG_TRUE 2


struct tt_entry {
    long long lock;
    float score;
    char flag;
    long sanity;
    unsigned char depth;
};

long* zobrist_table;
void zobrist_init();
void zobrist_hash(struct board* board, int location, int old_location, int type);


#define TT_TABLE_SIZE (1024*1024*128)

struct tt_entry* tt_table;
void tt_init();
void tt_store(struct node* node, float score, char flag, int depth, int player);
struct tt_entry* tt_retrieve(struct node* node, int player);


#endif //HIVE_TT_H
