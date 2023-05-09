
#ifndef BEEKEEPER_UTILS_H
#define BEEKEEPER_UTILS_H


// Do includes after defines.
#include "position.h"
#include "board.h"

int to_tile_index(unsigned char tile);

// http://www.concentric.net/~Ttwang/tech/inthash.htm
unsigned long mix(unsigned long a, unsigned long b, unsigned long c);

bool tile_fits(Board &board, Position &point, Position &new_point);

bool has_neighbour(Board &board, Position &location);

#endif //BEEKEEPER_UTILS_H
