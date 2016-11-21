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
#include <time.h>
#include <signal.h>

#include "ipconf.h"
//#include "timestamp.h"
#include "crc32.h"

#define _POSIX_C_SOURCE     200112L

#define MAXFILENAME 20 //maximum length of filename

char* get_timestamp(void);
void print_help(void);
void write_logfile(int fd, char* msg);
void cntrl_c_handler(int ignored);

volatile sig_atomic_t quit = 0;


/** @brief client main function
 *
 */
int main(int argc, char **argv)
{
    int sockfd, n, logfile, inputfile = 0;
    extern char *optarg;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE];
    char filename[MAXFILENAME];
    char logname[MAXFILENAME];
    char logmsg[MAXLINE];
    char c;
    uint32_t crc;

//Creation of the socket
    memset(&servaddr, 0, sizeof(servaddr));

//open/create logfile
    sprintf(logname, "./log/clientlog_%d\n",getpid());
    logfile = open(logname, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU | S_IRGRP);
    if(logfile == -1) {
        perror("unable to open logfile");
    }
    write_logfile(logfile, "Client started");

// catch cntrl_c signal
    signal(SIGINT, cntrl_c_handler);

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
            sprintf(logmsg,"opened: '%s' as input\n", filename);
            printf("%s", logmsg);
            write_logfile(logfile, logmsg);
            break;
        case 'h':
            print_help();
            exit(0);
        }
    }




//Create a socket for the client
//If sockfd<0 there was an error in the creation of the socket
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        sprintf(logmsg, "Problem in creating the socket\n");
        perror(logmsg);
        write_logfile(logfile,logmsg);
        exit(2);
    }
//Creation of the socket
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(ip_address);
    servaddr.sin_port =  htons(port_number); //convert to big-endian order

//Connection of the client to the socket
    if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0) {
        sprintf(logmsg, "Unable to connect. Please check IP and Port settings.");
        perror(logmsg);
        write_logfile(logfile,logmsg);
        exit(3);
    }
    sprintf(logmsg,"Succesfully connected to %s on port %d\n", ip_address, port_number);
    printf("%s", logmsg);
    write_logfile(logfile, logmsg);

    //redirect file stdinput to file
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
            write_logfile(logfile, logmsg);
            quit = 0;
        } else {
            //check for special commands (must not be hashed)
            if(!(strncmp(sendline,"~logout~",8) == 0) || (strncmp(sendline,"~shutdown~",10) == 0)) {
                crc = 17;//crc32(sendline, strlen(sendline));
                sprintf(logmsg,"Hash of %s is '%u'\n", sendline, crc);
                write_logfile(logfile, logmsg);
                sprintf(sendline,"%u\n",crc);
                printf("calculated hash: %s\n", sendline);
            }
        }
        send(sockfd, sendline, strlen(sendline), 0);
        n = recv(sockfd, recvline, MAXLINE-1,0);
        if (n == 0) {
            sprintf(logmsg,"The server terminated prematurely");
            perror(logmsg);
            write_logfile(logfile, logmsg);
            exit(4);
        } else {
            recvline[n]='\0';
            if(strncmp(recvline,"~do-logout~",11) == 0) {
                sprintf(logmsg,"client logged out successfully! Tschüssi!\n");
                printf("%s",logmsg);
                write_logfile(logfile, logmsg);
                close(sockfd);  
                close(logfile);
                if(inputfile>0)
                    close(inputfile);
                exit(0);
            }
            sprintf(logmsg,"String received from the server: %s\n", recvline);
            write_logfile(logfile, logmsg);
            printf("%s",logmsg);
            printf("Your message > ");
        }
    }
    exit(0);
}

/** @brief writes messages to the logfile
 *
 */
void write_logfile(int fd, char* msg) 
{
    char logbuf[MAXLINE+20];
    sprintf(logbuf,"%s: %s\n",get_timestamp(), msg);
    if(write(fd,logbuf,strlen(logbuf)) == -1)
        perror("unable to write to logfile");
}

/** @brief catches the control + c signal
 *
 */
void cntrl_c_handler(int ignored) 
{
    quit = 1;
    printf("\n\n\n ##############################\n Press any key to leave the app\n ##############################\n");
}

/** @brief generates a timestamp
 *
 */
char* get_timestamp(void) 
{
    time_t now = time(NULL);
    return asctime(localtime(&now));
}

/** @brief prints the help message to stdout
 *
 */
void print_help(void) 
{
    const char helpmsg[] = {"~HELP~\nTo use this client you have to add the IP and the Port as parameter.\nFor example:\n\n ./client -i 127.0.0.1 -p 7777 -f ./Client_read/read1\n\nThe Client will connect to IP 127.0.0.1 on port 7777 and uses the file 'read1' in the folder 'Client_read' as source.\n\nAll options:\n# -i ... IP address\n# -p ... Port \n# -h ... Help\n# -f ... read from file\n\nIf you do not set any IP or port, default values (ipconf.h) will be used.\nSpecial command:\n ~logout~ ... client will disconnect and terminate.\n ~shutdown~ ... server will shutdown\n\nExit"};
    printf("%s\n", helpmsg);
}

//EOF
