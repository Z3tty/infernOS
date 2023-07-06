#ifndef LIST_H
#define LIST_H

#include "../interrupt.h"
#include "../thread.h"
#include "../scheduler.h"

/* 
 * This element must be first in the structure 
 * that will use the linked list 
 */
struct list {
  struct list *next;
  struct list *prev;
};

#define LIST_CAST(x) ((struct list *)(x))

#define LIST_INIT(head) \
  do {            \
    LIST_CAST(head)->next = LIST_CAST(head); \
    LIST_CAST(head)->prev = LIST_CAST(head); \
  } while(0);

#define LIST_LINK(head, element) \
  do {                                      \
    LIST_CAST(element)->prev = LIST_CAST(head)->prev; \
    LIST_CAST(element)->prev->next = LIST_CAST(element);      \
    LIST_CAST(element)->next = LIST_CAST(head);      \
    LIST_CAST(head)->prev = LIST_CAST(element);      \
  } while(0);

#define LIST_UNLINK(element) \
  do {                                               \
    LIST_CAST(element)->next->prev = LIST_CAST(element)->prev; \
    LIST_CAST(element)->prev->next = LIST_CAST(element)->next; \
  } while(0);

#define LIST_FIRST(head, element) \
  element = (typeof(element))LIST_CAST(head)->next;

#define LIST_LAST(head, element) \
  element = (typeof(element))LIST_CAST(head)->prev;

#define LIST_FOR_EACH(head, element_i) \
  for (element_i = (typeof(element_i))(LIST_CAST(head)->next);  \
       element_i != (typeof(element_i))head;       \
       element_i = (typeof(element_i))(LIST_CAST(element_i)->next)) 

/* 
 * The safe version of the for each loop
 * An element that is currently processed can be unlinked 
 */
#define LIST_FOR_EACH_SAFE(head, element_i, helper) \
  for (element_i = (typeof(element_i))(LIST_CAST(head)->next),  \
       helper = (typeof(element_i))(LIST_CAST(element_i)->next);\
       element_i != (typeof(element_i))head;                    \
       element_i = helper,                                      \
       helper = (typeof(element_i))(LIST_CAST(element_i)->next))

/*
 * Safe list head
 */
struct slist {
  struct list list_head;
  int list_spinlock;
};

#define SLIST_INIT(head) \
  LIST_INIT(head); \
  spinlock_init(&((struct slist *)head)->list_spinlock)

#define SLIST_LOCK(head) \
  spinlock_acquire(&((struct slist *)head)->list_spinlock)

#define SLIST_UNLOCK(head) \
  spinlock_release(&((struct slist *)head)->list_spinlock)

#endif
