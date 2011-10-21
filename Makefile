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
	main.c \

TESTSRC = \
	dynarr.c \
	dyntest.c \
	
DEPS = 
OBJ = $(SRC:.c=.o)
TESTOBJ = $(TESTSRC:.c=.o)

all: $(EXE) test

$(EXE): $(OBJ)
	$(CC) -o $(EXE) $(CFLAGS) $(OBJ)

test: $(TESTOBJ)
	$(CC) -o test $(CFLAGS) $(TESTOBJ)
	./test
	rm -f test

$(OBJ) : %.o : %.c $(DEPS)
	$(CC) -c $(CFLAGS) $< -o $@

clean :
	rm -f $(OBJ)
	rm -f $(TESTOBJ)
	rm -f $(EXE)
	rm -f test
