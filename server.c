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
#include "logfile.h"

void cntrl_c_handler(int ignored);
void hash_cracker(uint32_t orig_crc, uint8_t conflict[5]);
void *get_in_addr(struct sockaddr *sa);


void SIGCHLD_handler(int) ;
void install_SIGCHLD_handler(void) ;

void *hashi_cracker(void *ptr);

typedef struct thread_job_s {
    char data[1024];
    int len;
    int socket_fd;
} thread_job_t;


volatile sig_atomic_t quit = 0;
int parentid = 0;

/** @brief server main function
 *
 */
int main (int argc, char **argv)
{
    int listenfd, connfd, n, connections = 0;
    pid_t childpid;
    socklen_t clilen;
    char buf[MAXLINE];
    struct sockaddr_in6 cliaddr, servaddr;
    char logbuf[MAXLINE];
    int reuseaddr = 1;
    pthread_t worker_thread[2];
    thread_job_t worker_data;

// catch cntrl_c signal
    signal(SIGINT, cntrl_c_handler);
    install_SIGCHLD_handler() ;

//Create a socket for the soclet
//If listenfd<0 there was an error in the creation of the socket
    if ((listenfd = socket (AF_INET6, SOCK_STREAM, 0)) <0) {
        perror("Problem in creating the socket");
        exit(EXIT_FAILURE);
    }

    // setsockopt() free previously used sockets()
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(reuseaddr)) == -1)
        perror("setsockopt error");


//preparation of the socket address
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_addr = in6addr_any;
    servaddr.sin6_port = htons(port_number);

//bind the socket
    bind (listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

//listen to the socket by creating a connection queue, then wait for clients
    if (listen(listenfd, LISTENQ) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("%s\n","Server running...waiting for connections.");
    log_message("./log/serverlog","Server started - waiting for connections\n");
    while(1) {
        if(quit==0) {
            clilen = sizeof(cliaddr);
            //accept a connection
            connfd = accept (listenfd, (struct sockaddr *) &cliaddr, &clilen);
            if(connfd == -1) {
                perror("Connecion not acceped");
                exit(0);
            }
            printf("%s\n","Received request...");
            log_message("./log/serverlog","Received request...\n");

            childpid = fork ();
        }
        if (childpid < 0) {
            perror("fork()");
            exit(0);
        } else if (childpid == 0) { //if it’s 0, it’s child process
            close(listenfd) ; // close listen port

            printf ("%s\n","Child created for dealing with client requests");
            log_message("./log/serverlog","Child created for dealing with client requests\n");
            uint32_t crc = 0;
            uint8_t new_hash[5];
            while (((n = recv(connfd, buf, MAXLINE-1,0)) > 0) && (quit == 0)) {
                printf("QUIT = %d",quit);
                buf[n-1] = '\0';
                sprintf(logbuf,"String received from client: %s\n", buf);
                log_message("./log/serverlog",logbuf);
                printf("%s", logbuf);
                if(strncmp(buf,"~logout~",8) == 0) {
                    sprintf(buf,"~do-logout~\n");
                    send(connfd, buf, strlen(buf), 0);
                    sprintf(logbuf,"Client logged out, Child proccess will terminate -> PID: %d\n",getpid());
                    log_message("./log/serverlog",logbuf);
                    printf("%s",logbuf);
                    close (listenfd);
                    exit(0);
                } else {
                    crc = crc32(buf, strlen(buf));
                    new_hash[0] = 'T';
                    new_hash[1] = 'E';
                    new_hash[2] = 'S';
                    new_hash[3] = 'T';
                    new_hash[4] = '\0';
                    // hash_cracker(crc, new_hash);
                    sprintf(logbuf,"conflict hash  of '%s' is '%s'\n",buf, new_hash);
                    log_message("./log/serverlog",logbuf);
                    printf("%s",logbuf);
                    sprintf(buf,"conflict hash = '%s'\n",new_hash);
                    send(connfd, buf, strlen(buf), 0);
                }
            }
            sprintf(buf,"~do-logout~\n");
            send(connfd, buf, strlen(buf), 0);
            close(connfd);
            sprintf(logbuf,"Client forced to log out, Child proccess will terminate -> PID: %d\n",getpid());
            log_message("./log/serverlog",logbuf);
            printf("%s",logbuf);
            exit(0);
        } else {  //parent
            //close socket of the server
            printf("\nIN PARENT!!! %d\n", getpid());
            close(connfd);
            if(quit ==1)
                exit(EXIT_SUCCESS);
        }
    }
    exit(0);
}



/** @brief catches control + c signal
 *
 */
void cntrl_c_handler(int ignored) {
    printf("ATTENTION: shutdown forced\nServer will shutdown. Clients will be forced to terminate when they send their next message.\n");

    quit = 1;
    printf("CNTRL-Handler -> PID: %d\n", getpid());
//	exit(0);
}


/** @brief calculates a hash conflict
 *
 */
void hash_cracker(uint32_t orig_crc,  uint8_t conflict[5])
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
    printf("%d %d %d %d\n", conflict[0], conflict[1], conflict[2], conflict[3]);
    printf("%c %c %c %c\n", conflict[0], conflict[1], conflict[2], conflict[3]);
}


void *hashi_cracker(void *ptr)
{
    /*
      thread_job_t *job = (thread_job_t *)ptr;
      uint32_t orig_crc = crc32(job->data, job->len);
      char result[1024];
      uint32_t cur_crc;
      uint32_t i = 0;

      // search for equal hash code
     while (1) {
        uint8_t in[] = {(i>>24), (i>>16), (i>>8), i};
        cur_crc = crc32(in, sizeof(in));

        if (cur_crc == orig_crc) {
          break;
        }
        i++;
      }

      // format result in string
      sprintf(result, "0x%08"PRIx32"\r\n", i);

      // send result to client
      if(send(job->socket_fd, result, strlen(result),
              0) != strlen(result) ) {
        perror("send");
      }

      */


    printf("IM AN A THREAD!!!\n");
    return NULL;
}

// SIGCHLD handler, derived from W. Richard Stevens,
// Network Programming, Vol.1, 2nd Edition, p128
void SIGCHLD_handler(int signo)
{
    pid_t  pid ;
    int  stat ;

    while ( (pid=waitpid(-1,&stat,WNOHANG)) > 0 )
        ;
    // optional actions, usually nothing ;
    return ;
}

// installer for the SIGCHLD handler
void install_SIGCHLD_handler(void)
{
    struct sigaction act ;
// block all signals during exec of SIGCHLD_handler
    sigfillset(&act.sa_mask) ;
    act.sa_handler = &SIGCHLD_handler ;
// auto restart interrupted system calls
    act.sa_flags = SA_RESTART ;
    sigaction (SIGCHLD,&act,NULL) ;
}
