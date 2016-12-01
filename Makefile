##############################
#          Makefile          #
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~#
# @author: Stefan Stockinger #
# @date:   2016-10-31        #
##############################

all :  
	rm -rf *~ *.o client server
	gcc -Wall --pedantic server.c crc32.c timestamp.c logfile.c -o server -lpthread 
	gcc -Wall --pedantic client.c crc32.c timestamp.c logfile.c -o client -lpthread 

server: server.c
	gcc -Wall --pedantic server.c crc32.c timestamp.c logfile.c -o server -lpthread 

client: client.c
	gcc -Wall --pedantic client.c crc32.c timestamp.c logfile.c -o client -lpthread 

clean:
	rm -rf *~ *.o client server

