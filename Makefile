CC=gcc
CFLAGS:= -g
LDFLAGS:=-lpthread -pthread

SRC := client.c server.c
OBJ := $(patsubst %.c,%.o,${SRC})

all:client server

client: client.o debug.o test_client.o
	$(CC) $(LDFLAGS) -o $@ $^

server: server.o debug.o
	$(CC) $(LDFLAGS) -o $@ $^

depend:
	${CC} -MM ${SRC} > depend


#%.o:%.c
#	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^ 

.PHONY: clean
clean:
	rm *.o ${OBJ} depend client server -rf

