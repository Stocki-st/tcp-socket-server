/*
 * @filename:    timestamp.c
 * @author:      Stefan Stockinger
 * @date:        2016-11-16
 * @description: creates a timestamp
*/

#include <stdlib.h>
#include <time.h>


/** @brief generates a timestamp
 *  
 */
char* get_timestamp(void)
{
    time_t now = time(NULL);
    return asctime(localtime(&now));
}


/** @brief generates a timestamp
 *  source of this fucking magic function: stefan tauner
 */
void getTimestamp(char *bufferTime) {
    time_t rawTime;
    time(&rawTime);
    struct tm *timeInfo;

    timeInfo = localtime(&rawTime);
    strftime(bufferTime, 80, "%Y/%d/%m-%H:%M:%S", timeInfo);
}
