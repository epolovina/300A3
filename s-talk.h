#ifndef _S_TALK_H_
#define _S_TALK_H_

#include "list.h"
#define MAX_NUM_THREADS 4
#define MSG_MAX_LEN 512

struct addrinfo* setupLocalServer(char* localPortNumberArg);
struct addrinfo* setupRemoteServer(char* remoteMachineName, char* remotePortNumber);

void* keyboard(void* unused);
void* printMessage(void* unused);
void* sender(void* remoteServer);
void* receiver(void* remoteServer);
void  loggerVal(char* log, char* msg);
void  loggerString(char* log);
// void List_print(List* pList);


#endif
