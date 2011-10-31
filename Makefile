EXE = myar

DEBUG = 
OPTIMIZATION = -Os
INCLUDEDIRS = 

CC = gcc
DOXYGEN = doxygen

CFLAGS = \
	$(INCLUDEDIRS) \
	-std=c99 \
	-Wall \
	-Wextra \
	-Wmissing-prototypes \
	-Wmissing-declarations \
	$(DEBUG) \
	$(OPTIMIZATION) \

SRC = \
	list.c \
	myar.c \
	main.c \

TESTSRC = \
	list.c \
	test.c \
	
DEPS = 
OBJ = $(SRC:.c=.o)
TESTOBJ = $(TESTSRC:.c=.o)

all: $(EXE) test doc

$(EXE): $(OBJ)
	$(CC) -o $(EXE) $(CFLAGS) $(OBJ)

doc:
	$(DOXYGEN) Doxyfile

test: $(TESTOBJ)
	$(CC) -o test $(CFLAGS) $(TESTOBJ)
	./test

$(OBJ) : %.o : %.c $(DEPS)
	$(CC) -c $(CFLAGS) $< -o $@

clean: cleantest cleandoc
	rm -f $(OBJ)
	rm -f $(EXE)

cleantest:
	rm -f $(TESTOBJ)
	rm -f test

cleandoc:
	rm -rf doc

