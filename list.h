#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

/**
 * An element in the linked list
 */
struct Node {
	/// Data for the element
	void *value;
	
	/// Previous element in the list
	struct Node *previous;
	
	/// Next element in the list
	struct Node *next;
};

/**
 * Linked list
 */
struct List {
	/// First element in the list
	struct Node *head;
	
	/// Last element in the list
	struct Node *tail;
	
	/// Number of elements in the list
	size_t size;
	
	/**
	 * @brief Pointer to copy function for the data type contained by the list
	 *
	 * @param dst Reference to a pointer to a block of memory to be allocated and copied to
	 * @param src Pointer to block of memory to copy from
	 */
	void (*copy)(void **dst, void *src);
	
	/**
	 * @brief Pointer to release function for the data type contained by the list
	 *
	 * @param res Pointer to block of memory to be released
	 */
	void (*release)(void *res);
};

/**
 * @brief Initialized a link list
 *
 * Preconditions: list is not NULL, copy is a pointer to a copy function, release is a pointer to a release function
 * Postconditions: head is NULL, tail is NULL, size is 0, function pointers are set
 *
 * @param list Pointer to the list structure
 * @param copy Pointer to a copy function. The first parameter is the destination and must be a pointer to a pointer so that memory may be allocated. The second parameter is the source.
 * @param release Pointer to a function which releases memory associated with the resource pointed to by the parameter.
 */
void list_init(
	struct List *list,
	void (*copy)(void **, void *),
	void (*release)(void *));

/**
 * @brief Retrieves the number of elements in the list.
 *
 * Preconditions: list is not NULL, list is initialized
 * 
 * Post conditions: 
 *
 * @param list Pointer to list structure
 * @return Number of elements in the list
 */
size_t list_size(struct List *list);

/**
 * @brief Retrives the element in the list located at index.
 *
 * Preconditions: list is not NULL, list is initialized, index is between zero and list.size
 * 
 * Postconditions: 
 *
 * @param list Pointer to list structure
 * @param index Index of the element to retrieve
 * @return Pointer to the element at the specified index
 */
void *list_get(struct List *list, size_t index);

/**
 * @brief Adds the element e to the back of the list.
 *
 * Preconditions: list is not NULL, list is initialized, e is not NULL
 *
 * Postconditions: The element e has been added to the back of the list
 *
 * @param list Pointer to list structure
 * @param e Pointer to element to add to the list
 */
void list_add_back(struct List *list, void *e);

/**
 * @brief Adds the element e to the front of the list.
 *
 * Preconditions: list is not NULL, list is initialized, e is not NULL
 *
 * Postconditions: The element e has been added to the front of the list
 *
 * @param list Pointer to list structure
 * @param e Pointer to element to add to the list
 */
void list_add_front(struct List *list, void *e);

/**
 * @brief Adds the element e to the middle of the list.
 *
 * Preconditions: list is not NULL, list is initialized, e is not NULL, index is between zero and list.size
 *
 * Postconditions: The element e has been added to the list at the specified index
 *
 * @param list Pointer to list structure
 * @param e Pointer to element to add to the list
 * @param index Index at which to insert the element
 */
void list_insert(struct List *list, size_t index, void *e);

/**
 * @brief Swaps the positions of two elements in the list.
 *
 * Preconditions: list is not NULL, list is initialized, index0 is between zero and list.size, index1 is between 0 and list.size
 *
 * Postconditions: The two elements have been swapped
 *
 * @param list Pointer to list structure
 * @param index0 Index of the first element to be swapped
 * @param index1 Index of the second element to be swapped
 */
void list_swap(struct List *list, size_t index0, size_t index1);

/**
 * @brief Removes an element from the list.
 *
 * Preconditions: list is not NULL, list is initialized, index is between zero and list.size
 *
 * Postconditions: The element at the specified index has been removed from the list
 *
 * @param list Pointer to list structure
 * @param index Index from which to remove an element
 */
void list_remove(struct List *list, size_t index);

/**
 * @brief Removes an element from the front of the list.
 *
 * Preconditions: list is not NULL, list is initialized, list is not empty
 *
 * Postconditions: The element at the front of the list has been removed
 * 
 * @param list Pointer to list structure
 */
void list_remove_front(struct List *list);

/**
 * @brief Removes an element from the back of the list.
 *
 * Preconditions: list is not NULL, list is initialized, list is not empty
 *
 * Postconditions: The element at the back of the list has been removed
 * 
 * @param list Pointer to list structure
 */
void list_remove_back(struct List *list);

/**
 * Removes all elements from the list.
 *
 * Preconditions: list is not NULL, list is initialized
 *
 * Postconditions: The list is empty
 *
 * @param list Pointer to the list structure
 */
void list_clear(struct List *list);

/**
 * Removes all elements from the list and disassociates function pointers.
 *
 * Preconditions: list is not NULL, list is initialized
 *
 * Postconditions: The list is empty, list.copy is NULL, list.release is NULL
 *
 * @param list Pointer to the list structure
 */
void list_free(struct List *list);

/**
 * Runs all unit tests for this list implementation.
 */
void list_test(void);

#endif // LIST_H

