/* 	dynarr.h : Dynamic Array implementation. */
#ifndef DYNARR_H
#define DYNARR_H

typedef char* string;

struct DynArr {
	string *data;	/* pointer to the data array */
	int size;		/* Number of elements in the array */
	int capacity;	/* capacity ofthe array */
};

/* Dynamic Array Functions */
void dynarr_init(struct DynArr *v, int capacity);	

void dynarr_free(struct DynArr *v);

void _dynarr_setCapacity(struct DynArr *v, int newCap);
int dynarr_size(struct DynArr *v);

void dynarr_add(struct DynArr *v, string val);
string dynarr_get(struct DynArr *v, int pos);
void dynarr_put(struct DynArr *v, int pos, string val);
void dynarr_swap(struct DynArr *v, int i, int  j);
void dynarr_remove(struct DynArr *v, int idx);

#endif // DYNARR_H

