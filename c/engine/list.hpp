//
// Created by duncan on 04-02-21.
//

#ifndef HIVE_LIST_HPP
#define HIVE_LIST_HPP

#define node_foreach(h, n) for ((n) = (h)->children.next; (n) != (h)->children.head; (n) = (n)->next)
#define node_foreach_safe(h, n, nn) for ((n) = (h)->children.next, (nn) = (n)->next; (n) != (h)->children.head; (n) = (nn), (nn) = (n)->next)
#define list_foreach(h, n) for ((n) = (h)->next; (n) != (h)->head; (n) = (n)->next)

#include <stdio.h>
#include <stdbool.h>

#define offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({                      \
      const typeof(((type *)0)->member) * __mptr = (ptr);     \
      (type *)((char *)__mptr - offset_of(type, member)); })

//template<class P, class M>
//P* container_of(M* ptr, const M P::*member) {
//        return (P*)( (char*)ptr - offsetof(member));
//}


struct list {
    struct list* head;
    struct list* next;
    struct list* prev;
};

extern void list_init(struct list* list);
extern void list_add(struct list* list, struct list* entry);
extern void list_insert_after(struct list* entry, struct list* new_entry);
extern bool list_empty(struct list* list);
extern bool list_remove_object(struct list* list);
extern void list_remove(struct list* entry);

#endif //HIVE_LIST_HPP
