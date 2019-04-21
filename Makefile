all: ClientServer

ClientServer: ClientServer.o
        gcc -g -o ClientServer  ClientServer.o 


ClientServer.o: ClientServer.c
        gcc -g -c ClientServer.c
