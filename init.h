#ifndef _INIT_H_
#define _INIT_H_

#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>

pthread_t screen_in;
pthread_t network_out;
pthread_t network_in;
pthread_t screen_out;

int  sockFD;
bool isActiveSession;

struct addrinfo* setupRemoteServer(char* remoteMachineName, char* remotePortNumber);
struct addrinfo* setupLocalServer(char* localPortNumberArg);

int  setup(struct addrinfo* remoteSetupInfo);
void shutdownThreads();
void cleanupPthreads();
void closeSocket();

void listFreeFn(void* pItem);

#endif
