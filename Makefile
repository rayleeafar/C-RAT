.PHONY: clean

CC = gcc
CFLAGS = -Wall  -W 
SRC = $(wildcard *.c)
OBJS = $(SRC:.c=.o)


client: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ 

$(OBJS): $(SRC)
	$(CC) $(CFLAGS) -c $^

makefile.dep: $(SRC)
	gcc -MM $^ > $@

clean:
	rm -f *.o client 

