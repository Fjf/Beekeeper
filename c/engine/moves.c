#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "moves.h"
#include "tt.h"


struct node *default_add_child(struct node *node, struct board *board) {
    struct node *child = default_init();

    child->board = board;

    node_add_child(node, child);
    return child;
}

struct node* default_init() {
    struct node *root = malloc(sizeof(struct node));
    node_init(root, NULL);
    return root;
}

struct node *(*dedicated_add_child)(struct node *node, struct board *board) = default_add_child;
struct node *(*dedicated_init)() = default_init;

int to_tile_index(unsigned char tile) {
    int color = (tile & COLOR_MASK) >> COLOR_SHIFT;
    int sum = color * N_TILES;
    int type = tile & TILE_MASK;
    int n    = ((tile & NUMBER_MASK) >> NUMBER_SHIFT) - 1;

    if (type == L_ANT) return sum + n;
    sum += N_ANTS;
    if (type == L_GRASSHOPPER) return sum + n;
    sum += N_GRASSHOPPERS;
    if (type == L_BEETLE) return sum + n;
    sum += N_BEETLES;
    if (type == L_SPIDER) return sum + n;
    sum += N_SPIDERS;
    // It must be a queen.
    return sum + n;
}

int sum_hive_tiles(struct board *board) {
    return N_TILES * 2 - (
            board->players[0].queens_left +
            board->players[0].grasshoppers_left +
            board->players[0].beetles_left +
            board->players[0].ants_left +
            board->players[0].spiders_left +
            board->players[1].spiders_left +
            board->players[1].ants_left +
            board->players[1].queens_left +
            board->players[1].grasshoppers_left +
            board->players[1].beetles_left) -
           board->n_stacked;
}


struct tile_stack *get_from_stack(struct board *board, int location, bool pop) {
    /*
     * Returns the highest tile in the stack, or NULL if no tile is found.
     * If pop is true, the tile will be removed from the stack.
     * Otherwise, just the tile will be returned.
     */
    struct tile_stack *highest_tile = NULL;
    int highest_idx = -1;
    for (int i = 0; i < TILE_STACK_SIZE; i++) {
        if (board->stack[i].location == location
            && highest_idx < board->stack[i].z) {
            highest_idx = board->stack[i].z;
            highest_tile = &board->stack[i];
        }
    }
    if (highest_tile == NULL) {
        return NULL;
    }

    if (pop) {
        // Remove from list
        highest_tile->location = -1;
        board->n_stacked--;
    }
    return highest_tile;
}

void full_update(struct board *board) {
    int n_updated = 0;
    int to_update = sum_hive_tiles(board);

#ifdef CENTERED
    int ly = board->min_y, hy = board->max_y + 1;
    int lx = board->min_x, hx = board->max_x + 1;
#else
    int ly = 0, hy = BOARD_SIZE;
    int lx = 0, hx = BOARD_SIZE;
#endif

//    int first_tile = -1;
    for (int x = lx; x < hx; x++) {
        if (n_updated == to_update) break;
        for (int y = ly; y < hy; y++) {
            if (n_updated == to_update) break;
            // Dont check empty tiles, or tiles which could already move before this.
            if (board->tiles[y * BOARD_SIZE + x].type == EMPTY) continue;

//            first_tile = y * BOARD_SIZE + x;

            // Allow everything at first, then let articulation fix this.
            n_updated++;
            board->tiles[y * BOARD_SIZE + x].free = can_move(board, x, y);
        }
    }

//    print_board(board);
//    articulation(board, first_tile);
    // TODO: After articulation, set all stacked beetles to 'free = true', because they are stacked.
}

void update_can_move(struct board *board, int location, int previous_location) {
    // Dont do this checking twice.
    if (board->has_updated) return;
    board->has_updated = true;

    // For articulation it might be beneficial to always full update.
    return full_update(board);

    board->tiles[location].free = true;

    int n_neighbours = 0;
    if (previous_location != -1) {
        // If the previous tile is not empty, this tile is not necessarily free.
        if (board->tiles[previous_location].type == EMPTY)
            board->tiles[previous_location].free = false;

        // Check around previous location
        int *points = get_points_around(previous_location / BOARD_SIZE, previous_location % BOARD_SIZE);
        for (int i = 0; i < 6; i++) {
            int px = points[i] % BOARD_SIZE;
            int py = points[i] / BOARD_SIZE;
            if (board->tiles[py * BOARD_SIZE + px].type == EMPTY) continue;
            if (n_neighbours == 1) {
                return full_update(board);
            }

            n_neighbours++;
            // For all neighbours, update whether or not they can move.
            board->tiles[py * BOARD_SIZE + px].free = can_move(board, px, py);
        }
    }
    int *points = get_points_around(location / BOARD_SIZE, location % BOARD_SIZE);
    n_neighbours = 0;
    for (int i = 0; i < 6; i++) {
        int px = points[i] % BOARD_SIZE;
        int py = points[i] / BOARD_SIZE;
        if (board->tiles[py * BOARD_SIZE + px].type == EMPTY) continue;

        if (n_neighbours == 1) {
            return full_update(board);
        }
        n_neighbours++;
        // For all neighbours, update whether or not they can move.
        board->tiles[py * BOARD_SIZE + px].free = can_move(board, px, py);
    }
}

/*
 * Adds all available moves to a node as children.
 */
void add_child(struct node *node, int location, int type, int previous_location) {
    struct tile_stack *ts;

    if (node->board->turn == MAX_TURNS - 1) {
        return;
    }

    // Create new board
    struct board *board = malloc(sizeof(struct board));
    if (board == NULL) {
        fprintf(stderr, "No memory left to allocate a board\n");
        exit(1);
    }
    memcpy(board, node->board, sizeof(struct board));
    board->move_location_tracker = 0;

    // Parent will track how many children it has this way.
    node->board->move_location_tracker++;
    if (location == -1) {
        // No valid moves are available
        board->turn++;

        struct node *child = dedicated_add_child(node, board);

        child->move.direction = 7;
        child->move.next_to = 0;
        child->move.tile = 0;
        return;
    }

    // Track how many tiles are on the board.
    if (previous_location == -1) {
        int masked_type = type & TILE_MASK;

        // Reduce the amount of available tiles per player
        // Only do this if the tile was not moved but instead newly placed
        if (masked_type == L_QUEEN) {
            type |= (N_QUEENS - board->players[board->turn % 2].queens_left + 1) << NUMBER_SHIFT;
            board->players[board->turn % 2].queens_left--;
        } else if (masked_type == L_BEETLE) {
            type |= (N_BEETLES - board->players[board->turn % 2].beetles_left + 1) << NUMBER_SHIFT;
            board->players[board->turn % 2].beetles_left--;
        } else if (masked_type == L_GRASSHOPPER) {
            type |= (N_GRASSHOPPERS - board->players[board->turn % 2].grasshoppers_left + 1) << NUMBER_SHIFT;
            board->players[board->turn % 2].grasshoppers_left--;
        } else if (masked_type == L_ANT) {
            type |= (N_ANTS - board->players[board->turn % 2].ants_left + 1) << NUMBER_SHIFT;
            board->players[board->turn % 2].ants_left--;
        } else if (masked_type == L_SPIDER) {
            type |= (N_SPIDERS - board->players[board->turn % 2].spiders_left + 1) << NUMBER_SHIFT;
            board->players[board->turn % 2].spiders_left--;
        }
    } else {
        ts = get_from_stack(board, previous_location, true);
        board->tiles[previous_location].type = (ts == NULL ? EMPTY : ts->type);
    }

    // If this move is on top of an existing tile, store this tile in the stack
    if (board->tiles[location].type != EMPTY) {
        for (int i = 0; i < TILE_STACK_SIZE; i++) {
            if (board->stack[i].location == -1) {
                // Get highest tile from stack
                ts = get_from_stack(board, location, false);

                // Store in stack
                board->stack[i].location = location;
                board->stack[i].type = board->tiles[location].type;
                board->stack[i].z = (ts == NULL ? 0 : ts->z + 1);
                break;
            }
        }

        // This is to track all the stacked tiles in a simple list to help cloning.
        board->n_stacked++;
    }

    // Do this for easier board-finished state checking (dont have to take into account beetles).
    if ((type & (TILE_MASK | COLOR_MASK)) == L_QUEEN) {
        board->light_queen_position = location;
    } else if ((type & (TILE_MASK | COLOR_MASK)) == D_QUEEN) {
        board->dark_queen_position = location;
    }

    // Update the zobrist hash for this child
    zobrist_hash(board, location, previous_location, type);

    // Store hash history
    board->hash_history[board->turn] = board->zobrist_hash;
    board->tiles[location].type = type;
    board->turn++;

    board->has_updated = false;
//    update_can_move(board, location, previous_location);

    // Set min and max tile positions to speedup translation.
    int x = location % BOARD_SIZE;
    int y = location / BOARD_SIZE;

#ifdef CENTERED
    int old_x = previous_location % BOARD_SIZE;
    int old_y = previous_location / BOARD_SIZE;

    // If the old x or y position was the minimum value, recompute the min/max value.
    if (old_x == board->min_x || old_y == board->min_y) {
        get_min_x_y(board, &board->min_x, &board->min_y);
    } if (old_x == board->max_x || old_y == board->max_y) {
        get_max_x_y(board, &board->max_x, &board->max_y);
    }

    board->min_x = MIN(board->min_x, x);
    board->max_x = MAX(board->max_x, x);

    board->min_y = MIN(board->min_y, y);
    board->max_y = MAX(board->max_y, y);

    // If the min or max is at the end, translate the board to the center.
    if ((board->min_x <= 3) || (board->min_y <= 3) || board->max_x > BOARD_SIZE - 4 || board->max_y > BOARD_SIZE - 4) {
        // After this move, ensure this board is centered.
        translate_board(board);
    }
#else
    translate_board_22(board);
#endif

    struct node *child = dedicated_add_child(node, board);
    child->move.previous_location = previous_location;
    child->move.location = location;

    // Add move notation for clarity
    if (node->board->tiles[location].type != EMPTY) {
        child->move.direction = 7;
        child->move.next_to = node->board->tiles[location].type;
    } else {
        int *points = get_points_around(y, x);
        for (unsigned char p = 0; p < 6; p++) {
            int point = points[p];
            if (child->board->tiles[point].type != EMPTY) {
                child->move.direction = p;
                child->move.next_to = child->board->tiles[point].type;
                break;
            }
        }
    }
    child->move.tile = (unsigned char) type;
}

int points_around[BOARD_SIZE * BOARD_SIZE][6];

void initialize_points_around() {
    for (int y = 0; y < BOARD_SIZE; y++) {
        for (int x = 0; x < BOARD_SIZE; x++) {
            int *points = points_around[y * BOARD_SIZE + x];
            points[0] = (y - 1) * BOARD_SIZE + (x - 1);
            points[1] = (y - 1) * BOARD_SIZE + (x + 0);
            points[2] = (y + 0) * BOARD_SIZE + (x - 1);
            points[3] = (y + 0) * BOARD_SIZE + (x + 1);
            points[4] = (y + 1) * BOARD_SIZE + (x + 0);
            points[5] = (y + 1) * BOARD_SIZE + (x + 1);
        }
    }
}


/*
 * Returns an array containing array indices around a given x,y coordinate.
 */
int *get_points_around(int y, int x) {
    return points_around[y * BOARD_SIZE + x];
}

/*
 * Generates the placing moves, only allowed to place next to allied tiles.
 */
void generate_placing_moves(struct node *node, int type) {
    /*
     * Returns:
     *   - 0: No errors occurred.
     *   - 1: Out of memory error.
     */

    int color = type & COLOR_MASK;

    struct board *board = node->board;

#ifdef CENTERED
    int initial_position = (BOARD_SIZE / 2) * BOARD_SIZE + BOARD_SIZE / 2;
#else
    int initial_position = (2) * BOARD_SIZE + 2;
#endif

    if (board->turn == 0) {
        return add_child(node, initial_position, type, -1);
    }
    if (board->turn == 1) {
        return add_child(node, initial_position + 1, type, -1);
    }

    int n_encountered = 0;
    int to_encounter = sum_hive_tiles(node->board);

#ifdef CENTERED
    int ly = board->min_y, hy = board->max_y + 1;
    int lx = board->min_x, hx = board->max_x + 1;
#else
    int ly = 0, hy = BOARD_SIZE;
    int lx = 0, hx = BOARD_SIZE;
#endif

    bool is_added[BOARD_SIZE * BOARD_SIZE] = {0};

    for (int y = ly; y < hy; y++) {
        if (n_encountered == to_encounter) break;
        for (int x = lx; x < hx; x++) {
            if (n_encountered == to_encounter) break;

            // Skip empty tiles
            if (board->tiles[y * BOARD_SIZE + x].type == EMPTY) continue;

            n_encountered++;
            // Get points around this point
            int *points = get_points_around(y, x);
            for (int p = 0; p < 6; p++) {
                int point = points[p];
                // Check if its empty
                if (board->tiles[point].type != EMPTY) continue;

                // Check if we added this location earlier, if so, remove this (reduce amount of duplicate nodes)
                if (is_added[point]) continue;
                is_added[point] = true;

                // Check for all neighbours of this point if its the same colour as the colour of the
                //  passed tile.
                int invalid = 0;
                int yy = point / BOARD_SIZE;
                int xx = point % BOARD_SIZE;
                int *neighbor_points = get_points_around(yy, xx);
                for (int np = 0; np < 6; np++) {
                    int np_index = neighbor_points[np];
                    if (np_index < 0 || np_index >= BOARD_SIZE * BOARD_SIZE) continue;

                    if (board->tiles[np_index].type == EMPTY) continue;

                    // Check if every tile around it has the same colour as the passed tile colour.
                    if ((board->tiles[np_index].type & COLOR_MASK) != color) {
                        invalid = 1;
                        break;
                    }
                }

                // If any neighbour of this point is another colour, check another point.
                if (invalid) continue;

                add_child(node, point, type, -1);
            }
        }
    }
}

int add_if_unique(int *array, int n, int value) {
    for (int i = 0; i < n; i++) {
        if (array[i] == value) return 0;
    }
    array[n] = value;
    return 1;
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


int connected_components(struct board *board, int index) {
    bool visited[BOARD_SIZE * BOARD_SIZE] = {0};

//    return find_articulation(board, index, -1, is_connected);

    int n_connected = 1;
    int frontier[N_TILES * 2];
    int frontier_p = 0; // Frontier pointer.

    frontier[frontier_p++] = index;
    visited[index] = true;

    while (frontier_p != 0) {
        int i = frontier[--frontier_p];
        int y = i / BOARD_SIZE;
        int x = i % BOARD_SIZE;

        int *points = get_points_around(y, x);
        for (int n = 0; n < 6; n++) {
            // Skip this tile if theres nothing on it
            if (board->tiles[points[n]].type == EMPTY
                || visited[points[n]])
                continue;

            // If it was not yet connected, add a connected tile.
            visited[points[n]] = true;

            frontier[frontier_p++] = points[n];
            n_connected += 1;
        }
    }

    return n_connected;
}


void generate_directional_grasshopper_moves(struct node *node, int orig_y, int orig_x, int x_incr, int y_incr) {
    struct board *board = node->board;
    int orig_pos = orig_y * BOARD_SIZE + orig_x;
    int tile_type = board->tiles[orig_pos].type;

    int x = orig_x + x_incr;
    int y = orig_y + y_incr;
    // It needs to jump at least 1 tile.
    if (board->tiles[y * BOARD_SIZE + x].type != EMPTY) {
        while (1) {
            x += x_incr;
            y += y_incr;
            if (board->tiles[y * BOARD_SIZE + x].type == EMPTY) {
                add_child(node, y * BOARD_SIZE + x, tile_type, orig_pos);
                break;
            }
        }
    }

}

void generate_grasshopper_moves(struct node *node, int orig_y, int orig_x) {
    // Grasshopper can jump in the 6 directions over all connected sets of tiles
    generate_directional_grasshopper_moves(node, orig_y, orig_x, 0, -1);
    generate_directional_grasshopper_moves(node, orig_y, orig_x, -1, -1);
    generate_directional_grasshopper_moves(node, orig_y, orig_x, 1, 0);
    generate_directional_grasshopper_moves(node, orig_y, orig_x, -1, 0);
    generate_directional_grasshopper_moves(node, orig_y, orig_x, 0, 1);
    generate_directional_grasshopper_moves(node, orig_y, orig_x, 1, 1);
}

bool has_neighbour(struct board *board, int location) {
    int x = location % BOARD_SIZE;
    int y = location / BOARD_SIZE;
    int *points = get_points_around(y, x);
    for (int i = 0; i < 6; i++) {
        if (board->tiles[points[i]].type != EMPTY) {
            return true;
        }
    }
    return false;
}

bool tile_fits(struct board *board, int x, int y, int new_x, int new_y) {
    bool tl = board->tiles[(y - 1) * BOARD_SIZE + (x - 1)].type == EMPTY;
    bool tr = board->tiles[(y - 1) * BOARD_SIZE + (x + 0)].type == EMPTY;
    bool l = board->tiles[(y + 0) * BOARD_SIZE + (x - 1)].type == EMPTY;
    bool r = board->tiles[(y + 0) * BOARD_SIZE + (x + 1)].type == EMPTY;
    bool bl = board->tiles[(y + 1) * BOARD_SIZE + (x + 0)].type == EMPTY;
    bool br = board->tiles[(y + 1) * BOARD_SIZE + (x + 1)].type == EMPTY;

    if (x - new_x == -1 && y - new_y == 0) {
        // Going to the right
        return tr || br;
    } else if (x - new_x == 1 && y - new_y == 0) {
        // Going to the left
        return tl || bl;
    } else if (x - new_x == 1 && y - new_y == 1) {
        // Going to the top left
        return l || tr;
    } else if (x - new_x == 0 && y - new_y == 1) {
        // Going to the top right
        return r || tl;
    } else if (x - new_x == 0 && y - new_y == -1) {
        // Going to the bottom left
        return l || br;
    } else if (x - new_x == -1 && y - new_y == -1) {
        // Going to the bottom right
        return r || bl;
    }
    print_board(board);
    fprintf(stderr, "Invalid neighbour found, check the coordinates. (%d,%d)->(%d,%d)\n", x, y, new_x, new_y);
    exit(1);
}

void generate_ant_moves(struct node *node, int orig_y, int orig_x) {
    struct board *board = node->board;

    // Store tile for temporary removal
    int tile_type = board->tiles[orig_y * BOARD_SIZE + orig_x].type;
    board->tiles[orig_y * BOARD_SIZE + orig_x].type = EMPTY;

    int ant_move_buffer[BOARD_SIZE * BOARD_SIZE];// = malloc(BOARD_SIZE * BOARD_SIZE * sizeof(int));
    int n_ant_moves = 0;
    int frontier[N_TILES];
    int frontier_p = 0; // Frontier pointer.
    int index = orig_y * BOARD_SIZE + orig_x;

    ant_move_buffer[n_ant_moves++] = index;
    frontier[frontier_p++] = index;

    while (frontier_p != 0) {
        int i = frontier[--frontier_p];
        int y = i / BOARD_SIZE;
        int x = i % BOARD_SIZE;

        int *points = get_points_around(y, x);
        for (int n = 0; n < 6; n++) {
            int point = points[n];

            // Ants cannot stack.
            if (board->tiles[point].type != EMPTY) continue;

            // Skip this tile if it has no neighbours
            // Connected hive requirement.
            if (!has_neighbour(board, point)) continue;

            int new_x = point % BOARD_SIZE;
            int new_y = point / BOARD_SIZE;
            if (!tile_fits(board, x, y, new_x, new_y)) continue;

            // If a unique tile is added, increment the tracker.
            int added = add_if_unique(ant_move_buffer, n_ant_moves, points[n]);

            if (added) {
                n_ant_moves++;
                frontier[frontier_p++] = points[n];
            }
        }
    }
    board->tiles[orig_y * BOARD_SIZE + orig_x].type = tile_type;

    // Generate moves based on these valid ant moves.
    for (int i = 0; i < n_ant_moves; i++) {
        // Moving to the same position it already was is not valid.
        if (ant_move_buffer[i] == orig_y * BOARD_SIZE + orig_x) continue;
        add_child(node, ant_move_buffer[i], tile_type, orig_y * BOARD_SIZE + orig_x);
    }
}

void generate_queen_moves(struct node *node, int y, int x) {
    /*
     * Generates the moves for the queen
     */
    struct board *board = node->board;

    // Store tile for temporary removal
    int tile_type = board->tiles[y * BOARD_SIZE + x].type;

    // If this tile is on top of something, get that tile.
    // Only do this because the beetle calls this function, so it saves programming effort
    struct tile_stack *temp = get_from_stack(node->board, y * BOARD_SIZE + x, false);
    if (temp == NULL) {
        board->tiles[y * BOARD_SIZE + x].type = EMPTY;
    } else {
        board->tiles[y * BOARD_SIZE + x].type = temp->type;
    }

    int *points = get_points_around(y, x);
    for (int p = 0; p < 6; p++) {
        int point = points[p];

        // Get all tiles which are connected to the queen
        if (board->tiles[point].type == EMPTY) continue;

        int xx = point % BOARD_SIZE;
        int yy = point / BOARD_SIZE;

        if (!tile_fits(board, x, y, xx, yy)) continue;

        // For all neighbors, if the neighbors neighbors == my neighbors -> valid move.
        int *neighbor_points = get_points_around(yy, xx);
        for (int i = 0; i < 6; i++) {
            if (board->tiles[neighbor_points[i]].type != EMPTY) continue;

            for (int j = 0; j < 6; j++) {
                if (neighbor_points[i] == points[j]) {
                    if (!tile_fits(board, x, y, points[j] % BOARD_SIZE, points[j] / BOARD_SIZE)) continue;

                    // This tile is connected.
                    add_child(node, neighbor_points[i], tile_type, y * BOARD_SIZE + x);
                }
            }
        }
    }

    board->tiles[y * BOARD_SIZE + x].type = tile_type;
}

void generate_beetle_moves(struct node *node, int y, int x) {
    /*
     * Generate the moves for the beetle
     * The beetle has all valid moves for the queen, and it can move on top of the hive.
     */
    struct board *board = node->board;
    int *points = get_points_around(y, x);

    // Store tile for temporary removal
    int tile_type = board->tiles[y * BOARD_SIZE + x].type;

    // If this tile is on top of something, get that tile.
    struct tile_stack *temp = get_from_stack(node->board, y * BOARD_SIZE + x, false);
    if (temp == NULL) {
        // If you are not on top of something, you can move like a queen, or on top of something.
        generate_queen_moves(node, y, x);

        board->tiles[y * BOARD_SIZE + x].type = EMPTY;

        for (int p = 0; p < 6; p++) {
            // Get all tiles which are connected to the beetle
            if (board->tiles[points[p]].type == EMPTY) continue;

            add_child(node, points[p], tile_type, y * BOARD_SIZE + x);
        }
    } else {
        // Beetle on top of something has no restrictions on movement off of the tile.
        board->tiles[y * BOARD_SIZE + x].type = temp->type;
        for (int p = 0; p < 6; p++) {
            add_child(node, points[p], tile_type, y * BOARD_SIZE + x);
        }
    }

    board->tiles[y * BOARD_SIZE + x].type = tile_type;
}

void generate_spider_moves(struct node *node, int orig_y, int orig_x) {
    struct board *board = node->board;

    // Store tile for temporary removal
    int tile_type = board->tiles[orig_y * BOARD_SIZE + orig_x].type;
    board->tiles[orig_y * BOARD_SIZE + orig_x].type = EMPTY;

    int *valid_moves = calloc(sizeof(int), BOARD_SIZE * BOARD_SIZE);
    int valid_moves_tracker = 0;

    // Frontier to track multiple points.
    int *frontier = calloc(BOARD_SIZE * BOARD_SIZE, sizeof(int));
    int frontier_p = 0; // Frontier pointer.
    int *next_frontier = calloc(BOARD_SIZE * BOARD_SIZE, sizeof(int));
    int next_frontier_p = 0; // Next frontier pointer.

    frontier[frontier_p++] = orig_y * BOARD_SIZE + orig_x;

    for (int iteration = 0; iteration < 3; iteration++) {
        while (frontier_p > 0) {
            // Get a point on the frontier
            int frontier_point = frontier[--frontier_p];
            int x = frontier_point % BOARD_SIZE;
            int y = frontier_point / BOARD_SIZE;
            int *points = get_points_around(y, x);

            // Iterate all points surrounding this frontier.
            // Generate a list containing all valid moves from the frontier onward.
            for (int p = 0; p < 6; p++) {
                int point = points[p];
                if (board->tiles[point].type == EMPTY) continue;
                int xx = point % BOARD_SIZE;
                int yy = point / BOARD_SIZE;

                if (!tile_fits(board, x, y, xx, yy)) continue;

                // For all neighbors, if the neighbors neighbors == my neighbors -> valid move.
                int *neighbor_points = get_points_around(yy, xx);
                for (int i = 0; i < 6; i++) {
                    if (board->tiles[neighbor_points[i]].type != EMPTY) continue;
                    for (int j = 0; j < 6; j++) {
                        if (neighbor_points[i] == points[j]) {
                            if (!tile_fits(board, x, y, points[j] % BOARD_SIZE, points[j] / BOARD_SIZE)) continue;

                            // If a unique tile is added, increment the tracker.
                            int added = add_if_unique(valid_moves, valid_moves_tracker, neighbor_points[i]);
                            if (added) {
                                valid_moves_tracker++;
                                next_frontier[next_frontier_p++] = neighbor_points[i];
                            }
                        }
                    }
                }
            }
        }
        // Swap two frontiers.
        int *temp = frontier;
        frontier = next_frontier;
        next_frontier = temp;
        frontier_p = next_frontier_p;
        next_frontier_p = 0;
    }
    board->tiles[orig_y * BOARD_SIZE + orig_x].type = tile_type;

    for (int i = 0; i < frontier_p; i++) {
        add_child(node, frontier[i], tile_type, orig_y * BOARD_SIZE + orig_x);
    }
    free(valid_moves);
    free(frontier);
    free(next_frontier);
}

bool can_move(struct board *board, int x, int y) {
    struct tile_stack *ts = get_from_stack(board, y * BOARD_SIZE + x, false);
    // If removing this tile does not change the tree, it is a valid move.
    if (ts != NULL) return true;


    struct tile *tile = &board->tiles[y * BOARD_SIZE + x];
    int n_hive_tiles = sum_hive_tiles(board) - 1;

    // Store the tile type for later use
    int tile_type = tile->type;
    tile->type = ts == NULL ? EMPTY : ts->type;


    // Check if the points around this point consist of a single spanning tree.
    bool valid = true;
    int *points = get_points_around(y, x);
    for (int p = 0; p < 6; p++) {
        if (board->tiles[points[p]].type == EMPTY) continue;

        // Count how many tiles are connected.
        int connect_count = connected_components(board, points[p]);
        if (connect_count != n_hive_tiles) {
            valid = false;
        }
        break;
    }

    // Restore original tile value.
    tile->type = tile_type;
    return valid;
}


void generate_free_moves(struct node *node, int player_bit, int flags) {
    struct board *board = node->board;

    // We do a full update as soon as we want to move.
    update_can_move(board, node->move.location, node->move.previous_location);

#ifdef CENTERED
    int ly = board->min_y, hy = board->max_y + 1;
    int lx = board->min_x, hx = board->max_x + 1;
#else
    int ly = 0, hy = BOARD_SIZE;
    int lx = 0, hx = BOARD_SIZE;
#endif

    for (int y = ly; y < hy; y++) {
        for (int x = lx; x < hx; x++) {
            struct tile *tile = &board->tiles[y * BOARD_SIZE + x];
            if (tile->type == EMPTY) continue;
            // Only move your own tiles
            if ((tile->type & COLOR_MASK) != player_bit) continue;

            if (!tile->free) continue;

            // If this tile can be removed without breaking the hive, add it to the valid moves list.
            if ((tile->type & TILE_MASK) == L_GRASSHOPPER) {
                generate_grasshopper_moves(node, y, x);
            } else if ((tile->type & TILE_MASK) == L_BEETLE) {
                generate_beetle_moves(node, y, x);
            } else if ((tile->type & TILE_MASK) == L_ANT) {
                // Dont move ants if this flag is set.
                if ((flags & MOVE_NO_ANTS) > 0) {
                    continue;
                }
                generate_ant_moves(node, y, x);
            } else if ((tile->type & TILE_MASK) == L_QUEEN) {
                generate_queen_moves(node, y, x);
            } else if ((tile->type & TILE_MASK) == L_SPIDER) {
                generate_spider_moves(node, y, x);
            }
        }
    }
}



int generate_children(struct node *root, double end_time, int flags) {
    /*
     * Returns 0 if nothing went wrong, an error-code otherwise
     *   1: No more moves could be generated due to turn limit.
     *   2: Time-budget is spent.
     *   3: Memory is full, no new nodes can be allocated.
     */

    // Ensure timely finishing
    struct timespec cur_time;
    clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cur_time);
    if (to_usec(cur_time) / 1e6 > end_time) return ERR_NOTIME;

    // Dont continue generating children if there is no more memory.
    if (max_nodes - n_nodes < 1000) {
        fprintf(stderr, "Not enough memory to hold amount of required nodes (%lld/%lld).\n", n_nodes, max_nodes);
        return ERR_NOMEM;
    }

    if (root->board->turn == MAX_TURNS - 1) {
        return ERR_NOMOVES;
    }

    // Only generate more nodes if you have no nodes yet
    if (list_empty(&root->children)) {
        generate_moves(root, flags);

        if (list_empty(&root->children)) {
            add_child(root, -1, 0, -1);
        }
    }
    return list_empty(&root->children);
}



void generate_moves(struct node *node, int flags) {
    int player_idx = node->board->turn % 2;
    int player_bit = player_idx << COLOR_SHIFT;

    struct board *board = node->board;

    int move = board->turn / 2;

    struct player *player = &board->players[player_idx];
    board->move_location_tracker = 0;
    // By move 4 for each player, the queen has to be placed.
    if (move == 3 && player->queens_left == 1) {
        return generate_placing_moves(node, L_QUEEN | player_bit);
    }

    if (player->spiders_left > 0) {
        generate_placing_moves(node, L_SPIDER | player_bit);
    }
    if (player->beetles_left > 0) {
        generate_placing_moves(node, L_BEETLE | player_bit);
    }
    if (player->grasshoppers_left > 0) {
        generate_placing_moves(node, L_GRASSHOPPER | player_bit);
    }
    if (player->ants_left > 0) {
        generate_placing_moves(node, L_ANT | player_bit);
    }

    // Queens cannot be placed in the first move (tournament rules)
    if (player->queens_left > 0 && move > 0)
        generate_placing_moves(node, L_QUEEN | player_bit);

    // Tiles can only be moved if their queen is on the board.
    if (player->queens_left == 0)
        generate_free_moves(node, player_bit, flags);
}

