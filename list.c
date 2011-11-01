#include <assert.h>
#include "list.h"
#include "string.h"

/**
 * @brief Allocates memory for and returns a new Node.
 *
 * @return Pointer to the newly allocated Node
 */
struct Node *_allocate_node(void);

/**
 * @brief Tests list_init().
 */
void _list_test_init(void);

/**
 * @brief Tests list_size().
 */
void _list_test_size(void);

/**
 * @brief Tests list_get().
 */
void _list_test_get(void);

/**
 * @brief Tests list_add_back().
 */
void _list_test_add_back(void);

/**
 * @brief Tests list_add_front().
 */
void _list_test_add_front(void);

/**
 * @brief Tests list_insert().
 */
void _list_test_insert(void);

/**
 * @brief Tests list_swap().
 */
void _list_test_swap(void);

/**
 * @brief Tests list_remove().
 */
void _list_test_remove(void);

/**
 * @brief Tests list_remove_front().
 */
void _list_test_remove_front(void);

/**
 * @brief Tests list_remove_back().
 */
void _list_test_remove_back(void);

/**
 * @brief Tests list_clear().
 */
void _list_test_clear(void);

/**
 * @brief Tests list_free().
 */
void _list_test_free(void);

void copy(void **dst, void *src) {
	assert(dst);
	assert(src);

	if (*dst != NULL) {
		free(*dst);
	}

	*dst = (int *)malloc((strlen(src) + 1) * sizeof(char));
	strcpy(*dst, src);
	assert(!strcmp(*dst, src));
}

void list_init(
		struct List *list,
		void (*copy)(void **, void *),
		void (*release)(void *)) {
	list->head = NULL;
	list->tail = NULL;
	list->size = 0;
	list->copy = copy;
	list->release = release;
}

size_t list_size(struct List *list) {
	return list->size;
}

void *list_get(struct List *list, size_t index) {
	assert(index < list->size);
	
	if (index == 0) {
		// If the head element was requested
		return list->head->value;
	} else if (index == (list->size - 1)) {
		// If the tail element was requested
		return list->tail->value;
	} else {
		// If an element in the middle of the list was requested
		int i;
		struct Node *current = list->head;
		
		// Iterate through the list until i == index
		for (i = 0; i < index; i++) {
			current = current->next;
			assert(current);
		}
		
		return current->value;
	}
}

void list_add_back(struct List *list, void *e) {
	struct Node *new_node = _allocate_node();
	assert(new_node);
	list->copy(&new_node->value, e);
	new_node->previous = list->tail;
	new_node->next = NULL;
		
	if (list->head == NULL) {		
		// If the list is empty
		// Initialize the head
		list->head = new_node;
	} else {
		// Append the new node after at the tail
		list->tail->next = new_node;
	}
	
	// Set the tail pointer to the new node
	list->tail = new_node;
	
	// Increment the list size
	list->size++;
}

void list_add_front(struct List *list, void * e) {
	struct Node *new_node = _allocate_node();
	assert(new_node);
	list->copy(&new_node->value, e);
	new_node->previous = NULL;
	new_node->next = list->head;
	
	if (list->tail == NULL) {
		// If the list is empty
		// Initialize the tail
		list->tail = new_node;
	} else {
		list->head->previous = new_node;
	}
	
	// Set the head pointer to the new node
	list->head = new_node;
	
	// Increment the list size
	list->size++;
}

void list_insert(struct List *list, size_t index, void *e) {
	assert(index <= list->size);
	
	if (index == 0) {
		// If adding at the head
		list_add_front(list, e);
	} else if (index == list->size) {
		// If adding at the tail
		list_add_back(list, e);
	} else {
		// If adding to the middle of the list
		int i;
		struct Node *current = list->head;
		
		struct Node *new_node = _allocate_node();
		assert(new_node);
		list->copy(&new_node->value, e);
		
		// Iterate through the list until i == index
		for (i = 0; i < index; i++) {
			current = current->next;
			assert(current);
		}
		
		// Insert the new node between the current node and the previous node
		new_node->previous = current->previous;
		new_node->next = current;
		
		// Update old references
		current->previous->next = new_node;
		current->previous = new_node;
			
		// Increment the list size
		list->size++;
	}
}

void list_swap(struct List *list, size_t index0, size_t index1) {
	assert(index0 < list->size);
	assert(index1 < list->size);
	
	struct Node *node0;
	struct Node *node1;
	struct Node tmp;
	
	if (index0 == 0) {
		// If index0 is the head element
		node0 = list->head;
	} else if (index1 == (list->size - 1)) {
		// If index0 is the tail element
		node0 = list->tail;
	} else {
		// If index0 is in the middle of the list
		int i;
		struct Node *current = list->head;
		
		// Iterate through the list until i == index
		for (i = 0; i < index0; i++) {
			current = current->next;
			assert(current);
		}
		
		node0 = current;
	}
	
	if (index1 == 0) {
		// If index1 is the head element
		node1 = list->head;
	} else if (index1 == (list->size - 1)) {
		// If index1 is the tail element
		node1 = list->tail;
	} else {
		// If index1 is in the middle of the list
		int i;
		struct Node *current = list->head;
		
		// Iterate through the list until i == index
		for (i = 0; i < index1; i++) {
			current = current->next;
			assert(current);
		}
		
		node1 = current;
	}
	
	// Swap the two values
	memset(&tmp, 0, sizeof(struct Node));
	list->copy(&tmp.value, node0->value);
	list->copy(&node0->value, node1->value);
	list->copy(&node1->value, tmp.value);
	list->release(tmp.value);
}

void list_remove(struct List *list, size_t index) {
	assert(index < list->size);
	
	if (index == 0) {
		// If removing from the head
		list_remove_front(list);
	} else if (index == (list->size - 1)) {
		// If removing from the tail
		list_remove_back(list);
	} else {
		// If removing from the middle of the list
		int i;
		struct Node *current = list->head;
		
		// Iterate through the list until i == index
		for (i = 0; i < index; i++) {
			current = current->next;
			assert(current);
		}
		
			
		// Remove the current node
		current->previous->next = current->next;
		
		list->release(current->value);
		free(current);
		
		// Decrement the list size
		list->size--;
	}
}

void list_remove_front(struct List *list) {
	// Set the head pointer to the next element in the list
	struct Node *tmp = list->head;
	list->head = list->head->next;

	list->release(tmp->value);
	free(tmp);
	
	// Decrement the list size
	list->size--;
	
	if (list->head == NULL) {
		// If the list is now empty
		// Set the tail pointer to NULL
		list->tail = NULL;
	} else {
		list->head->previous = NULL;
	}
}

void list_remove_back(struct List *list) {
	// Set the tail pointer to the previous element in the list
	struct Node *tmp = list->tail;
	list->tail = list->tail->previous;

	list->release(tmp->value);
	free(tmp);
	
	// Decrement the list size
	list->size--;
	
	if (list->tail == NULL) {
		// If the list is now empty
		// Set the tail pointer to NULL
		list->head = NULL;
	} else {
		list->tail->next = NULL;
	}
}

void list_clear(struct List *list) {
	// Remove the head element until there are no elements remaining
	while (list->head) {
		list_remove_front(list);
	}
	
	assert(list->tail == NULL);
	assert(list->size == 0);
}

void list_free(struct List *list) {
	list_clear(list);
	list->copy = NULL;
	list->release = NULL;
}

void list_test(void) {
	_list_test_init();
	_list_test_size();
	_list_test_get();
	_list_test_add_back();
	_list_test_add_front();
	_list_test_insert();
	_list_test_swap();
	_list_test_remove();
	_list_test_remove_front();
	_list_test_remove_back();
	_list_test_clear();
	_list_test_free();
}

struct Node *_allocate_node(void) {
	struct Node *node = (struct Node *)malloc(sizeof(struct Node));
	memset(node, 0, sizeof(struct Node));
	return node;
}

void _list_test_init(void) {
	struct List list;
	list_init(&list, copy, free);

	assert(list.size == 0);
	assert(list.head == NULL);
	assert(list.tail == NULL);
	assert(list.copy != NULL);
	assert(list.release != NULL);
}

void _list_test_size(void) {
	struct List list;
	list_init(&list, copy, free);

	list_add_back(&list, "1");
	list_add_back(&list, "2");
	list_add_back(&list, "3");
	list_add_back(&list, "4");
	list_add_back(&list, "5");
	list_add_back(&list, "6");

	assert(list_size(&list) == 6);

	list_free(&list);
}

void _list_test_get(void) {
	struct List list;

	list_init(&list, copy, free);

	list_add_back(&list, "1");
	list_add_back(&list, "2");
	list_add_back(&list, "3");
	list_add_back(&list, "4");
	list_add_back(&list, "5");
	list_add_back(&list, "6");

	assert(!strcmp(list_get(&list, 0), "1"));
	assert(!strcmp(list_get(&list, 1), "2"));
	assert(!strcmp(list_get(&list, 2), "3"));
	assert(!strcmp(list_get(&list, 3), "4"));
	assert(!strcmp(list_get(&list, 4), "5"));
	assert(!strcmp(list_get(&list, 5), "6"));

	list_free(&list);
}

void _list_test_add_back(void) {
	_list_test_get();
}

void _list_test_add_front(void) {
	struct List list;
	list_init(&list, copy, free);

	list_add_front(&list, "1");
	list_add_front(&list, "2");
	list_add_front(&list, "3");
	list_add_front(&list, "4");
	list_add_front(&list, "5");
	list_add_front(&list, "6");

	assert(!strcmp(list_get(&list, 0), "6"));
	assert(!strcmp(list_get(&list, 1), "5"));
	assert(!strcmp(list_get(&list, 2), "4"));
	assert(!strcmp(list_get(&list, 3), "3"));
	assert(!strcmp(list_get(&list, 4), "2"));
	assert(!strcmp(list_get(&list, 5), "1"));

	list_free(&list);
}

void _list_test_insert(void) {
	struct List list;
	list_init(&list, copy, free);

	// Insert to an empty list
	list_insert(&list, 0, "1");
	assert(list.size == 1);

	// Insert to the front of a nonempty list
	list_insert(&list, 0, "2");
	assert(list.size == 2);

	// Insert to the end of a nonempty list
	list_insert(&list, 2, "3");
	assert(list.size == 3);

	// Insert to the middle of an nonempty list
	list_insert(&list, 1, "4");
	assert(list.size == 4);

	assert(!strcmp(list_get(&list, 0), "2"));
	assert(!strcmp(list_get(&list, 1), "4"));
	assert(!strcmp(list_get(&list, 2), "1"));
	assert(!strcmp(list_get(&list, 3), "3"));

	list_free(&list);
}

void _list_test_swap(void) {
	struct List list;
	list_init(&list, copy, free);

	list_add_back(&list, "1");
	list_add_back(&list, "2");
	list_add_back(&list, "3");
	list_add_back(&list, "4");
	list_add_back(&list, "5");
	list_add_back(&list, "6");

	list_swap(&list, 1, 4);
	list_swap(&list, 0, 5);
	
	assert(!strcmp(list_get(&list, 0), "6"));
	assert(!strcmp(list_get(&list, 1), "5"));
	assert(!strcmp(list_get(&list, 2), "3"));
	assert(!strcmp(list_get(&list, 3), "4"));
	assert(!strcmp(list_get(&list, 4), "2"));
	assert(!strcmp(list_get(&list, 5), "1"));

	list_free(&list);
}

void _list_test_remove(void) {
	struct List list;
	list_init(&list, copy, free);

	list_add_back(&list, "1");
	list_add_back(&list, "2");
	list_add_back(&list, "3");
	list_add_back(&list, "4");
	list_add_back(&list, "5");
	list_add_back(&list, "6");

	list_remove(&list, 1);
	list_remove(&list, 3);

	assert(list.size == 4);
	assert(!strcmp(list_get(&list, 0), "1"));
	assert(!strcmp(list_get(&list, 1), "3"));
	assert(!strcmp(list_get(&list, 2), "4"));
	assert(!strcmp(list_get(&list, 3), "6"));

	list_free(&list);
}

void _list_test_remove_front(void) {
	struct List list;
	list_init(&list, copy, free);

	list_add_back(&list, "1");
	list_add_back(&list, "2");
	list_add_back(&list, "3");
	list_add_back(&list, "4");
	list_add_back(&list, "5");
	list_add_back(&list, "6");

	list_remove_front(&list);
	list_remove_front(&list);

	assert(list.size == 4);
	assert(!strcmp(list_get(&list, 0), "3"));
	assert(!strcmp(list_get(&list, 1), "4"));
	assert(!strcmp(list_get(&list, 2), "5"));
	assert(!strcmp(list_get(&list, 3), "6"));

	list_free(&list);
}

void _list_test_remove_back(void) {
	struct List list;
	list_init(&list, copy, free);

	list_add_back(&list, "1");
	list_add_back(&list, "2");
	list_add_back(&list, "3");
	list_add_back(&list, "4");
	list_add_back(&list, "5");
	list_add_back(&list, "6");

	list_remove_back(&list);
	list_remove_back(&list);

	assert(list.size == 4);
	assert(!strcmp(list_get(&list, 0), "1"));
	assert(!strcmp(list_get(&list, 1), "2"));
	assert(!strcmp(list_get(&list, 2), "3"));
	assert(!strcmp(list_get(&list, 3), "4"));

	list_free(&list);
}

void _list_test_clear(void) {
	struct List list;
	list_init(&list, copy, free);

	list_add_back(&list, "1");
	list_add_back(&list, "2");
	list_add_back(&list, "3");
	list_add_back(&list, "4");
	list_add_back(&list, "5");
	list_add_back(&list, "6");

	list_clear(&list);

	assert(list.size == 0);
	assert(list.head == NULL);
	assert(list.tail == NULL);
	assert(list.copy != NULL);
	assert(list.release != NULL);

	list_free(&list);
}

void _list_test_free(void) {
	struct List list;
	list_init(&list, copy, free);

	list_add_back(&list, "1");
	list_add_back(&list, "2");
	list_add_back(&list, "3");
	list_add_back(&list, "4");
	list_add_back(&list, "5");
	list_add_back(&list, "6");

	list_free(&list);

	assert(list.size == 0);
	assert(list.head == NULL);
	assert(list.tail == NULL);
	assert(list.copy == NULL);
	assert(list.release == NULL);

	assert(copy != NULL);
	assert(free != NULL);
}

