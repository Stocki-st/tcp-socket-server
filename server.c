/*
 * @filename:    server.c
 * @author:      Stefan Stockinger
 * @date:        2016-10-31
 * @description: this is the server main loop
*/

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <wait.h>
#include <signal.h>

#include <semaphore.h>
#include <pthread.h>


#include "ipconf.h"

char* get_timestamp(void);
void *logger_management(void *ptr);

sem_t mutex;


int main (int argc, char **argv)
{
    int listenfd, logfile, connfd, n = 0;
    pid_t childpid;
    socklen_t clilen;
    char buf[MAXLINE];
    struct sockaddr_in cliaddr, servaddr;
    char logbuf[64];


    //pthread_t logger_thread;

//Create Logger Thread
    //pthread_create(&logger_thread,NULL,logger_management, "Logger Thread");
    //pthread_join(logger_thread, NULL);

//Create a socket for the soclet
//If sockfd<0 there was an error in the creation of the socket
    if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("Problem in creating the socket");
        exit(2);
    }

//preparation of the socket address
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port_number);

//bind the socket
    bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

//listen to the socket by creating a connection queue, then wait for clients
    listen (listenfd, LISTENQ);
    printf("%s\n","Server running...waiting for connections.");

    for( ; ; ) {
        clilen = sizeof(cliaddr);
        //accept a connection
        connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
	    if(connfd == -1)
	        perror("Connecion not acceped");
        else
           printf("%s\n","Received request...");

        childpid = fork ();
        if (childpid < 0) {
            perror("fork()");
        } else if (childpid == 0) { //if it’s 0, it’s child process
            printf ("%s\n","Child created for dealing with client requests");
            ///TODO: send message to parent with log data


            while ( (n = recv(connfd, buf, MAXLINE,0)) > 0)
            {
                printf("\nString received from and resent to the client: %s\n",buf);
                if(strncmp(buf,"~logout~",8) == 0) {
                    sprintf(buf,"~do-logout~\n");
                    send(connfd, buf, strlen(buf), 0);
                    
                    printf("Client logged out, Child proccess will termiate -> PID: %d\n",getpid());
                    fflush(stdout);
                    close(connfd);
                    close (listenfd);
                    exit(0);
                } else if(strncmp(buf,"~shutdown~",10) == 0) {
                    printf("shutdown forced");

                    sprintf(buf,"~do-logout~\n");
                    send(connfd, buf, strlen(buf), 0);
                    close(connfd);
                    printf("Server will shutdown. Clients will be forced to terminate.\n");
                    exit(0);
                } else {
                    send(connfd, buf, n, 0);
                }
                
            }
        } else {  //parent
            //TODO: logfile
printf("jetz wieder im parent");
            logfile = open("./log/serverlog", O_WRONLY | O_CREAT, S_IRWXU | S_IRGRP);
            if(logfile == -1)
                perror("unable to open logfile");
            else {
                char logmsg[] = "testasdiasdi";
                sprintf(logbuf,"%s: %s",get_timestamp(), logmsg);
                if(write(logfile,logbuf,strlen(logbuf)) == -1)
                    perror("unable to write to logfile");
		//close socket of the server
  	        close(connfd);
            }
        }
    }
}


char* get_timestamp(void)
{
    time_t now = time(NULL);
    return asctime(localtime(&now));
}

void *logger_management(void *ptr)
{
    printf("logger_management called");
}
