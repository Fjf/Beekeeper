//
// Created by duncan on 04-02-21.
//

#include <stdlib.h>
#include <stdbool.h>
#include "list.h"



void list_init(struct list* new) {
    new->prev = new;
    new->next = new;
    new->head = new;
}

void list_add(struct list* list, struct list* entry) {
    // Set new pn_node list pointers.
    entry->prev = list->prev;
    entry->next = list;
    entry->head = list;
    // Set list pn_node pointers.
    list->prev->next = entry;
    list->prev = entry;
}

void list_insert_after(struct list* entry, struct list* new) {
    new->prev = entry;
    new->next = entry->next;
    new->head = entry->head;

    entry->next->prev = new;
    entry->next = new;
}

void list_remove(struct list* entry) {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->prev = entry;
    entry->next = entry;
}

bool list_empty(struct list *list) {
    return list->next == list && list->prev == list;
}
