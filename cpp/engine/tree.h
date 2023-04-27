
#ifndef BEEKEEPER_TREE_H
#define BEEKEEPER_TREE_H

#include <list>
#include "position.h"
#include "board.h"
#include <iostream>
#include <cstring>

class Move {
public:
    uint8_t tile;
    uint8_t next_to;
    uint8_t direction;
    Position previous_location;
    Position location;

    Move() = default;

    std::string to_string() const;

    std::string tile_string(uint8_t tile_type) const;
};


#pragma pack(1)
class alignas(8) Node {
public:
    Move move;
    Board board;
    Node *parent = nullptr;
    std::list<Node, std::allocator<Node>> children;

    Node() = default;

    inline Node copy() const {
        Node node;
        const size_t n_bytes = ((sizeof(Move) + sizeof(Board) + 8) / 8) * 8;
        memcpy((void*) &node, this, n_bytes);
        return node;
    }

    void print();

    [[nodiscard]] std::string get_move() const;

};


void node_init(struct node *node, void *data);

void node_add_child(struct node *node, struct node *child);

void node_free_children(struct node *root);

void node_free(struct node *root);

void node_copy(struct node *dest, struct node *src);

char *string_move(struct node *node);

void print_move(struct node *node);

struct node *list_get_node(struct list *list);


#endif //BEEKEEPER_TREE_H
