CC=gcc
CFLAGS=-I. -g
LFLAGS=-g -lreadline -lm -ldl -lpthread
DEPS = utils.h state.h methods.h
OBJ = mql.o utils.o lexer.o methods.o stack.o element.o ops.o arith.o control.o \
 	manip.o vectors.o driver_sqlite.o sqlite3.o buffers.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

mql:  $(OBJ) 
	gcc -o $@ $^ $(CFLAGS) $(LFLAGS)
	

clean:
	rm *.o

all: mql
	
	
