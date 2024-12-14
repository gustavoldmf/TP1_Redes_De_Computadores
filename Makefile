all:
	gcc -Wall -c common.c
	gcc -Wall client.c common.o -o bin/client
	gcc -Wall server.c common.o -o bin/server
	gcc -Wall server-mt.c common.o -lpthread -o server-mt

clean:
	rm common.o client server server-mt 