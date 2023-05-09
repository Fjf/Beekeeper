
#include <string>
#include <algorithm>
#include <iostream>
#include "tree.h"
#include "tt.h"
#include "utils.h"



/*
 * Generates the placing moves, only allowed to place next to allied tiles.
 */
template <typename T>
void BaseNode<T>::generate_placing_moves(uint8_t type) {

    uint8_t color = type & COLOR_MASK;

    const Position invalid_position = Position(-1, -1);

    if (board.turn == 0) {
        Position initial_position = Position((BOARD_SIZE / 2), BOARD_SIZE / 2);
        add_child<true>(initial_position, type, invalid_position);
        return;
    }
    if (board.turn == 1) {
        Position initial_position = Position((BOARD_SIZE / 2) + 1, (BOARD_SIZE / 2));
        add_child<true>(initial_position, type, invalid_position);
        return;
    }


    uint8_t n_encountered = 0;
    uint8_t to_encounter = board.sum_hive_tiles();

    bool is_added[BOARD_SIZE][BOARD_SIZE] = {false};

    for (int8_t y = board.min.y; y < board.max.y + 1; y++) {
        if (n_encountered == to_encounter) break;
        for (int8_t x = board.min.x; x < board.max.x + 1; x++) {
            if (n_encountered == to_encounter) break;

            // Skip empty tiles
            if (board.tiles[y][x] == EMPTY) continue;

            n_encountered++;
            // Get points around this point
            auto points = Position::get_points_around(x, y);
            for (const Position &point: points) {
                // Check if its empty
                if (board.tiles[point.y][point.x] != EMPTY or is_added[point.y][point.x]) continue;

                is_added[point.y][point.x] = true;

                // Check for all neighbours of this point if its the same colour as the colour of the
                //  passed tile.
                bool invalid = false;
                auto neighbor_points = point.get_points_around();
#pragma unroll
                for (const Position &np_index : neighbor_points) {
                    // Check if every tile around it has the same colour as the passed tile colour.
                    if (board.tiles[np_index.y][np_index.x] != EMPTY
                        and (board.tiles[np_index.y][np_index.x] & COLOR_MASK) != color) {
                        invalid = true;
                        break;
                    }
                }

                // If any neighbour of this point is another colour, check another point.
                if (invalid) continue;

                add_child<true>(point, type, invalid_position);
            }
        }
    }
}


//bool visited[N_TILES * 2];
//int tin[N_TILES * 2], low[N_TILES * 2];
//int timer;
//void find_articulation(struct board *board, int idx, int parent) {
//    int v = to_tile_index(board->tiles[idx].type);
//    visited[v] = true;
//    tin[v] = low[v] = timer++;
//
//    int children = 0;
//
//    int *points = get_points_around(idx / BOARD_SIZE, idx % BOARD_SIZE);
//    for (int i = 0; i < 6; i++) {
//        int to_idx = points[i];
//        // Skip empty tiles
//        if (board->tiles[to_idx].type == EMPTY) continue;
//
//        int to = to_tile_index(board->tiles[to_idx].type);
//        // Dont go back to parent
//        if (to_idx == parent) continue;
//
//        if (visited[to]) {
//            low[v] = MIN(low[v], tin[to]);
//        } else {
//            find_articulation(board, to_idx, idx);
//            low[v] = MIN(low[v], low[to]);
//            if (low[to] >= tin[v] && parent != -1) {
//                // This is a node which cannot be removed.
//                board->tiles[idx].free = false;
//            }
//            children++;
//        }
//    }
//    if (parent == -1 && children > 1) {
//        board->tiles[idx].free = false;
//    }
//}
//
//void articulation(struct board* board, int index) {
//    timer = 0;
//    memset(&visited, false, sizeof(visited));
//    memset(&tin, -1, sizeof(tin));
//    memset(&low, -1, sizeof(low));
//
//    find_articulation(board, index, -1);
//}

template <typename T>
void BaseNode<T>::generate_directional_grasshopper_moves(Position &orig_pos, int x_incr, int y_incr) {
    int tile_type = board.tiles[orig_pos.y][orig_pos.x];

    Position position = Position(orig_pos.x + x_incr, orig_pos.y + y_incr);

    // It needs to jump at least 1 tile.
    if (board.tiles[position.y][position.x] != EMPTY) {
        while (true) {
            position.x += x_incr;
            position.y += y_incr;
            if (board.tiles[position.y][position.x] == EMPTY) {
                add_child<false>(position, tile_type, orig_pos);
                break;
            }
        }
    }

}

template <typename T>
void BaseNode<T>::generate_grasshopper_moves(Position &orig_pos) {
    // Grasshopper can jump in the 6 directions over all connected sets of tiles
    generate_directional_grasshopper_moves(orig_pos, 0, -1);
    generate_directional_grasshopper_moves(orig_pos, -1, -1);
    generate_directional_grasshopper_moves(orig_pos, 1, 0);
    generate_directional_grasshopper_moves(orig_pos, -1, 0);
    generate_directional_grasshopper_moves(orig_pos, 0, 1);
    generate_directional_grasshopper_moves(orig_pos, 1, 1);
}



template <typename T>
void BaseNode<T>::generate_ant_moves(Position &orig) {
    // Store tile for temporary removal
    int tile_type = board[orig];
    board[orig] = EMPTY;

    bool ant_visited[BOARD_SIZE][BOARD_SIZE] = {false};
    std::list<Position> ant_history;
    std::list<Position> frontier;

    ant_visited[orig.y][orig.x] = true;
    frontier.push_back(orig);
    Position point;
    while (!frontier.empty()) {
        point = frontier.back();
        frontier.pop_back();

        auto points = point.get_points_around();
        for (Position &new_point : points) {
            // Ants cannot stack.
            if (board[new_point] != EMPTY) continue;

            // Skip this tile if it has no neighbours
            // Connected hive requirement.
            if (!has_neighbour(board, new_point)) continue;

            // If this tile cannot fit through the gap, skip the tile.
            if (!tile_fits(board, point, new_point)) continue;

            // Skip tile if we already added this point.
            if (ant_visited[new_point.y][new_point.x]) continue;

            ant_visited[new_point.y][new_point.x] = true;

            ant_history.push_back(new_point);
            frontier.push_back(new_point);
        }
    }
    board[orig] = tile_type;

    // Generate moves based on these valid ant moves.
    for (Position &p : ant_history) {
        add_child<false>(p, tile_type, orig);
    }
}

template <typename T>
void BaseNode<T>::generate_queen_moves(Position &orig) {
    /*
     * Generates the moves for the queen
     */
    // Store tile for temporary removal
    int tile_type = board[orig];

    // If this tile is on top of something, get that tile.
    // Only do this because the beetle calls this function, so it saves programming effort
    Board::tile_stack *temp = board.get_from_stack(orig, false);
    if (temp == nullptr) {
        board[orig] = EMPTY;
    } else {
        board[orig] = temp->type;
    }

    auto points = orig.get_points_around();
    for (Position &point : points) {
        // Get all tiles which are connected to the queen
        if (board[point] == EMPTY) continue;


        if (!tile_fits(board, orig, point)) continue;

        // For all neighbors, if the neighbors neighbors == my neighbors -> valid move.
        auto neighbor_points = point.get_points_around();
        for (Position &neighbor_point : neighbor_points) {
            if (board[neighbor_point] != EMPTY) continue;

            for (Position &double_neighbour : points) {
                // Skip because neighbours do not match.
                if (neighbor_point != double_neighbour) continue;

                // Skip this tile if the move from the original orig to this new orig does not fit.
                if (!tile_fits(board, orig, double_neighbour)) continue;

                // This tile is connected.
                add_child<false>(neighbor_point, tile_type, orig);
            }
        }
    }

    board[orig] = tile_type;
}
template <typename T>
void BaseNode<T>::generate_beetle_moves(Position &point) {
    /*
     * Generate the moves for the beetle
     * The beetle has all valid moves for the queen, and it can move on top of the hive.
     */
    auto points = point.get_points_around();

    // Store tile for temporary removal
    int tile_type = board[point];

    // If this tile is on top of something, get that tile.
    Board::tile_stack *temp = board.get_from_stack(point, false);
    if (temp == nullptr) {
        // If you are not on top of something, you can move like a queen, or on top of something.
        generate_queen_moves(point);

        board[point] = EMPTY;

        for (Position &new_point : points) {
            // Get all tiles which are connected to the beetle
            if (board[new_point] == EMPTY) continue;

            add_child<false>(new_point, tile_type, point);
        }
    } else {
        // Beetle on top of something has no restrictions on movement off of the tile.
        board[point] = temp->type;
        for (Position &new_point : points) {
            add_child<false>(new_point, tile_type, point);
        }
    }

    board[point] = tile_type;
}

template <typename T>
void BaseNode<T>::generate_spider_moves(Position &orig) {
    // Store tile for temporary removal
    int tile_type = board[orig];
    board[orig] = EMPTY;

    bool spider_visited[BOARD_SIZE][BOARD_SIZE] = {false};

    // Frontier to track multiple points.
    std::list<Position> frontier;
    std::list<Position> next_frontier;

    frontier.push_back(orig);

    const size_t SPIDER_WALKING_DISTANCE = 3;
    for (int iteration = 0; iteration < SPIDER_WALKING_DISTANCE; iteration++) {
        while (!frontier.empty()) {
            // Get a point on the frontier
            Position frontier_point = frontier.back();
            frontier.pop_back();
            auto points = frontier_point.get_points_around();

            // Iterate all points surrounding this frontier.
            // Generate a list containing all valid moves from the frontier onward.
            for (Position &point : points) {
                if (board[point] == EMPTY) continue;
                if (!tile_fits(board, frontier_point, point)) continue;

                // For all neighbors, if the neighbors neighbors == my neighbors -> valid move.
                auto neighbor_points = point.get_points_around();
                for (Position &neighbor_point : neighbor_points) {
                    if (board[neighbor_point] != EMPTY) continue;
                    for (Position &double_neighbor : points) {
                        if (neighbor_point != double_neighbor) continue;

                        if (!tile_fits(board, frontier_point, double_neighbor)) continue;

                        // Skip tile if we already added this point.
                        if (spider_visited[neighbor_point.y][neighbor_point.x]) continue;
                        spider_visited[neighbor_point.y][neighbor_point.x] = true;

                        next_frontier.push_back(neighbor_point);
                    }
                }
            }
        }
        // Swap two frontiers.
        frontier.swap(next_frontier);
        next_frontier.clear();
    }
    board[orig] = tile_type;

    for (Position &point : frontier) {
        add_child<false>(point, tile_type, orig);
    }
}

template <typename T>
void BaseNode<T>::generate_free_moves(int player_bit) {

    // We do a full update as soon as we want to move.
    board.update_can_move(move.location, move.previous_location);

#ifdef CENTERED
    int ly = board->min_y, hy = board->max_y + 1;
    int lx = board->min_x, hx = board->max_x + 1;
#else
    int8_t ly = 0, hy = BOARD_SIZE;
    int8_t lx = 0, hx = BOARD_SIZE;
#endif

    Position position;
    for (int8_t y = ly; y < hy; y++) {
        for (int8_t x = lx; x < hx; x++) {
            position.x = x;
            position.y = y;

            unsigned char tile = board[position];
            if (tile == EMPTY) continue;
            // Only move your own tiles
            if ((tile & COLOR_MASK) != player_bit) continue;

            if (!board.free[position.y][position.x]) continue;

            // If this tile can be removed without breaking the hive, add it to the valid moves list.
            if ((tile & TILE_MASK) == L_GRASSHOPPER) {
                generate_grasshopper_moves( position);
            } else if ((tile & TILE_MASK) == L_BEETLE) {
                generate_beetle_moves( position);
            } else if ((tile & TILE_MASK) == L_ANT) {
                generate_ant_moves( position);
            } else if ((tile & TILE_MASK) == L_QUEEN) {
                generate_queen_moves( position);
            } else if ((tile & TILE_MASK) == L_SPIDER) {
                generate_spider_moves( position);
            }
        }
    }
}

template <typename T>
void BaseNode<T>::generate_moves() {
    int player_idx = board.turn % 2;
    int player_bit = player_idx << COLOR_SHIFT;

    int player_move = board.turn / 2;

    Board::player_info &player = board.players[player_idx];
    // By player_move 4 for each player, the queen has to be placed.
    if (player_move == 3 && player.queens_left == 1) {
        return generate_placing_moves(L_QUEEN | player_bit);
    }

    if (player.spiders_left > 0) {
        generate_placing_moves(L_SPIDER | player_bit);
    }
    if (player.beetles_left > 0) {
        generate_placing_moves(L_BEETLE | player_bit);
    }
    if (player.grasshoppers_left > 0) {
        generate_placing_moves( L_GRASSHOPPER | player_bit);
    }
    if (player.ants_left > 0) {
        generate_placing_moves(L_ANT | player_bit);
    }

    // Queens cannot be placed in the first player_move (tournament rules)
    if (player.queens_left > 0 && player_move > 0)
        generate_placing_moves(L_QUEEN | player_bit);

    // Tiles can only be moved if their queen is on the board.
    if (player.queens_left == 0)
        generate_free_moves(player_bit);
}


template <typename T>
std::string BaseNode<T>::get_move() const {
    std::string response = move.to_string();
    response += ' ';

    if (move.direction == 5) {
        response += '\\';
    } else if (move.direction == 3) {
        response += '-';
    } else if (move.direction == 1) {
        response += '/';
    }
    move.to_string();

    if (move.direction == 4) {
        response += '/';
    } else if (move.direction == 2) {
        response += '-';
    } else if (move.direction == 0) {
        response += '\\';
    }

    response += '\n';
    response += '\0';

    return response;
}

template <typename T>
void BaseNode<T>::print() {
    std::cout << "Move: " << move.to_string() << std::endl;
    std::cout << board.to_string() << std::endl;
}

template <typename T>
int BaseNode<T>::generate_children() {
    if (board.turn >= MAX_TURNS - 1) {
        return ERR_NOMOVES;
    }

    // Only generate more nodes if you have no nodes yet
    if (children.empty()) {
        generate_moves();

        if (children.empty()) {
            // Generate dummy move (pass)
            Position dummy_pos = Position(-1, -1);
            add_child<false>(dummy_pos, 0, dummy_pos);
        }
    }
    return children.empty();
}


/*
 * Adds all available moves to a node as children.
 */
template <typename T>
template<bool placed>
void BaseNode<T>::add_child(const Position &location, int type, const Position &previous_location) {
    if (board.turn == MAX_TURNS - 1) {
        return;
    }

    // Create new board
    T child = copy();
    Board& board = child.board;

//    printf("Adding child with type %d on turn %d\n", type, board.turn);
//    std::cout << "Position = " << previous_location << std::endl;

    child.parent = this;

    if (location.x == -1) {
        // No valid moves are available
        board.turn++;

        child.move.location = Position(0, 0);
        child.move.previous_location = Position(0, 0);
        child.move.direction = 7;
        child.move.next_to = 0;
        child.move.tile = 0;
        children.push_back(child);
        return;
    }

    // Previous location being a negative value means this is a newly placed tile.
    // Track how many tiles are on the board.
    if (placed) {
        int masked_type = type & TILE_MASK;
        Board::player_info &player = board.players[board.turn % 2];
        // Reduce the amount of available tiles per player
        // Only do this if the tile was not moved but instead newly placed
        if (masked_type == L_QUEEN) {
            type |= (N_QUEENS - player.queens_left + 1) << NUMBER_SHIFT;
            player.queens_left--;
        } else if (masked_type == L_BEETLE) {
            type |= (N_BEETLES - player.beetles_left + 1) << NUMBER_SHIFT;
            player.beetles_left--;
        } else if (masked_type == L_GRASSHOPPER) {
            type |= (N_GRASSHOPPERS - player.grasshoppers_left + 1) << NUMBER_SHIFT;
            player.grasshoppers_left--;
        } else if (masked_type == L_ANT) {
            type |= (N_ANTS - player.ants_left + 1) << NUMBER_SHIFT;
            player.ants_left--;
        } else if (masked_type == L_SPIDER) {
            type |= (N_SPIDERS - player.spiders_left + 1) << NUMBER_SHIFT;
            player.spiders_left--;
        }
    } else {
        Board::tile_stack *ts = board.get_from_stack(previous_location, true);
        board.tiles[previous_location.y][previous_location.x] = (ts == nullptr ? EMPTY : ts->type);
    }

    // If this move is on top of an existing tile, store this tile in the stack
    if (board.tiles[location.y][location.x] != EMPTY) {
        for (auto &tile_stack : board.stack) {
            if (tile_stack.position.x == -1) {
                // Get highest tile from tile_stack
                Board::tile_stack *ts = board.get_from_stack(location, false);

                // Store in tile_stack
                tile_stack.position = location;
                tile_stack.type = board.tiles[location.y][location.x];
                tile_stack.z = (ts == nullptr ? 0 : ts->z + 1);
                break;
            }
        }

        // This is to track all the stacked tiles in a simple list to help cloning.
        board.n_stacked++;
    }

    // Do this for easier board-finished state checking (dont have to take into account beetles).
    if ((type & (TILE_MASK | COLOR_MASK)) == L_QUEEN) {
        board.light_queen = location;
    } else if ((type & (TILE_MASK | COLOR_MASK)) == D_QUEEN) {
        board.dark_queen = location;
    }

    // Update the zobrist hash for this child
    zobrist_hash(board, location, previous_location, type);

    // Store hash history
//    board.hash_history.push_back(board.zobrist_hash);
    board.tiles[location.y][location.x] = type;
    board.turn++;

    board.has_updated = false;

    // If the old x or y position was the minimum value, recompute the min/max value.
    if (previous_location.x == board.min.x || previous_location.y == board.min.y) {
        board.get_min_x_y();
    }
    if (previous_location.x == board.max.x || previous_location.y == board.max.y) {
        board.get_max_x_y();
    }

    board.min.x = std::min(board.min.x, location.x);
    board.max.x = std::max(board.max.x, location.x);

    board.min.y = std::min(board.min.y, location.y);
    board.max.y = std::max(board.max.y, location.y);

    // If the min or max is at the end, translate the board to the center.
    if ((board.min.x <= 3) || (board.min.y <= 3) || board.max.x > BOARD_SIZE - 4 || board.max.y > BOARD_SIZE - 4) {
        // After this move, ensure this board is centered.
        board.center();
    }

    child.move.previous_location = previous_location;
    child.move.location = location;

    // Initialize values to 0.
    child.move.direction = 0;
    child.move.next_to = 0;

    // Add move notation for clarity
    if (board.tiles[location.y][location.x] != EMPTY) {
        child.move.direction = 7;
        child.move.next_to = board.tiles[location.y][location.x];
    } else {
        auto points = location.get_points_around();
        size_t p = 0;
        for (Position &point : points) {
            if (child.board.tiles[point.y][point.x] != EMPTY) {
                child.move.direction = p;
                child.move.next_to = child.board.tiles[point.y][point.x];
                break;
            }
            p++;
        }
    }
    child.move.tile = type;
    children.push_back(std::move(child));
}

