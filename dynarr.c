/*	dynarr.c: Dynamic Array implementation. */
#include <assert.h>
#include <stdlib.h>
#include "dynarr.h"

/* ************************************************************************
	Dynamic Array Functions
************************************************************************ */

/* Initialize (including allocation of data array) dynamic array.

	param: 	v		pointer to the dynamic array
	param:	cap 	capacity of the dynamic array
	pre:	v is not null
	post:	internal data array can hold cap elements
	post:	v->data is not null
*/
void dynarr_init(struct DynArr *v, int capacity) {
	assert(capacity > 0);
	v->capacity = capacity;
	v->data = (string *)malloc(sizeof(string) * capacity);
	assert(v->data != NULL);
	v->size = 0;
}

/* Deallocate data array in dynamic array. 
	param: 	v		pointer to the dynamic array
	pre:	none
	post:	v.data points to null
	post:	size and capacity are 0
	post:	the memory used by v->data is freed
*/
void dynarr_free(struct DynArr *v) {
	if(v->data != NULL) {
		int i;
		for (i = 0; i < v->size; i++) {
			if (v->data[i] != NULL) {
				free(v->data[i]);
				v->data[i] = NULL;
			}
		}
		
		free(v->data); 	/* free the space on the heap */
		v->data = NULL;   	/* make it point to null */
	}

	v->size = 0;
	v->capacity = 0;
}

/* Resizes the underlying array to be the size cap 

	param: 	v		pointer to the dynamic array
	param:	cap		the new desired capacity
	pre:	v is not null
	post:	v has capacity newCap
*/
void _dynarr_setCapacity(struct DynArr *v, int newCap) {
	int i;
	string *oldData = v->data;
	int oldSize = v->size;

	/* Create a new dyn array with larger underlying array */
	dynarr_init(v, newCap);
	for(i = 0; i < oldSize; i++) {
		v->data[i] = oldData[i];
	}

	v->size = oldSize;

	/* Remember, init did not free the original data */
	free(oldData);
}

/* Get the size of the dynamic array

	param: 	v		pointer to the dynamic array
	pre:	v is not null
	post:	none
	ret:	the size of the dynamic array
*/
int dynarr_size(struct DynArr *v) {
	return v->size;
}

/* 	Adds an element to the end of the dynamic array

	param: 	v		pointer to the dynamic array
	param:	val		the value to add to the end of the dynamic array
	pre:	the dynArry is not null
	post:	size increases by 1
	post:	if reached capacity, capacity is doubled
	post:	val is in the last utilized position in the array
*/
void dynarr_add(struct DynArr *v, string val) {
	/* Check to see if a resize is necessary */
	if(v->size >= v->capacity) {
		_dynarr_setCapacity(v, 2 * v->capacity);
	}
	
	v->data[v->size] = (string)malloc((strlen(val) + 1) * sizeof(char));
	assert(v->data[v->size]);
	strcpy(v->data[v->size], val);
	v->size++;
}

/*	Get an element from the dynamic array from a specified position
	
	param: 	v		pointer to the dynamic array
	param:	pos		integer index to get the element from
	pre:	v is not null
	pre:	v is not empty
	pre:	pos < size of the dyn array and >= 0
	post:	no changes to the dyn Array
	ret:	value stored at index pos
*/

string dynarr_get(struct DynArr *v, int pos) {
   assert(pos < v->size);
   assert(pos >= 0);
   
   return v->data[pos];
}

/*	Put an item into the dynamic array at the specified location,
	overwriting the element that was there

	param: 	v		pointer to the dynamic array
	param:	pos		the index to put the value into
	param:	val		the value to insert 
	pre:	v is not null
	pre:	v is not empty
	pre:	pos >= 0 and pos < size of the array
	post:	index pos contains new value, val
*/
void dynarr_put(struct DynArr *v, int pos, string val) {
	assert(pos < v->size);
	assert(pos >= 0);

	free(v->data[pos]);
	v->data[pos] = NULL;
	v->data[pos] = (string)malloc((strlen(val) + 1) * sizeof(char));	
	assert(v->data[pos]);
	strcpy(v->data[pos], val);
}

/*	Swap two specified elements in the dynamic array

	param: 	v		pointer to the dynamic array
	param:	i,j		the elements to be swapped
	pre:	v is not null
	pre:	v is not empty
	pre:	i, j >= 0 and i,j < size of the dynamic array
	post:	index i now holds the value at j and index j now holds the value at i
*/
void dynarr_swap(struct DynArr *v, int i, int  j) {
	string temp;

	assert(i < v->size);
	assert(j < v->size);
	assert(i >= 0);
	assert(j >= 0);

	temp = v->data[i];
	v->data[i] = v->data[j];
	v->data[j] = temp;

}

/*	Remove the element at the specified location from the array,
	shifts other elements back one to fill the gap

	param: 	v		pointer to the dynamic array
	param:	idx		location of element to remove
	pre:	v is not null
	pre:	v is not empty
	pre:	idx < size and idx >= 0
	post:	the element at idx is removed
	post:	the elements past idx are moved back one
*/
void dynarr_remove(struct DynArr *v, int idx) {
	int i;

	assert(idx < v->size);
	assert(idx >= 0);

	free(v->data[idx]);
	v->data[idx] = NULL;

	/*Move all elements up*/
	for(i = idx; i <= v->size - 2; i++) {
		v->data[i] = v->data[i + 1];
	}

	v->size--;
}

