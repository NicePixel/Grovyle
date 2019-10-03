CC=gcc
FLAGS=-Wall
LIBS=
BIN=./grovyle

all:
	-@mkdir obj
	$(CC) $(FLAGS) -c src/main.c -o obj/main.o
	$(CC) $(FLAGS) -c src/file.c -o obj/file.o
	$(CC) obj/main.o obj/file.o -o $(BIN) $(LIBS)
