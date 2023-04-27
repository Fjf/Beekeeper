//
// Created by duncan on 09-03-21.
//


#include "tt.h"
#include "tree.h"

// Global defines for transposition table.
struct tt_entry *tt_table = nullptr;
int64_t *zobrist_table = nullptr;


void zobrist_init() {
    zobrist_table = static_cast<int64_t *>(malloc(BOARD_SIZE * BOARD_SIZE * N_UNIQUE_TILES * 2 * sizeof(int64_t)));
    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE * N_UNIQUE_TILES * 2; i++) {
        int32_t r1 = rand(), r2 = rand();
        zobrist_table[i] = r1 + ((int64_t) r2 << 32);
    }
}

void zobrist_hash(Board &board, const Position &location, const Position &old_location, int type) {
    int tile_type = type & TILE_MASK;
    int color = (type & COLOR_MASK) >> COLOR_SHIFT;
    int idx = tile_type + N_UNIQUE_TILES * color;
    if (location.x >= 0)
        board.zobrist_hash ^= zobrist_table[location.flat_index() * N_UNIQUE_TILES * 2 + idx];
    if (old_location.x >= 0)
        board.zobrist_hash ^= zobrist_table[old_location.flat_index() * N_UNIQUE_TILES * 2 + idx];
}

void tt_store(Node &node, float score, char flag, int depth, int player) {
    int64_t idx = node.board.zobrist_hash % TT_TABLE_SIZE + player * TT_TABLE_SIZE;
    struct tt_entry *entry = &tt_table[idx];

    if (entry->flag != -1) {
        // There was already an entry here TODO: Replacement scheme

        // Simple depth related replacement scheme.
//        if (entry->depth > depth) return;
    }
    unsigned char bit_length = 10;
    int64_t sanity = ((((node.board.dark_queen.flat_index() << bit_length)
                        | node.board.light_queen.flat_index()) << bit_length)
                      | (node.board.min.flat_index()) << bit_length
                      | (node.board.max.flat_index()) << bit_length);
    entry->sanity = sanity;
    entry->lock = node.board.zobrist_hash / TT_TABLE_SIZE;
    entry->score = score;
    entry->flag = flag;
    entry->depth = depth;
}

struct tt_entry *tt_retrieve(Node &node, int player) {
    int64_t idx = node.board.zobrist_hash % TT_TABLE_SIZE + player * TT_TABLE_SIZE;
    struct tt_entry *entry = &tt_table[idx];
    int64_t lock = node.board.zobrist_hash / TT_TABLE_SIZE;

    unsigned char bit_length = 10;
    int64_t sanity = ((((node.board.dark_queen.flat_index() << bit_length)
                        | node.board.light_queen.flat_index()) << bit_length)
                      | (node.board.min.flat_index()) << bit_length
                      | (node.board.max.flat_index()) << bit_length);
    if (entry->flag == -1 || entry->lock != lock || entry->sanity != sanity) return nullptr;

    return entry;
}

void tt_init() {
    tt_table = static_cast<tt_entry *>(malloc(TT_TABLE_SIZE * 2 * sizeof(struct tt_entry)));
    for (int i = 0; i < TT_TABLE_SIZE; i++) {
        tt_table[i].flag = -1;
    }
}