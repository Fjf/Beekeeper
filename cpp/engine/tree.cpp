
#include <string>
#include <algorithm>
#include <iostream>
#include "tree.h"



std::string Node::get_move() const {
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

void Node::print() {
    std::cout << "Move: " << move.to_string() << std::endl;
    std::cout << board.to_string() << std::endl;
}

std::string Move::to_string() const {
    std::string response;
    unsigned char color = tile & COLOR_MASK;
    if (color == LIGHT) {
        response += 'w';
    } else {
        response += 'b';
    }

    unsigned char type = tile & TILE_MASK;
    if (type == L_ANT) {
        response += 'A';
    } else if (type == L_BEETLE) {
        response += 'B';
    } else if (type == L_QUEEN) {
        response += 'Q';
    } else if (type == L_GRASSHOPPER) {
        response += 'G';
    } else if (type == L_SPIDER) {
        response += 'S';
    }

    unsigned char number = (tile & NUMBER_MASK) >> NUMBER_SHIFT;
    response += (std::to_string(number));
    return response;
}

Move::Move() {
    tile = 0;
    next_to = 0;
    direction = 0;
    previous_location = Position(0, 0);
    location = Position(0, 0);
}
