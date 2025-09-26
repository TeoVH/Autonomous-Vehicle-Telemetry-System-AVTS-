# Makefile para servidor de telemetría
CC = gcc
CFLAGS = -Wall -pthread
OBJ = main.o server.o client_handler.o logger.o
TARGET = server

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) -lssl -lcrypto

main.o: main.c server.h logger.h
	$(CC) $(CFLAGS) -c main.c

server.o: server.c server.h client_handler.h logger.h
	$(CC) $(CFLAGS) -c server.c

client_handler.o: client_handler.c client_handler.h logger.h server.h
	$(CC) $(CFLAGS) -c client_handler.c

logger.o: logger.c logger.h
	$(CC) $(CFLAGS) -c logger.c

clean:
	rm -f *.o $(TARGET)

