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
#include <fcntl.h>

#include "ipconf.h"
#include "timestamp.h"

#define MAXFILENAME 20 //maximum length of filename

int main(int argc, char **argv)
{
    int sockfd, n, logfile, inputfile = 0;
    extern char *optarg;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE];
    char filename[MAXFILENAME];
    char logname[MAXFILENAME];
    char logbuf[MAXLINE+20];
    char c;


    const char helpmsg[] = {"~HELP~\nTo use this client you have to add the IP and the Port as parameter.\nFor example:\n\n ./client -i 127.0.0.1 -p 7777 -f ./Client_read/read1\n\nThe Client will connect to IP 127.0.0.1 on port 7777 and uses the file 'read1' in the folder 'Client_read' as source.\n\nAll options:\n# -i ... IP address\n# -p ... Port \n# -h ... Help\n# -f ... read from file\n\nIf you do not set any IP or port, default values (ipconf.h) will be used.\nSpecial command:\n ~logout~ ... client will disconnect and terminate.\n ~shutdown~ ... server will shutdown\n\nExit"};

//check command line parameters
    while((c = getopt(argc, argv, "i:p:f:h")) != -1) {
        switch(c) {
        case 'i':
            ip_address = strdup(optarg);
            break;
        case 'p':
            port_number = atoi(optarg);
            break;
        case 'f':
            snprintf(filename, MAXFILENAME,"%s",optarg);
            inputfile = open(filename, O_RDONLY , S_IRWXU | S_IRGRP);
            if(inputfile == -1) {
                perror("input file not found\n");
                exit(0);
            }
            printf("opened: '%s'\n", filename);
            break;
        case 'h':
            perror(helpmsg);
            exit(0);
        }
    }


//open/create logfile
    sprintf(logname, "./log/clientlog_%d\n",getpid());
    logfile = open(logname, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU | S_IRGRP);
    if(logfile == -1) {
        perror("unable to open logfile");
    }

//Create a socket for the client
//If sockfd<0 there was an error in the creation of the socket
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("Problem in creating the socket");
        exit(2);
    } else {
        //sprintf(logbuf,"%s: %s\n", get_timestamp(),"Start of Client Logfile");
        sprintf(logbuf,"%s: %s\n", "42","Start of Client Logfile");
        if(write(logfile,logbuf,strlen(logbuf)) == -1)
            perror("unable to write to logfile");
    }
//Creation of the socket
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(ip_address);
    servaddr.sin_port =  htons(port_number); //convert to big-endian order

//Connection of the client to the socket
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
        perror("Unable to connect. Please check IP and Port settings.");
        exit(3);
    }
    printf("Succesfully connected to %s on port %d\n", ip_address, port_number);
    if(inputfile>0)
        dup2(inputfile, STDIN_FILENO);

    printf("Your message > ");
    while (fgets(sendline, MAXLINE-1, stdin) != NULL) {
        if(inputfile>0)
            printf("%s\n", sendline);
        send(sockfd, sendline, strlen(sendline), 0);
        n = recv(sockfd, recvline, MAXLINE-1,0);
        if (n == 0) {
            //error: server terminated prematurely
            perror("The server terminated prematurely");
            exit(4);
        } else {
            recvline[n]='\0';
            if(strncmp(recvline,"~do-logout~",11) == 0) {
                printf("client logged out successfully! Tschüssi!\n");
                close(sockfd);
                exit(0);
            }
            printf("%s %s\n","String received from the server: ", recvline);
            printf("Your message > ");
        }
    }
    exit(0);
}

/*
logfile = open("./log/clientlog", O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP);
    if(logfile == -1)
        perror("unable to open logfile");
    else {
        char logmsg[] = "testasdiasdi";
        sprintf(logbuf,"%s: %s",get_timestamp(), logmsg);
        if(write(logfile,logbuf,strlen(logbuf)) == -1)
            perror("unable to write to logfile");
    }*/
