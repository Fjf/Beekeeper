
#include <string>
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

