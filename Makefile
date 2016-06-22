CC=gcc
CFLAGS=-I. -g
LFLAGS=-lreadline -lm
DEPS = utils.h state.h methods.h
OBJ = mql.o utils.o lexer.o methods.o stack.o element.o ops.o arith.o control.o vectors.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

mql: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LFLAGS)
	
	
