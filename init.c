#include "init.h"
#include "receiver.h"
#include "sender.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct addrinfo* setupLocalServer(char* localPortNumberArg)
{
    // int localPortNumber = atoi(localPortNumberArg);
    fprintf(stdout, "localPortNumber: %s\n", localPortNumberArg);

    // adapted from https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
    struct addrinfo localInfo, *result, *ptr;

    memset(&localInfo, 0, sizeof(localInfo));
    localInfo.ai_family   = AF_INET;
    localInfo.ai_socktype = SOCK_DGRAM;
    localInfo.ai_flags    = AI_PASSIVE;

    int status = getaddrinfo(NULL, localPortNumberArg, &localInfo, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return NULL;
    }

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        sockFD = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sockFD == -1) {
            fprintf(stderr, "%s", "Error: Could not create socket\n");
            return NULL;
        }

        if (bind(sockFD, ptr->ai_addr, ptr->ai_addrlen) == -1) {
            fprintf(stderr, "%s", "Error: Could not bind socket\n");
            close(sockFD);
            return NULL;
        }

        if (ptr->ai_addr->sa_family == AF_INET) {
            return ptr;
        }
    }
    return NULL;
}

struct addrinfo* setupRemoteServer(char* remoteMachineName, char* remotePortNumber)
{
    fprintf(stdout, "remotePortNumber: %s\n", remotePortNumber);
    fprintf(stdout, "remoteMachineName: %s\n", remoteMachineName);
    char ipstr[INET6_ADDRSTRLEN];

    struct addrinfo remoteInfo, *result, *ptr;

    memset(&remoteInfo, 0, sizeof(remoteInfo));
    remoteInfo.ai_family   = AF_INET;
    remoteInfo.ai_socktype = SOCK_DGRAM;
    remoteInfo.ai_flags    = AI_PASSIVE;

    // adapted from https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
    int status = getaddrinfo(remoteMachineName, remotePortNumber, &remoteInfo, &result);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        exit(0);
        return NULL;
    }

    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        if (ptr->ai_addr->sa_family == AF_INET) {
            struct sockaddr_in* sin = (struct sockaddr_in*) ptr->ai_addr;

            void* addr = &(sin->sin_addr);
            inet_ntop(ptr->ai_family, addr, ipstr, sizeof(ipstr));
            fprintf(stdout, "Ip of remote computer: %s\n", ipstr);
            return ptr;
        }
    }
    return NULL;
}

int setup(struct addrinfo* remoteSetupInfo)
{
    isActiveSession = true;

    if (pthread_create(&screen_in, NULL, keyboard, NULL) != 0) {
        return -1;
    }
    if (pthread_create(&network_out, NULL, sender, remoteSetupInfo) != 0) {
        return -1;
    }

    if (pthread_create(&network_in, NULL, receiver, remoteSetupInfo) != 0) {
        return -1;
    }

    if (pthread_create(&screen_out, NULL, printMessage, NULL) != 0) {
        return -1;
    }


    pthread_join(screen_in, NULL);
    pthread_join(network_out, NULL);
    pthread_join(network_in, NULL);
    pthread_join(screen_out, NULL);

    return 0;
}

void closeSocket()
{
    close(sockFD);
}

void cleanupPthreads()
{
    cleanupPthreads_receiver();
    cleanupPthreads_sender();
}

void listFreeFn(void* pItem)
{
    pItem = NULL;
}
