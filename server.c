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

#define DEBUG 1

#define _POSIX_C_SOURCE     200112L

/*
#define MQLOGKEY 0x77777777
#define MQTYPE 1

struct msg {
  long type;
  char msg[20];
};

*/

void cntrl_c_handler(int ignored);
void hash_cracker(uint32_t orig_crc, char conflict[5]);

void *logger_thread_func(void *ptr);
void *server_thread_func(void *ptr);
//sem_t mutex;



volatile sig_atomic_t quit = 0;


/** @brief server main function
 *
 */
int main (int argc, char **argv)
{
    int listenfd, connfd, n, connections = 0;
    pid_t childpid;
    socklen_t clilen;
    char buf[MAXLINE];
    struct sockaddr_in cliaddr, servaddr;
    char logbuf[64];

// catch cntrl_c signal
    signal(SIGINT, cntrl_c_handler);

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
//If listenfd<0 there was an error in the creation of the socket
    if ((listenfd = socket (AF_INET, SOCK_STREAM, 0)) <0) {
        perror("Problem in creating the socket");
        exit(EXIT_FAILURE);
    }

//preparation of the socket address
    servaddr.sin_family = AF_UNSPEC;
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
                uint32_t crc = 0;
                char new_hash[5];
                while ((n = recv(connfd, buf, MAXLINE-1,0)) > 0)
                {

                    buf[n] = '\0';
                    printf("String received from and resent to the client: %s", buf);
                    if(strncmp(buf,"~logout~",8) == 0) {
                        sprintf(buf,"~do-logout~\n");
                        send(connfd, buf, strlen(buf), 0);
                        printf("Client logged out, Child proccess will terminate -> PID: %d\n",getpid());
                        close(connfd);
                        close (listenfd);
                        connections--;
                        exit(0);
                    } else if((strncmp(buf,"~shutdown~",10) == 0) ||(quit ==1) ) {
                        printf("ATTENTION: shutdown forced\n");
                        sprintf(buf,"~do-logout~\n");
                        send(connfd, buf, strlen(buf), 0);
                        close(connfd);
                        printf("Server will shutdown. Clients will be forced to terminate.\n");
                        exit(0);
                    } else {
                        crc = crc32(buf, strlen(buf));
#ifdef DEBUG
                        new_hash[0] = 'T';
                        new_hash[1] = 'E';
                        new_hash[2] = 'S';
                        new_hash[3] = 'T';
                        new_hash[4] = '\0';
#else
                        hash_cracker(crc, new_hash);
#endif
                        printf("conflict hash = '%s'\n\n", new_hash);
                        sprintf(buf,"conflict hash = '%s'\n",new_hash);
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


/** @brief catches control + c signal
 *
 */
void cntrl_c_handler(int ignored) {

    printf("\nexit with strg c \n");
    quit = 1;
    exit(0);
}


/** @brief calculates a hash conflict
 *
 */
void hash_cracker(uint32_t orig_crc,  char conflict[5])
{
    //char result[1024];
    uint32_t cur_crc;
    uint32_t i = 0;

    /* search for equal hash code */
    while (1) {
        uint8_t in[] = {(i>>24), (i>>16), (i>>8), i};
        cur_crc = crc32(in, sizeof(in));

        if (cur_crc == orig_crc) {
            conflict[0] = in[0];
            conflict[1] = in[1];
            conflict[2] = in[2];
            conflict[3] = in[3];
            conflict[4] = '\0';
            break;
        }
        i++;
    }
}
