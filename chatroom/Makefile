all: client server

client: client.o
	gcc client.o -o client -pthread -lncurses

client.o: client.c 
	gcc -c -g client.c

server: server.o
	gcc server.o -o server -pthread

server.o: server.c server.h
	gcc -c -g server.c

clean:
	rm -rf *.o
	rm -rf client server
