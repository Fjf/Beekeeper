
#include <string>
#include <iostream>
#include "utils.h"

int to_tile_index(unsigned char tile) {
    int type = tile & TILE_MASK;
    if (type == EMPTY) {
        return 0;
    }

    int color = (tile & COLOR_MASK) >> COLOR_SHIFT;
    int sum = color * N_TILES + 1; // +1, 0 is empty tile.
    int n = ((tile & NUMBER_MASK) >> NUMBER_SHIFT) - 1;

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

unsigned long mix(unsigned long a, unsigned long b, unsigned long c) {
    a = a - b;
    a = a - c;
    a = a ^ (c >> 13);
    b = b - c;
    b = b - a;
    b = b ^ (a << 8);
    c = c - a;
    c = c - b;
    c = c ^ (b >> 13);
    a = a - b;
    a = a - c;
    a = a ^ (c >> 12);
    b = b - c;
    b = b - a;
    b = b ^ (a << 16);
    c = c - a;
    c = c - b;
    c = c ^ (b >> 5);
    a = a - b;
    a = a - c;
    a = a ^ (c >> 3);
    b = b - c;
    b = b - a;
    b = b ^ (a << 10);
    c = c - a;
    c = c - b;
    c = c ^ (b >> 15);
    return c;
}

bool has_neighbour(Board &board, Position &location) {
    auto points = location.get_points_around();
    for (Position &point : points) {
        if (board.tiles[point.y][point.x] != EMPTY) {
            return true;
        }
    }
    return false;
}

bool tile_fits(Board &board, Position &point, Position &new_point) {
    bool tl = board.tiles[point.y - 1][point.x - 1] == EMPTY;
    bool tr = board.tiles[point.y - 1][point.x + 0] == EMPTY;
    bool l = board.tiles[point.y + 0][point.x - 1] == EMPTY;
    bool r = board.tiles[point.y + 0][point.x + 1] == EMPTY;
    bool bl = board.tiles[point.y + 1][point.x + 0] == EMPTY;
    bool br = board.tiles[point.y + 1][point.x + 1] == EMPTY;

    int xd = point.x - new_point.x;
    int yd = point.y - new_point.y;
    if (xd == -1 && yd == 0) {
        // Going to the right
        return tr || br;
    } else if (xd == 1 && yd == 0) {
        // Going to the left
        return tl || bl;
    } else if (xd == 1 && yd == 1) {
        // Going to the top left
        return l || tr;
    } else if (xd == 0 && yd == 1) {
        // Going to the top right
        return r || tl;
    } else if (xd == 0 && yd == -1) {
        // Going to the bottom left
        return l || br;
    } else if (xd == -1 && yd == -1) {
        // Going to the bottom right
        return r || bl;
    }
    std::   cout << board.to_string() << std::endl;
    std::cerr << "Invalid neighbour found, check the coordinates. " << point.to_string() << " -> "
              << new_point.to_string() << std::endl;
    throw std::invalid_argument("Invalid coords.");
}