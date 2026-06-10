DEPENDENCIES = -lraylib -lm -lpthread -lrt -lX11
CFLAGS = -g

pong: main.o game.o
	cc -o pong build/main.o build/game.o $(DEPENDENCIES)

main.o: src/main.c
	cc $(CFLAGS) -c src/main.c -o build/main.o

game.o: src/game.c
	cc $(CFLAGS) -c src/game.c -o build/game.o

clean:
	rm pong build/main.o build/game.o
