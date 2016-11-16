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

Client: client.c
	gcc -Wall --pedantic client.c timestamp.c -o client -lpthread



clean:
	rm -rf *~ *.o client server

