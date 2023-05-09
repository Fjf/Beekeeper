
#ifndef BEEKEEPER_MOVE_H
#define BEEKEEPER_MOVE_H

#include <cstring>
#include <iostream>
#include "position.h"

class Move {
public:
    uint8_t tile;
    uint8_t next_to;
    uint8_t direction;
    Position previous_location;
    Position location;

    Move() = default;

    [[nodiscard]] std::string to_string() const;

    [[nodiscard]] static std::string tile_string(uint8_t tile_type);
};


#endif //BEEKEEPER_MOVE_H
