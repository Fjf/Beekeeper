//
// Created by duncan on 09-03-21.
//

#ifndef HIVE_TT_H
#define HIVE_TT_H


#include "board.h"
#include "tree.h"

#define TT_FLAG_UPPER 0
#define TT_FLAG_LOWER 1
#define TT_FLAG_TRUE 2


struct tt_entry {
    int64_t lock;
    float score;
    char flag;
    int64_t sanity;
    unsigned char depth;
};

extern int64_t* zobrist_table;
void zobrist_init();
void zobrist_hash(Board& board, Position& location, Position& old_location, int type);


//#define TT_TABLE_SIZE (1024*1024*256)
#define TT_TABLE_SIZE (1)

extern struct tt_entry* tt_table;

void tt_init();
void tt_store(Node& node, float score, char flag, int depth, int player);
struct tt_entry* tt_retrieve(Node& node, int player);


#endif //HIVE_TT_H
