
#ifndef BEEKEEPER_BOARD_H
#define BEEKEEPER_BOARD_H

#include <vector>
#include <string>
#include "position.h"
#include "utils.h"

class Board {
public:
    unsigned char tiles[BOARD_SIZE][BOARD_SIZE] = {0};
    unsigned char free[BOARD_SIZE][BOARD_SIZE] = {0};

    int turn = 0;

    struct player_info {
        unsigned char beetles_left = N_BEETLES;
        unsigned char grasshoppers_left = N_GRASSHOPPERS;
        unsigned char queens_left = N_QUEENS;
        unsigned char ants_left = N_ANTS;
        unsigned char spiders_left = N_SPIDERS;
    } players[2];

    Position light_queen = Position(-1, -1);
    Position dark_queen = Position(-1, -1);

    Position min = Position(BOARD_SIZE / 2, BOARD_SIZE / 2);
    Position max = Position(BOARD_SIZE / 2, BOARD_SIZE / 2);

    char n_stacked = 0;

    struct tile_stack {
        unsigned char type = 0;
        Position position = Position(-1, -1);
        unsigned char z = 0;
    } stack[TILE_STACK_SIZE] = {0};

    long long zobrist_hash = 0;
//    std::vector<long long> hash_history = std::vector<long long>();

    bool has_updated = false;

    int finished();

    Board();

    std::string to_string();

    int sum_hive_tiles();

    tile_stack *get_from_stack(Position &position, bool pop);

    void get_min_x_y();

    void get_max_x_y();

    void center();

    void update_can_move(Position &position, Position &previous_position);

    unsigned char &operator[](Position &position) { return tiles[position.y][position.x]; };

private:

    int count_tiles_around(Position &position);

    bool is_surrounded(Position &position);


    void update_free_tiles();

    bool can_move(Position &position);

    int connected_components(Position &position);
};

#endif //BEEKEEPER_BOARD_H
