
#ifndef BEEKEEPER_POSITION_H
#define BEEKEEPER_POSITION_H

#include <string>
#include <list>
#include <vector>
#include <array>
#include "constants.h"

class Position {
public:
    int8_t x;
    int8_t y;


    Position() = default;

    Position(int8_t x, int8_t y) {
        this->x = x;
        this->y = y;
    }

    bool operator==(Position &other) const {
        return other.x == x and other.y == y;
    }

    bool operator==(const Position &other) const {
        return other.x == x and other.y == y;
    }

    bool operator!=(Position &other) const {
        return other.x != x or other.y != y;
    }

    int flat_index() const { return y * BOARD_SIZE + x; };

    std::string to_string() const { return std::to_string(x) + "," + std::to_string(y); };

    friend auto operator<<(std::ostream &os, Position const &m) -> std::ostream & {
        return os << m.to_string();
    }

    const std::array<Position, 6> get_points_around() const;

    static std::array<Position, 6> get_points_around(int x, int y);
};

/*
 * Returns an array containing array indices around a given x,y coordinate.
 */
inline const std::array<Position, 6> Position::get_points_around() const {
    return {
            Position(x - 1, y - 1),
            Position(x + 0, y - 1),
            Position(x - 1, y + 0),
            Position(x + 1, y + 0),
            Position(x + 0, y + 1),
            Position(x + 1, y + 1),
    };
}

inline std::array<Position, 6> Position::get_points_around(int x, int y) {
    return {
            Position(x - 1, y - 1),
            Position(x + 0, y - 1),
            Position(x - 1, y + 0),
            Position(x + 1, y + 0),
            Position(x + 0, y + 1),
            Position(x + 1, y + 1),
    };
}

#endif // BEEKEEPER_POSITION_H
