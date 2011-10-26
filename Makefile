EXE = myar

DEBUG = 
OPTIMIZATION = -Os
INCLUDEDIRS = 

CC = gcc

CFLAGS = \
	$(INCLUDEDIRS) \
	-Wall \
	-Wextra \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	$(DEBUG) \
	$(OPTIMIZATION) \

SRC = \
	dynarr.c \
	stack.c \
	main.c \

TESTSRC = \
	list.c \
	test.c \
	dynarr.c \
	dyntest.c \
	stack.c \
	
DEPS = 
OBJ = $(SRC:.c=.o)
TESTOBJ = $(TESTSRC:.c=.o)

all: $(EXE) test

$(EXE): $(OBJ)
	$(CC) -o $(EXE) $(CFLAGS) $(OBJ)

test: $(TESTOBJ)
	$(CC) -o test $(CFLAGS) $(TESTOBJ)
	./test
	make cleantest

$(OBJ) : %.o : %.c $(DEPS)
	$(CC) -c $(CFLAGS) $< -o $@

clean : cleantest
	rm -f $(OBJ)
	rm -f $(EXE)

cleantest :
	rm -f $(TESTOBJ)
	rm -f test

