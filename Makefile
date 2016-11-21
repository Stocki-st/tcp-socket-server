##############################
#          Makefile          #
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
# @author: Stefan Stockinger #
# @date:   2016-10-31        #
##############################

all :   main.c crc32.c
	gcc -Wall --pedantic main.c crc32.c

server: server.c
	gcc -Wall --pedantic server.c crc32.c timestamp.c -o server -lpthread 

Client: client.c crc32.c crc32.h timestamp.c
	gcc -Wall --pedantic -I . -c timestamp.c
	gcc -Wall --pedantic -I . -c crc32.c
	gcc -Wall --pedantic -I . -c client.c
	gcc -Wall --pedantic -o client client.o crc32.o timestamp.o

clean:
	rm -rf *~ *.o client server

