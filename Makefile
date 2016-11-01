##############################
#          Makefile          #
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
# @author: Stefan Stockinger #
# @date:   2016-10-31        #
##############################

all :   main.c crc32.c
	gcc -Wall --pedantic main.c crc32.c

server: server.c
	gcc -Wall --pedantic server.c -o server -lpthread 

Client: client.c
	gcc -Wall --pedantic client.c -o client



clean:
	rm -rf *~ *.o client server
