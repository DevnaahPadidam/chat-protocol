CC = gcc
CFLAGS = -I./common -I./common/quiche/include -I$(HOME)/local/include -g
LDFLAGS = -L./common/quiche/target/release -L$(HOME)/local/lib -lquiche -lm -lpthread -ljson-c

CLIENT_SRC = client/client.c common/message.c common/states.c
SERVER_SRC = server/server.c common/message.c common/states.c

CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

all: server client

server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o server/server $(SERVER_OBJ) $(LDFLAGS)

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o client/client $(CLIENT_OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(CLIENT_OBJ) $(SERVER_OBJ) server/server client/client
