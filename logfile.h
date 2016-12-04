/*
 * @filename:    logfile.h
 * @author:      Stefan Stockinger
 * @date:        2016-11-16
 * @description: this file contains the logfile function
*/
#ifndef LOGFILE_H
#define LOGFILE_H


/** @brief writes to a logfile
 * @param	filename    pointer to the filename
 * 		msg         pointer to the massages which should be written to the logfile
 *
 * @return 	nothing (void)
 */
void log_message(char *filename, char* msg);

#endif
