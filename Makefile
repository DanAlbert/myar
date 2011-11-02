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
	myar.c \
	main.c \
	
DEPS = 
OBJ = $(SRC:.c=.o)

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) -o $(EXE) $(CFLAGS) $(OBJ)

doc:
	$(DOXYGEN) Doxyfile

$(OBJ) : %.o : %.c $(DEPS)
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJ)
	rm -f $(EXE)

cleandoc:
	rm -rf doc
