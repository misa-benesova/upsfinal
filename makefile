CC = gcc
CFLAGS = -Wall -Wextra -pedantic -ansi -lpthread -g

BIN = server.exe
OBJ = server.o 

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(CFLAGS)

main.o: server.c
	gcc -c server.c $(CFLAGS)

clean:
	del -f $(BIN) $(OBJ)
