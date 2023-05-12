
#ifndef BEEKEEPER_TREE_H
#define BEEKEEPER_TREE_H

#include <list>
#include "position.h"
#include "board.h"
#include <iostream>
#include <cstring>
#include "move.h"


#pragma pack(push, 1)

template<typename T>
class alignas(8) BaseNode {
public:
    Move move{};
    Board board{};
    BaseNode<T> *parent = nullptr;
    std::list<BaseNode<T>> children = std::list<BaseNode<T>>();
    T data{};

    BaseNode() = default;
    BaseNode(const BaseNode<T> &) = default;

    [[nodiscard]] inline BaseNode<T> copy() const {
        BaseNode<T> node;
        const size_t n_bytes = ((sizeof(Move) + sizeof(Board) + 8) / 8) * 8;
        memcpy((void *) &node, this, n_bytes);
        return node;
    }

    void print();

    int generate_children();

    void generate_directional_grasshopper_moves(Position &orig_pos, int x_incr, int y_incr);

    void generate_grasshopper_moves(Position &orig_pos);

    void generate_ant_moves(Position &orig);

    void generate_queen_moves(Position &orig);

    void generate_beetle_moves(Position &point);

    void generate_spider_moves(Position &orig);

    void generate_free_moves(int player_bit);

    void generate_placing_moves(uint8_t type);


    template<bool placed>
    void add_child(const Position &location, int type, const Position &previous_location);

    [[nodiscard]] std::string get_move() const;

    void generate_moves();
};

#pragma pack(pop)


#endif //BEEKEEPER_TREE_H
