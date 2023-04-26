#include <algorithm>
#include <cstring>
#include <list>
#include "board.h"

int Board::finished() {
    int res = 0;
    if (light_queen.x != -1) {
        // Check queen 1
        if (is_surrounded(light_queen)) {
            res = 2;
        }
    }

    if (dark_queen.x != -1) {
        // Check queen 2
        if (is_surrounded(dark_queen)) {
            if (res == 0)
                res = 1;
            else
                res = 3;
        }
    }

    // Check draw by repetition.
//    int sum = 0;
//    for (int i = 0; i < board->turn; i++) {
//        if (board->hash_history[i] == board->zobrist_hash) sum++;
//
//        if (sum == 3) res = 3;
//    }

    // Draw due to turn limit
    // TODO: Set turn limit in game instead of hardcoded
    if (turn >= 10000 - 1) {
        int l = count_tiles_around(light_queen);
        int d = count_tiles_around(dark_queen);
        return d > l ? 1 : 2;
    }

    return res;
}

std::string Board::to_string() {
    std::string response;
    for (int x = 0; x < BOARD_SIZE; x++) {
        response += "  ";
    }
    for (int x = 0; x < BOARD_SIZE; x++) {
        response += "---";
    }
    response += "\n";

    for (int y = 0; y < BOARD_SIZE; y++) {
        // Add spaces padding for the display
        for (int x = 0; x < BOARD_SIZE - y - 1; x++) {
            response += "  ";
        }
        response += "/";

        for (int x = 0; x < BOARD_SIZE; x++) {
            response += " ";

            unsigned char tile = tiles[y][x];
            int type = tile & (COLOR_MASK | TILE_MASK);
            int n = (tile & NUMBER_MASK) >> NUMBER_SHIFT;
            if (type == EMPTY) {
                response += " ";
            } else if (type == L_ANT) {
                response += "A";
            } else if (type == L_GRASSHOPPER) {
                response += "G";
            } else if (type == L_BEETLE) {
                response += "B";
            } else if (type == L_SPIDER) {
                response += "S";
            } else if (type == L_QUEEN) {
                response += "Q";
            } else if (type == D_ANT) {
                response += "a";
            } else if (type == D_GRASSHOPPER) {
                response += "g";
            } else if (type == D_BEETLE) {
                response += "b";
            } else if (type == D_SPIDER) {
                response += "s";
            } else if (type == D_QUEEN) {
                response += "q";
            }

            // Add number after tile
            if (n > 0) {
                response += std::to_string(n);
            } else {
                response += " ";
            }
        }

        response += "/\n";
    }
    response += " ";
    for (int x = 0; x < BOARD_SIZE; x++) {
        response += "---";
    }
    response += "\n";
    return response;
}

void Board::get_min_x_y() {
    Position lower = Position(min.x, min.y);
    min.x = min.y = BOARD_SIZE;
    for (int y = lower.y; y < BOARD_SIZE; y++) {
        for (int x = lower.x; x < min.x; x++) {
            unsigned char tile = tiles[y][x];
            if (tile == EMPTY) continue;

            // Set min_y to the first non-empty tile row
            if (min.y == BOARD_SIZE) min.y = y;

            if (x < min.x) min.x = x;
        }
    }
}

void Board::get_max_x_y() {
    Position upper = Position(max.x, max.y);
    max.x = max.y = 0;
    for (int y = upper.y; y >= 0; y--) {
        for (int x = upper.x; x >= max.x; x--) {
            unsigned char tile = tiles[y][x];
            if (tile == EMPTY) continue;

            // Set min_y to the first non-empty tile row
            if (max.y == 0) max.y = y;

            if (x > max.x) max.x = x;
        }
    }
}

void Board::center() {
    int src_begin = min.y * BOARD_SIZE + min.x;
    int src_end = max.y * BOARD_SIZE + max.x;
    int size = (src_end - src_begin) + 1;

    Position dest = Position(
            (BOARD_SIZE / 2) - (max.x - min.x + 1) / 2,
            (BOARD_SIZE / 2) - (max.y - min.y + 1) / 2
    );

    Position translate = Position(dest.x - min.x, dest.y - min.y);

    // Move all the tile tracking structs the same amount as the rest of the board.
    int translate_offset = (translate.flat_index()) - src_begin;
    if (light_queen.x != -1) {
        light_queen.x += translate.x;
        light_queen.y += translate.y;
    }
    if (dark_queen.x != -1) {
        dark_queen.x += translate.x;
        dark_queen.y += translate.y;
    }
    for (auto &i : stack) {
        if (i.position.x != -1) {
            i.position.x += translate.x;
            i.position.y += translate.y;
        }
    }

    size_t dest_begin = translate.flat_index();

    memmove(
            tiles + dest_begin,
            tiles + src_begin * sizeof(char),
            size * sizeof(char)
    );
    memset(tiles, 0, dest_begin);
    memset(tiles + dest_begin + size, 0, BOARD_SIZE - (dest_begin + size));

    min.x += translate.x;
    max.x += translate.x;
    min.y += translate.y;
    max.y += translate.y;
}

int Board::count_tiles_around(Position &position) {
    int count = 0;
    auto points = position.get_points_around();
    for (Position &point : points) {
        if (tiles[point.y][point.x] != EMPTY) {
            count++;
        }
    }
    return count;
}

bool Board::is_surrounded(Position &position) {
    auto points = position.get_points_around();
    for (Position &point : points) {
        if (tiles[point.y][point.x] == EMPTY) {
            return false;
        }
    }
    return true;
}


int Board::sum_hive_tiles() {
    return N_TILES * 2 - (
            players[0].queens_left +
            players[0].grasshoppers_left +
            players[0].beetles_left +
            players[0].ants_left +
            players[0].spiders_left +
            players[1].spiders_left +
            players[1].ants_left +
            players[1].queens_left +
            players[1].grasshoppers_left +
            players[1].beetles_left) -
           n_stacked;
}


Board::tile_stack *Board::get_from_stack(Position &position, bool pop) {
    /*
     * Returns the highest tile in the stack, or NULL if no tile is found.
     * If pop is true, the tile will be removed from the stack.
     * Otherwise, just the tile will be returned.
     */
    struct tile_stack *highest_tile = nullptr;
    int highest_idx = -1;
    for (auto &i : stack) {
        if (i.position == position
            && highest_idx < i.z) {
            highest_idx = i.z;
            highest_tile = &i;
        }
    }
    if (highest_tile == nullptr) {
        return nullptr;
    }

    if (pop) {
        // Remove from list
        highest_tile->position.x = -1;
        n_stacked--;
    }
    return highest_tile;
}

void Board::initialize() {
    memset(tiles, 0, sizeof(tiles));
    memset(free, 0, sizeof(free));

    turn = 0;

    players[0] = {N_BEETLES, N_GRASSHOPPERS, N_QUEENS, N_ANTS, N_SPIDERS};
    players[1] = {N_BEETLES, N_GRASSHOPPERS, N_QUEENS, N_ANTS, N_SPIDERS};

    light_queen = {-1, -1};
    dark_queen = {-1, -1};

    min = {BOARD_SIZE / 2, BOARD_SIZE / 2};
    max = {BOARD_SIZE / 2, BOARD_SIZE / 2};

    n_stacked = 0;

    memset(stack, 0, sizeof(stack));

    zobrist_hash = 0;
//    std::vector<long long> hash_history = std::vector<long long>();

    has_updated = false;
}
