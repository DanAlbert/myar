#include "dyntest.h"
#include "stack.h"

int main(int argc, char **argv) {
	printf("list_test()\n");
	list_test();
	printf("dynarr_test()\n");
	dynarr_test();
	printf("stack_test()\n");
	stack_test();

	return 0;
}

