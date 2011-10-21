#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dynarr.h"
#include "types.h"

BOOL testInit(void);
BOOL testFree(void);
BOOL testSetCapacity(void);
BOOL testSize(void);
BOOL testAdd(void);
BOOL testGet(void);
BOOL testPut(void);
BOOL testSwap(void);
BOOL testRemove(void);

int main(int argc, char** argv)
{
	testInit();
	testFree();
	testSetCapacity();
	testSize();
	testAdd();
	testGet();
	testPut();
	testSwap();
	testRemove();
	
	return 0;
}

BOOL testInit(void) {
	const int TEST_CAP = 10;
	struct DynArr arr;

	dynarr_init(&arr, TEST_CAP);

	assert(arr.data != NULL);
	assert(arr.size == 0);
	assert(arr.capacity == TEST_CAP);

	dynarr_free(&arr);
	
	return TRUE;
}

BOOL testFree(void) {
	const int TEST_CAP = 10;
	struct DynArr arr;

	dynarr_init(&arr, TEST_CAP);
	dynarr_free(&arr);

	assert(arr.data == NULL);
	assert(arr.size == 0);
	assert(arr.capacity == 0);
	
	return TRUE;
}

BOOL testSetCapacity(void) {
	const int TEST_CAP = 10;
	const int TEST_SET_CAP = 42;
	struct DynArr arr;

	dynarr_init(&arr, TEST_CAP);

	assert(arr.capacity == TEST_CAP);
	_dynarr_setCapacity(&arr, TEST_SET_CAP);
	assert(arr.capacity == TEST_SET_CAP);

	dynarr_free(&arr);
	
	return TRUE;
}

BOOL testSize(void) {
	const int TEST_CAP = 10;
	const char* TEST_STRING = "test string";
	struct DynArr arr;

	dynarr_init(&arr, TEST_CAP);

	assert(dynarr_size(&arr) == 0);
	dynarr_add(&arr, TEST_STRING);
	assert(dynarr_size(&arr) == 1);

	dynarr_free(&arr);
	
	return TRUE;
}

BOOL testAdd(void) {
	const int TEST_CAP = 1;
	const char* TEST_STRING = "test string";
	const char* TEST_STRING2 = "another test string";
	struct DynArr arr;

	dynarr_init(&arr, TEST_CAP);

	dynarr_add(&arr, TEST_STRING);
	assert(!strcmp(dynarr_get(&arr, 0), TEST_STRING));
	dynarr_add(&arr, TEST_STRING2);
	assert(!strcmp(dynarr_get(&arr, 0), TEST_STRING));
	assert(!strcmp(dynarr_get(&arr, 1), TEST_STRING2));

	dynarr_free(&arr);
	
	return TRUE;
}

BOOL testGet(void) {
	const int TEST_CAP = 10;
	const char* TEST_STRING = "test string";
	const char* TEST_STRING2 = "another test string";
	struct DynArr arr;

	dynarr_init(&arr, TEST_CAP);

	dynarr_add(&arr, TEST_STRING);
	dynarr_add(&arr, TEST_STRING2);
	assert(!strcmp(dynarr_get(&arr, 0), TEST_STRING));
	assert(!strcmp(dynarr_get(&arr, 1), TEST_STRING2));

	dynarr_free(&arr);
	
	return TRUE;
}

BOOL testPut(void) {
	const int TEST_CAP = 10;
	const char* TEST_STRING = "test string";
	const char* TEST_STRING2 = "another test string";
	struct DynArr arr;

	dynarr_init(&arr, TEST_CAP);

	dynarr_add(&arr, TEST_STRING);
	dynarr_put(&arr, 0, TEST_STRING2);
	assert(dynarr_size(&arr) == 1);
	assert(!strcmp(dynarr_get(&arr, 0), TEST_STRING2));

	dynarr_free(&arr);
	
	return TRUE;
}

BOOL testSwap(void) {
	const int TEST_CAP = 10;
	const char* TEST_STRING = "test string";
	const char* TEST_STRING2 = "another test string";
	struct DynArr arr;

	dynarr_init(&arr, TEST_CAP);

	dynarr_add(&arr, TEST_STRING);
	dynarr_add(&arr, TEST_STRING2);
	dynarr_swap(&arr, 0, 1);
	assert(!strcmp(dynarr_get(&arr, 0), TEST_STRING2));
	assert(!strcmp(dynarr_get(&arr, 1), TEST_STRING));

	dynarr_free(&arr);
	
	return TRUE;
}

BOOL testRemove(void) {
	const int TEST_CAP = 10;
	const char* TEST_STRING = "test string";
	const char* TEST_STRING2 = "another test string";
	struct DynArr arr;

	dynarr_init(&arr, TEST_CAP);

	dynarr_add(&arr, TEST_STRING);
	dynarr_add(&arr, TEST_STRING2);
	dynarr_remove(&arr, 0);
	assert(dynarr_size(&arr) == 1);
	assert(!strcmp(dynarr_get(&arr, 0), TEST_STRING2));

	dynarr_free(&arr);
	
	return TRUE;
}

