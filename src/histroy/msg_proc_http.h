/*
 * msg_proc_http.h
 *
 *  Created on: 2016/10/13
 *      Author: lc
 */
#ifndef MSG_PROC_HISTORY_H_
#define MSG_PROC_HISTORY_H_

#ifdef __cplusplus
    extern "C"{
#endif


char *history_getGPS(const char *imeiName, int starttime, int endtime);
char *history_getItinerary(const char *imeiName, int starttime, int endtime);
int telephone_deleteTelNumber(const char *imeiName);
int telephone_replaceTelNumber(const char *imeiName, const char *telNumber);
char *telephone_getTelNumber(const char *imeiName);
void history_freeMsg(char *msg);


#ifdef __cplusplus
    }
#endif

#endif /* MSG_PROC_HISTORY_H_ */

