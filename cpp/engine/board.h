
#ifndef BEEKEEPER_BOARD_H
#define BEEKEEPER_BOARD_H

#include "constants.h"
#include <vector>
#include <string>
#include "position.h"

class Board {
public:
    uint8_t tiles[BOARD_SIZE][BOARD_SIZE];
    uint8_t free[BOARD_SIZE][BOARD_SIZE];

    int32_t turn;

    struct player_info {
        uint8_t beetles_left;
        uint8_t grasshoppers_left;
        uint8_t queens_left;
        uint8_t ants_left;
        uint8_t spiders_left;
    } players[2];

    Position light_queen;
    Position dark_queen;

    Position min;
    Position max;

    int8_t n_stacked;

    struct tile_stack {
        uint8_t type;
        uint8_t z;
        Position position;
    } stack[TILE_STACK_SIZE];

    int64_t zobrist_hash;

    bool has_updated;


    int finished();

    void initialize();

    Board() = default;

    std::string to_string();

    int sum_hive_tiles();

    tile_stack *get_from_stack(const Position &position, bool pop);

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
