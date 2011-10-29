#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

struct Node {
	void *value;
	struct Node *previous;
	struct Node *next;
};

struct List {
	struct Node *head;
	struct Node *tail;
	size_t size;
	void (*copy)(void **, void *);
	void (*release)(void *);
};

// Initializers
void list_init(
	struct List *list,
	void (*copy)(void **, void *),
	void (*release)(void *));

// Accessors
size_t list_size(struct List *list);
void *list_get(struct List *list, size_t index);

// Modifiers
void list_add_back(struct List *list, void *e);
void list_add_front(struct List *list, void *e);
void list_insert(struct List *list, size_t index, void *e);
void list_swap(struct List *list, size_t index0, size_t index1);
void list_remove(struct List *list, size_t index);
void list_remove_front(struct List *list);
void list_remove_back(struct List *list);
void list_clear(struct List *list);
void list_free(struct List *list);

void list_test(void);

#endif // LIST_H

