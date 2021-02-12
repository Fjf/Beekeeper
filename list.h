//
// Created by duncan on 04-02-21.
//

#ifndef HIVE_LIST_H
#define HIVE_LIST_H

#define list_foreach(node, head) for ((head) = (node)->children.next; (head) != (node)->children.head; (head) = (head)->next)

#include <stddef.h>
#include "stdbool.h"

#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

struct list {
    struct list* head;
    struct list* next;
    struct list* prev;
};

void list_init(struct list* list);
void list_add(struct list* list, struct list* entry);
bool list_empty(struct list* list);
bool list_remove_object(struct list* list);
void list_remove(struct list* entry);

#endif //HIVE_LIST_H
