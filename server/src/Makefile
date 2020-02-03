IDIR =../include
CC=gcc
CFLAGS=-I$(IDIR)

server: server.c execution_engine.o
		$(CC) server.c -o server execution_engine.o $(CFLAGS)


client: client.c 
		$(CC) client.c -o client

execution_engine.o: execution_engine.c 
		$(CC) -c execution_engine.c $(CFLAGS)


clean :
		rm server execution_engine.o client 

           