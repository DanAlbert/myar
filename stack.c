#include <assert.h>
#include <stdlib.h>
#include "stack.h"
#include "types.h"

BOOL stack_testInit(void);
BOOL stack_testFree(void);
BOOL stack_testPush(void);
BOOL stack_testPop(void);
BOOL stack_testTop(void);
BOOL stack_testSize(void);

void stack_init(Stack *s, int cap) {
	dynarr_init(s, cap);
}

void stack_free(Stack *s) {
	dynarr_free(s);
}

void stack_push(Stack *s, string str) {
	dynarr_add(s, str);
}

string stack_pop(Stack *s) {
	string old = dynarr_get(s, dynarr_size(s) - 1);
	string str = (string)malloc((strlen(old) + 1) * sizeof(char));
	strcpy(str, old);
	dynarr_remove(s, dynarr_size(s) - 1);
	return str;
}

string stack_top(Stack *s) {
	return dynarr_get(s, dynarr_size(s) - 1);
}

int stack_size(Stack *s) {
	return dynarr_size(s);
}

void stack_test(void) {
	stack_testInit();
	stack_testFree();
	stack_testPush();
	stack_testPop();
	stack_testTop();
	stack_testSize();
}

BOOL stack_testInit(void) {
	const int TEST_CAP = 10;
	Stack s;

	stack_init(&s, TEST_CAP);

	assert(s.data != NULL);
	assert(s.size == 0);
	assert(s.capacity == TEST_CAP);

	stack_free(&s);

	return TRUE;
}

BOOL stack_testFree(void) {
	const int TEST_CAP = 10;
	Stack s;

	stack_init(&s, TEST_CAP);
	stack_free(&s);

	assert(s.data == NULL);
	assert(s.size == 0);
	assert(s.capacity == 0);

	return TRUE;
}

BOOL stack_testPush(void) {
	const int TEST_CAP = 10;
	const char *TEST_STR1 = "test string 1";
	const char *TEST_STR2 = "test string 2";
	Stack s;

	stack_init(&s, TEST_CAP);

	stack_push(&s, TEST_STR1);
	stack_push(&s, TEST_STR2);
	assert(!strcmp(TEST_STR1, s.data[0]));
	assert(!strcmp(TEST_STR2, s.data[1]));

	stack_free(&s);

	return TRUE;
}

BOOL stack_testPop(void) {
	const int TEST_CAP = 10;
	const char *TEST_STR1 = "test string 1";
	const char *TEST_STR2 = "test string 2";
	Stack s;

	char *tmp;

	stack_init(&s, TEST_CAP);

	stack_push(&s, TEST_STR1);
	stack_push(&s, TEST_STR2);

	tmp = stack_pop(&s);
	assert(!strcmp(TEST_STR2, tmp));
	free(tmp);

	tmp = stack_pop(&s);
	assert(!strcmp(TEST_STR1, tmp));
	free(tmp);

	stack_free(&s);

	return TRUE;
}

BOOL stack_testTop(void) {
	const int TEST_CAP = 10;
	const char *TEST_STR1 = "test string 1";
	const char *TEST_STR2 = "test string 2";
	Stack s;

	char *tmp;

	stack_init(&s, TEST_CAP);

	stack_push(&s, TEST_STR1);
	stack_push(&s, TEST_STR2);

	assert(!strcmp(TEST_STR2, stack_top(&s)));
	free(stack_pop(&s));
	assert(!strcmp(TEST_STR1, stack_top(&s)));

	stack_free(&s);

	return TRUE;
}

BOOL stack_testSize(void) {
	const int TEST_CAP = 10;
	const char *TEST_STR1 = "test string 1";
	const char *TEST_STR2 = "test string 2";
	Stack s;

	stack_init(&s, TEST_CAP);

	stack_push(&s, TEST_STR1);
	stack_push(&s, TEST_STR2);

	assert(stack_size(&s) == 2);

	stack_free(&s);

	return TRUE;
}

