CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lm
EXEC = allocate
OBJ = queue.o processQueue.o allocate.o utils.o

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ) $(LDFLAGS)

%.o: %.c %.h
	$(CC) -c -o $@ $< $(CFLAGS)

clean:
	rm -rf *.o $(EXEC)