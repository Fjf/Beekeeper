//
// Created by duncan on 04-02-21.
//

#include "list.hpp"

void list_init(struct list* new_entry) {
    new_entry->prev = new_entry;
    new_entry->next = new_entry;
    new_entry->head = new_entry;
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

void list_insert_after(struct list* entry, struct list* new_entry) {
    new_entry->prev = entry;
    new_entry->next = entry->next;
    new_entry->head = entry->head;

    entry->next->prev = new_entry;
    entry->next = new_entry;
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
