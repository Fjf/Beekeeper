
#ifndef BEEKEEPER_TREE_H
#define BEEKEEPER_TREE_H

#include <list>
#include "position.h"
#include "board.h"
#include <iostream>

#pragma pack(1)
class Move {
public:
    unsigned char tile = 0;
    unsigned char next_to = 0;
    unsigned char direction = 0;
    Position previous_location = Position(0, 0);
    Position location = Position(0, 0);

    Move();

    std::string to_string() const;
};


#pragma pack(1)
class Node {
public:
    std::list<Node, std::allocator<Node>> children;
    Node *parent = nullptr;
    Move move = Move();
    Board board = Board();
//    MCTS data;

    Node() = default;

    inline Node copy() const {
        Node node = Node();
        node.move = move;
        node.board = board;
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
