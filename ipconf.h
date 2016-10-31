/* 
 * @filename:    ipconf.h
 * @author:      Stefan Stockinger
 * @date:        2016-10-31 
 * @description: this file contains global network settings
*/


#ifndef IPCONF_H 
#define IPCONF_H

#define MAXLINE 4096 //max text line length
#define LISTENQ 8 //maximum number of client connections

char* ip_address = "127.0.0.1";
uint16_t port_number = 7777;

#endif
