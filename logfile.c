/*
 * @filename:    timestamp.c
 * @author:      Stefan Stockinger
 * @date:        2016-11-16
 * @description: this file contains the logfile function
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#include "timestamp.h"

#define MAXLINE 4096

void log_message(char *filename, char* msg)
{
    char logbuf[MAXLINE+20];
    char timestamp[20];

    int logfile = open(filename, O_WRONLY | O_APPEND | O_CREAT, S_IRWXU | S_IRGRP);
    if(logfile == -1) {
        perror("unable to open logfile");
    }
    getTimestamp(timestamp);
    sprintf(logbuf,"%s: %s",timestamp, msg);
    if(write(logfile,logbuf,strlen(logbuf)) == -1)
        perror("unable to write to logfile");
}
