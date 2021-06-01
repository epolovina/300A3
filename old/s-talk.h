#ifndef _S_TALK_H_
#define _S_TALK_H_

#include "list.h"

#define MSG_MAX_LEN 512

struct addrinfo* setupLocalServer(char* localPortNumberArg);
struct addrinfo* setupRemoteServer(char* remoteMachineName,
                                   char* remotePortNumber);

void* keyboard(void* unused);
void* printMessage(void* unused);
void* sender(void* remoteServer);
void* receiver(void* remoteServer);
void cleanupThreads();
int setup(struct addrinfo* remoteSetupInfo);
void shutdown_screen_in();
void shutdown_network_in();
void shutdown_network_out();
void shutdown_screen_out();

#endif
