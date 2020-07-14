#ifndef _INIT_H_
#define _INIT_H_

#include "list.h"
#include <netdb.h>
#include <pthread.h>

pthread_t screen_in;
pthread_t network_out;
pthread_t network_in;
pthread_t screen_out;

extern pthread_cond_t  inputListEmptyCondVar;
extern pthread_mutex_t inputListEmptyMutex;

extern pthread_cond_t  readInputListCondVar;
extern pthread_mutex_t readInputListMutex;

extern pthread_cond_t  readReceivedListCondVar;
extern pthread_mutex_t readReceivedListMutex;

extern pthread_cond_t  receivedListEmptyCondVar;
extern pthread_mutex_t receivedListEmptyMutex;

int sockFD;

struct addrinfo* setupRemoteServer(char* remoteMachineName, char* remotePortNumber);
struct addrinfo* setupLocalServer(char* localPortNumberArg);

int setup(struct addrinfo* remoteSetupInfo);

void shutdown_screen_in();
void shutdown_network_in();
void shutdown_network_out();
void shutdown_screen_out();
void cleanupThreads();
void listFreeFn(void* pItem);

#endif
