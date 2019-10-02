CC=gcc
FLAGS=-Wall
LIBS=
BIN=./grovyle

all:
	-@mkdir obj
	$(CC) $(FLAGS) -c src/main.c -o obj/main.o
	$(CC) obj/main.o -o $(BIN) $(LIBS)
