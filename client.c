/*
 * @filename:    client.c
 * @author:      Stefan Stockinger
 * @date:        2016-10-31
 * @description: this is the client main loop
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include "ipconf.h"

int main(int argc, char **argv)
{
    int sockfd;
    extern char *optarg;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE];
    char c;
    const char helpmsg[] = {"~HELP~\nTo use this client you have to add the IP and the Port as parameter.\nFor example:\n\n ./client -i 127.0.0.1 -p 7777\n\nAll options:\n# -i ... IP address\n# -p ... Port \n# -h ... Help\n\nIf you do not set any IP or port, default values (ipconf.h) will be used.\nSpecial command:\n ~logout~ ... client will disconnect and terminate.\n ~shutdown~ ... server will shutdown\n\nExit"};

//check command line parameters
    while((c = getopt(argc, argv, "i:p:h")) != -1)
    {
        switch(c)
        {
        case 'i':
            ip_address = strdup(optarg);
            break;
        case 'p':
            port_number = atoi(optarg);
            break;
        case 'h':
            perror(helpmsg);
            exit(0);
        }
    }

//Create a socket for the client
//If sockfd<0 there was an error in the creation of the socket
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0)
    {
        perror("Problem in creating the socket");
        exit(2);
    }

//Creation of the socket
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(ip_address);
    servaddr.sin_port =  htons(port_number); //convert to big-endian order

//Connection of the client to the socket
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0)
    {
        perror("Unable to connect. Please check IP and Port settings.");
        exit(3);
    }
    printf("Succesfully connected to %s on port %d\n", ip_address, port_number);


    while (fgets(sendline, MAXLINE, stdin) != NULL)
    {
        send(sockfd, sendline, strlen(sendline), 0);
        if (recv(sockfd, recvline, MAXLINE,0) == 0)
        {
            //error: server terminated prematurely
            perror("The server terminated prematurely");
            exit(4);
        }
        if(strncmp(recvline,"~do-logout~",11) == 0) {
            printf("client logged out successfully! Tschüssi!\n");
            close(sockfd);
            exit(0);
        }
        printf("\nString received from the server: %s\n", recvline);
    }
    exit(0);
}
