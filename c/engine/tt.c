//
// Created by duncan on 09-03-21.
//

#include <stdlib.h>
#include <stdio.h>
#include "tt.h"


void zobrist_init() {
    zobrist_table = malloc(BOARD_SIZE * BOARD_SIZE * N_UNIQUE_TILES * 2 * sizeof(long));
    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE * N_UNIQUE_TILES * 2; i++) {
        long r1 = rand(), r2 = rand();
        zobrist_table[i] = r1 + (r2 << 32);
    }
}

void zobrist_hash(struct board* board, int location, int old_location, int type) {
    int tile_type = type & TILE_MASK;
    int color = (type & COLOR_MASK) >> COLOR_SHIFT;
    int idx = tile_type + N_UNIQUE_TILES * color;
    if (location >= 0)
        board->zobrist_hash ^= zobrist_table[location * N_UNIQUE_TILES * 2 + idx];
    if (old_location >= 0)
        board->zobrist_hash ^= zobrist_table[old_location * N_UNIQUE_TILES * 2 + idx];
}

void tt_store(struct node* node, float score, char flag, int depth, int player) {
    long idx = node->board->zobrist_hash % TT_TABLE_SIZE + player * TT_TABLE_SIZE;
    struct tt_entry* entry = &tt_table[idx];

    if (entry->flag != -1) {
        // There was already an entry here TODO: Replacement scheme

        // Simple depth related replacement scheme.
//        if (entry->depth > depth) return;
    }
    long long sanity = (long long) (node->board->dark_queen_position) | node->board->light_queen_position;

    entry->sanity = sanity;
    entry->lock = node->board->zobrist_hash / TT_TABLE_SIZE;
    entry->score = score;
    entry->flag = flag;
    entry->depth = depth;
}

struct tt_entry* tt_retrieve(struct node* node, int player) {
    long idx = node->board->zobrist_hash % TT_TABLE_SIZE + player * TT_TABLE_SIZE;
    struct tt_entry* entry = &tt_table[idx];
    long long lock = node->board->zobrist_hash / TT_TABLE_SIZE;

    long long sanity = (long long) (node->board->dark_queen_position) << 32 | node->board->light_queen_position;
    if (entry->flag == -1 || entry->lock != lock || entry->sanity != sanity) return NULL;
    return entry;
}

void tt_init() {
    tt_table = malloc(TT_TABLE_SIZE * 2 * sizeof(struct tt_entry));
    for (int i = 0; i < TT_TABLE_SIZE; i++) {
        tt_table[i].flag = -1;
    }
}