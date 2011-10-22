#ifndef STACK_H
#define STACK_H

#include "dynarr.h"

typedef struct DynArr Stack;

void stack_init(Stack *s, int cap);
void stack_free(Stack *s);
void stack_push(Stack *s, string str);
string stack_pop(Stack *s);
string stack_top(Stack *s);
int stack_size(Stack *s);

void stack_test(void);

#endif // STACK_H

