CC       = gcc
CFLAGS   = -Wall -O2 -pthread
LDFLAGS  = -pthread

SRV_OBJS = main.o server.o client_handler.o logger.o config.o
SRV_BIN  = server
LIBS     = -lcrypto

CLI_OBJS = test_client.o
CLI_BIN  = test_client

.PHONY: all clean run test

all: $(SRV_BIN) $(CLI_BIN)

$(SRV_BIN): $(SRV_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

main.o: main.c server.h logger.h
	$(CC) $(CFLAGS) -c $<

server.o: server.c server.h client_handler.h logger.h
	$(CC) $(CFLAGS) -c $<

client_handler.o: client_handler.c client_handler.h logger.h server.h
	$(CC) $(CFLAGS) -c $<

logger.o: logger.c logger.h
	$(CC) $(CFLAGS) -c $<

$(CLI_BIN): $(CLI_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_client.o: test_client.c
	$(CC) $(CFLAGS) -c $<

run: $(SRV_BIN)
	./$(SRV_BIN) 5050 log.txt

test: $(CLI_BIN)
	./$(CLI_BIN) 127.0.0.1 5050 ADMIN

clean:
	rm -f *.o $(SRV_BIN) $(CLI_BIN)