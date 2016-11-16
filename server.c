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
#include <wait.h>
#include <signal.h>
#include <sys/msg.h>

#include <semaphore.h>
#include <pthread.h>


#include "ipconf.h"
#include "crc32.h"
#include "timestamp.h"

/*
#define MQLOGKEY 0x77777777
#define MQTYPE 1

struct msg {
  long type;
  char msg[20];
};

*/


void *logger_thread_func(void *ptr);
void *server_thread_func(void *ptr);
//sem_t mutex;


int main (int argc, char **argv)
{
    int listenfd, connfd, n, connections = 0;
    pid_t childpid;
    socklen_t clilen;
    char buf[MAXLINE];
    struct sockaddr_in cliaddr, servaddr;
    char logbuf[64];
    /*
    struct msg data;
    data.type = MQTYPE;
    long int msqid = msgget(MQKEY, IPC_CREAT | S_IRWXU | S_IROTH);s
    */

    /*
        pthread_t logger_thread, server_thread;

    //Create Logger Thread
        pthread_create(&logger_thread,NULL,&logger_thread_func, NULL);
        pthread_create(&server_thread,NULL,&server_thread_func, NULL);
        pthread_join(logger_thread, NULL);
        pthread_join(server_thread, NULL);*/

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

        connections++;
        if(connections > LISTENQ) {
///TODO: send error to client and block connection
            //  printf ("%s\n","connection blocked - reached maximum number of connections");
            //  connections--;
        } else {
            childpid = fork ();
            if (childpid < 0) {
                perror("fork()");
            } else if (childpid == 0) { //if it’s 0, it’s child process
                printf ("%s\n","Child created for dealing with client requests");
                ///TODO: send message to parent with log data

                while ((n = recv(connfd, buf, MAXLINE-1,0)) > 0)
                {

                    buf[n] = '\0';
                    printf("%s %s\n","String received from and resent to the client:", buf);
                    fflush(stdout);

                    if(strncmp(buf,"~logout~",8) == 0) {
                        sprintf(buf,"~do-logout~\n");
                        send(connfd, buf, strlen(buf), 0);
                        printf("Client logged out, Child proccess will terminate -> PID: %d\n",getpid());
                        fflush(stdout);
                        close(connfd);
                        close (listenfd);
                        connections--;
                        exit(0);
                    } else if(strncmp(buf,"~shutdown~",10) == 0) {
                        printf("ATTENTION: shutdown forced\n");
                        sprintf(buf,"~do-logout~\n");
                        send(connfd, buf, strlen(buf), 0);
                        close(connfd);
                        printf("Server will shutdown. Clients will be forced to terminate.\n");
                        exit(0);
                    } else {
                        uint32_t crc = crc32(buf, strlen(buf));
                        sprintf(buf,"%u\n",crc);
                        send(connfd, buf, strlen(buf), 0);
                    }
                }
            } else {  //parent
                //TODO: logfile
                //close socket of the server
                close(connfd);
            }
        }
    }
}



void *logger_thread_func(void *ptr)
{
    int logfile;
    char logbuf[] = "pt";
    logfile = open("./log/serverlog", O_WRONLY | O_APPEND | O_CREAT, S_IRWXU | S_IRGRP);
    if(logfile == -1)
        perror("unable to open logfile");
    else {
        char logmsg[] = "testasdiasdi";
        sprintf(logbuf,"%s: %s",get_timestamp(), logmsg);
        if(write(logfile,logbuf,strlen(logbuf)) == -1)
            perror("unable to write to logfile");
    }
    printf("logger_management called");
    return 0;
}

void *server_thread_func(void *ptr)
{
    return 0;
}

