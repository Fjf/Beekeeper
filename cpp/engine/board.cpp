#include <algorithm>
#include <cstring>
#include <list>
#include "board.h"

int Board::finished() {
    int res = UNDECIDED;
    if (light_queen.x != -1) {
        // Check queen 1
        if (is_surrounded(light_queen)) {
            res = LIGHT_WON;
        }
    }

    if (dark_queen.x != -1) {
        // Check queen 2
        if (is_surrounded(dark_queen)) {
            if (res == 0)
                res = DARK_WON;
            else
                res = DRAW;
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
    if (turn >= MAX_TURNS - 1) {
        return DRAW;
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
    for (int8_t y = lower.y; y < BOARD_SIZE; y++) {
        for (int8_t x = lower.x; x < min.x; x++) {
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
    for (int8_t y = upper.y; y >= 0; y--) {
        for (int8_t x = upper.x; x >= max.x; x--) {
            unsigned char tile = tiles[y][x];
            if (tile == EMPTY) continue;

            // Set min_y to the first non-empty tile row
            if (max.y == 0) max.y = y;

            if (x > max.x) max.x = x;
        }
    }
}

void Board::center() {
    uint16_t src_begin = min.flat_index();
    uint16_t src_end = max.flat_index();
    uint16_t size = (src_end - src_begin) + 1;
    Position dest = Position(
            (BOARD_SIZE / 2) - (max.x - min.x + 1) / 2,
            (BOARD_SIZE / 2) - (max.y - min.y + 1) / 2
    );

    Position translate = Position(dest.x - min.x, dest.y - min.y);

    // Move all the tile tracking structs the same amount as the rest of the board.
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

    size_t dest_begin = dest.flat_index();

    /*
     *
     *     0 0 0 0 0 0 0
     *     1 1 1 1 0 0 0
     *     0 0 0 1 0 0 0
     *     0 0 0 0 0 0 0
     *     0 0 0 0 0 0 0
     *     0 0 0 0 0 0 0
     *
     *     0 0 0 0 0 0 0
     *     0 0 0 0 0 0 0
     *     0 1 1 1 1 0 0
     *     0 0 0 0 1 0 0
     *     0 0 0 0 0 0 0
     *     0 0 0 0 0 0 0
     *
     *
     *
     *
     */

    void (*tiles_dest_vptr) = &tiles;
    char *tiles_dest = static_cast<char *>(tiles_dest_vptr);

    std::memmove(
            tiles_dest + dest_begin,
            tiles_dest + src_begin,
            size * sizeof(uint8_t)
    );
    std::memset(tiles_dest, 0, dest_begin * sizeof(uint8_t));

    std::memset(tiles_dest + dest_begin + size + 1, 0, sizeof(tiles) - (dest_begin + size - 1));

//// Copy data into temp array
//    char t[BOARD_SIZE * BOARD_SIZE] = {0};
//    char *temp = reinterpret_cast<char*>(&t);
//
//    memcpy(temp + dest.flat_index() * sizeof(char),
//           ((char *) &tiles) + src_begin * sizeof(char),
//           (size) * sizeof(char)
//    );
//
//    memset(&tiles, 0, BOARD_SIZE * BOARD_SIZE * sizeof(char));
//    // Copy data back into main array after clearing data.
//    memcpy(&tiles, temp, BOARD_SIZE * BOARD_SIZE * sizeof(char));


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


Board::tile_stack *Board::get_from_stack(const Position &position, bool pop) {
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


bool Board::can_move(Position &position) {
    Board::tile_stack *ts = get_from_stack(position, false);
    // If removing this tile does not change the tree, it is a valid move.
    if (ts != nullptr) return true;


    unsigned char *tile = &tiles[position.y][position.x];
    int n_hive_tiles = sum_hive_tiles() - 1;

    // Store the tile type for later use
    unsigned char tile_type = *tile;
    *tile = EMPTY;

    // Check if the points around this point consist of a single spanning tree.
    bool valid = true;
    auto points = position.get_points_around();
    for (Position &point : points) {
        if (tiles[point.y][point.x] == EMPTY) continue;

        // Count how many tiles are connected.
        int connect_count = connected_components(point);
        if (connect_count != n_hive_tiles) {
            valid = false;
        }
        break;
    }

    // Restore original tile value.
    *tile = tile_type;
    return valid;
}


void Board::update_free_tiles() {
    int n_updated = 0;
    int to_update = sum_hive_tiles();
    Position pos;
    for (int x = min.x; x < max.x + 1; x++) {
        if (n_updated == to_update) break;

        for (int y = min.y; y < max.y + 1; y++) {
            if (n_updated == to_update) break;

            // Dont check empty tiles, or tiles which could already move before this.
            if (tiles[y][x] == EMPTY) continue;

            // Allow everything at first, then let articulation fix this.
            n_updated++;

            pos = Position(x, y);
            free[y][x] = can_move(pos);
        }
    }

//    print_board(board);
//    articulation(board, first_tile);
    // TODO: After articulation, set all stacked beetles to 'free = true', because they are stacked.
}

void Board::update_can_move(Position &position, Position &previous_position) {
    // Dont do this checking twice.
    if (has_updated) return;
    has_updated = true;

    // For articulation it might be beneficial to always full update.
    update_free_tiles();
//    board->free[position] = true;
//
//    int n_neighbours = 0;
//    if (previous_position != -1) {
//        // If the previous tile is not empty, this tile is not necessarily free.
//        if (board->tiles[previous_position] == EMPTY)
//            board->free[previous_position] = false;
//
//        // Check around previous position
//        int *points = get_points_around(previous_position / BOARD_SIZE, previous_position % BOARD_SIZE);
//        for (int i = 0; i < 6; i++) {
//            int px = points[i] % BOARD_SIZE;
//            int py = points[i] / BOARD_SIZE;
//            if (board->tiles[py * BOARD_SIZE + px] == EMPTY) continue;
//            if (n_neighbours == 1) {
//                update_free_tiles(board);
//                return;
//            }
//
//            n_neighbours++;
//            // For all neighbours, update whether or not they can move.
//            board->free[py * BOARD_SIZE + px] = can_move(board, px, py);
//        }
//    }
//    int *points = get_points_around(position / BOARD_SIZE, position % BOARD_SIZE);
//    n_neighbours = 0;
//    for (int i = 0; i < 6; i++) {
//        int px = points[i] % BOARD_SIZE;
//        int py = points[i] / BOARD_SIZE;
//        if (board->tiles[py * BOARD_SIZE + px] == EMPTY) continue;
//
//        if (n_neighbours == 1) {
//            update_free_tiles(board);
//            return;
//        }
//        n_neighbours++;
//        // For all neighbours, update whether or not they can move.
//        board->free[py * BOARD_SIZE + px] = can_move(board, px, py);
//    }
}


int Board::connected_components(Position &original_position) {
    bool visited[BOARD_SIZE][BOARD_SIZE] = {false};

//    return find_articulation(board, index, -1, is_connected);

    int n_connected = 1;
    std::list<Position> frontier;

    frontier.push_back(original_position);
    visited[original_position.y][original_position.x] = true;
    Position position;
    while (!frontier.empty()) {
        position = frontier.back();
        frontier.pop_back();

        auto points = position.get_points_around();
        for (Position &point : points) {
            // Skip this tile if theres nothing on it
            if (tiles[point.y][point.x] == EMPTY
                || visited[point.y][point.x])
                continue;

            // If it was not yet connected, add a connected tile.
            visited[point.y][point.x] = true;


            frontier.push_back(point);
            n_connected += 1;
        }
    }

    return n_connected;
}

