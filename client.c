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
#include<netdb.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include "ipconf.h"
#include "logfile.h"
#include "crc32.h"

#define _GNU_SOURCE

#define MAXFILENAME 25 //maximum length of filename

void print_help(void);
void cntrl_c_handler(int ignored);
int hostname_to_ip(char *hostname , char *ip);

volatile sig_atomic_t quit = 0;

enum {DEFAULT=0, IPv4, IPv6} eIPversion;

/** @brief client main function
 *
 */
int main(int argc, char **argv)
{
    int sockfd, n, inputfile = 0;
    extern char *optarg;
    struct sockaddr_in servaddr;
    struct sockaddr_in6 servaddr6;
    int ipflag = DEFAULT;
    char sendline[MAXLINE], recvline[MAXLINE];
    char filename[MAXFILENAME];
    char logname[MAXFILENAME];
    char logmsg[MAXLINE];
    char c;
    uint32_t crc;

//Creation of the socket
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&servaddr6, 0, sizeof(servaddr6));

//open/create logfile
    sprintf(logname, "./log/clientlog_%d\n",getpid());
    log_message(logname, "Client started\n");

//check command line parameters
    while((c = getopt(argc, argv, "i:p:f:h")) != -1) {
        switch(c) {
        case 'i':
            if(inet_pton(AF_INET, optarg, &servaddr.sin_addr)) {
                ipflag = IPv4;
            } else if(inet_pton(AF_INET6 , optarg, &servaddr6.sin6_addr)) {
                ipflag = IPv6;
                servaddr.sin_family = AF_INET6;
            } else {
                if(hostname_to_ip(strdup(optarg), ip_address)) {
                    sprintf(logmsg,"unable to resolve hostname - please check your input!");
                    perror(logmsg);
                    log_message(logname, logmsg);
                    exit(EXIT_FAILURE);
                }
            }
            break;
        case 'p':
            port_number = atoi(optarg);
            break;
        case 'f':
            snprintf(filename, MAXFILENAME,"%s",optarg);
            inputfile = open(filename, O_RDONLY , S_IRWXU | S_IRGRP);
            if(inputfile == -1) {
                perror("input file not found\n");
                exit(EXIT_FAILURE);
            }
            sprintf(logmsg,"opened: '%s' as input\n", filename);
            printf("%s", logmsg);
            log_message(logname, logmsg);
            break;
        case 'h':
            print_help();
            exit(EXIT_SUCCESS);
        }
    }

//try to establish connection to desired ip/port using the choosen ip type
    switch(ipflag) {
    case DEFAULT:
        servaddr.sin_addr.s_addr= inet_addr(ip_address);
    case IPv4:
        servaddr.sin_family = AF_INET;
        servaddr.sin_port =  htons(port_number);
//convert to big-endian order
//Create a socket for the client
//If sockfd<0 there was an error in the creation of the socket
        if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
            sprintf(logmsg, "Problem in creating the socket\n");
            perror(logmsg);
            log_message(logname,logmsg);
            exit(EXIT_FAILURE);
        }
        printf("IPv4 socket created... Try to connect to server...\n");

//Connection of the client to the socket
        if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
            sprintf(logmsg, "Unable to connect. Please check IP and Port settings.\nExit ");
            perror(logmsg);
            log_message(logname,logmsg);
            exit(EXIT_FAILURE);
        }
        sprintf(logmsg,"Succesfully connected to %s on port %d\n", ip_address, port_number);
        printf("%s", logmsg);
        log_message(logname, logmsg);
        break;
    case IPv6:
        servaddr6.sin6_family = AF_INET6;
        servaddr6.sin6_port =  htons(port_number);
//convert to big-endian order
//Create a socket for the client
//If sockfd<0 there was an error in the creation of the socket
        if ((sockfd = socket (AF_INET6, SOCK_STREAM, 0)) <0) {
            sprintf(logmsg, "Problem in creating the socket\n");
            perror(logmsg);
            log_message(logname,logmsg);
            exit(EXIT_FAILURE);
        }
        printf("IPv6 socket created... Try to connect to server...\n");

//Connection of the client to the socket
        if (connect(sockfd, (struct sockaddr *) &servaddr6, sizeof(servaddr6))<0) {
            sprintf(logmsg, "Unable to connect. Please check IP and Port settings.\nExit ");
            perror(logmsg);
            log_message(logname,logmsg);
            exit(EXIT_FAILURE);
        }
        sprintf(logmsg,"Succesfully connected to server\n");
        printf("%s", logmsg);
        log_message(logname, logmsg);
        break;
    default:
        sprintf(logmsg, "Unable to connect. Please check IP and Port settings.\nExit ");
        perror(logmsg);
        log_message(logname,logmsg);
        exit(EXIT_FAILURE);
    }

//catch strg+c command as soon as connection is established
    signal(SIGINT, cntrl_c_handler);

    //redirect file stdinput to file if there was a file choosen
    if(inputfile>0)
        dup2(inputfile, STDIN_FILENO);

    printf("Your message > ");
    while (fgets(sendline, MAXLINE-1, stdin) != NULL) {
        if(inputfile>0)
            printf("%s\n", sendline);

        //check for strg+c signal
        if(quit == 1) {
            sprintf(sendline,"~logout~\n");
            sprintf(logmsg,"~logout~: user input STRG+C\n");
            log_message(logname, logmsg);
            quit = 0;
        } else {
            //check for special commands (must not be hashed)
            if(!(strncmp(sendline,"~logout~",8) == 0) ) {
                crc = crc32(sendline, strlen(sendline));
                sendline[strlen(sendline)-1] = '\0';
                sprintf(logmsg,"Hash of '%s' is '%u'\n", sendline, crc);
                log_message(logname, logmsg);
                sprintf(sendline,"%u\n",crc);
                printf("calculated hash: %s\n . . . Please wait for server . . . \n", sendline);
            }
        }
        send(sockfd, sendline, strlen(sendline), 0);
        n = recv(sockfd, recvline, MAXLINE-1,0);
        if (n == 0) {
            sprintf(logmsg,"The server terminated prematurely");
            perror(logmsg);
            log_message(logname, logmsg);
            exit(EXIT_FAILURE);
        } else {
            recvline[n]='\0';
            if(strncmp(recvline,"~do-logout~",11) == 0) {
                sprintf(logmsg,"client logged out successfully! Tschüssi!\n");
                printf("%s",logmsg);
                log_message(logname, logmsg);
                close(sockfd);
                if(inputfile>0)
                    close(inputfile);
                exit(EXIT_SUCCESS);
            }
            sprintf(logmsg,"String received from the server: %s\n", recvline);
            log_message(logname, logmsg);
            printf("%s",logmsg);
            printf("Your message > ");
        }
    }
    exit(EXIT_SUCCESS);
}



/** @brief catches the control + c signal
 *
 *  @param ignored   number of ignored signal
 */
void cntrl_c_handler(int ignored)
{
    quit = 1;
    printf("\n\n\n ###############################\n Press 'RETURN' to leave the app\n ###############################\n");
}



/** @brief get ip from domain name
 *   source: https://cis.technikum-wien.at/documents/mes/1/sec/semesterplan/em2/vo-03-sockets.handout.pdf
*
 * @param	hostname	pointer to the hostname
 * 			ip          pointer to the ip adress
 *
 * @return 	0 on success, -1 if there was a problem
 */
int hostname_to_ip(char *hostname , char *ip)
{
    struct addrinfo * servinfo ;
    int ret = getaddrinfo (hostname, NULL , NULL , &servinfo );
    if ( ret != 0) {
        return -1;
    }

// loop through all the results and connect to the first we can
    struct addrinfo *p;
    //char ipstr [8*4 + 7 + 1];
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if (p->ai_family == AF_INET) {
            struct sockaddr_in *s = (struct sockaddr_in *)p->ai_addr;
            inet_ntop(AF_INET, &s->sin_addr, ip, sizeof(ip));
        } else { // AF_INET6
            struct sockaddr_in6 *s = (struct sockaddr_in6 *)p->ai_addr ;
            inet_ntop(AF_INET6, &s->sin6_addr, ip, sizeof(ip));
        }
    }
    freeaddrinfo(servinfo);
    return 0;
}



/** @brief prints the help message to stdout
 *
 */
void print_help(void)
{
    const char helpmsg[] = {"~HELP~\nTo use this client you have to add the IP and the Port as parameter.\nFor example:\n\n ./client -i 127.0.0.1 -p 7777 -f ./Client_read/read1\n\nThe Client will connect to IP 127.0.0.1 on port 7777 and uses the file 'read1' in the folder 'Client_read' as source.\n\nAll options:\n# -i ... IP address\n# -p ... Port \n# -h ... Help\n# -f ... read from file\n\nIf you do not set any IP or port, default values (ipconf.h) will be used.\nSpecial command:\n ~logout~ ... client will disconnect and terminate.\n\nExit"};
    printf("%s\n", helpmsg);
}

//EOF
