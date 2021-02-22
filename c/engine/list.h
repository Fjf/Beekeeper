//
// Created by duncan on 04-02-21.
//

#ifndef HIVE_LIST_H
#define HIVE_LIST_H

#define node_foreach(h, n) for ((n) = (h)->children.next; (n) != (h)->children.head; (n) = (n)->next)
#define node_foreach_safe(h, n, nn) for ((n) = (h)->children.next, (nn) = (n)->next; (n) != (h)->children.head; (n) = (nn), (nn) = (n)->next)
#define list_foreach(h, n) for ((n) = (h)->next; (n) != (h)->head; (n) = (n)->next)

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
void list_insert_after(struct list* entry, struct list* new);
bool list_empty(struct list* list);
bool list_remove_object(struct list* list);
void list_remove(struct list* entry);

#endif //HIVE_LIST_H
