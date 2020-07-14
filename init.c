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
    int localPortNumber = atoi(localPortNumberArg);
    printf("localPortNumber: %d\n", localPortNumber);

    // adapted from https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
    struct addrinfo hints, *res, *ptr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    int status = getaddrinfo(NULL, localPortNumberArg, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return NULL;
    }

    for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
        sockFD = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sockFD == -1) {
            printf("Error: Could not create socket\n");
            return NULL;
        }
        if (bind(sockFD, ptr->ai_addr, ptr->ai_addrlen) == -1) {
            printf("Error: Could not bind socket\n");
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
    printf("remotePortNumber: %s\n", remotePortNumber);
    printf("remoteMachineName: %s\n", remoteMachineName);
    char ipstr[INET6_ADDRSTRLEN];

    struct addrinfo hints, *res, *ptr;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags    = AI_PASSIVE;

    // adapted from https://beej.us/guide/bgnet/html/#getaddrinfoprepare-to-launch
    int status = getaddrinfo(remoteMachineName, remotePortNumber, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return NULL;
    }

    for (ptr = res; ptr != NULL; ptr = ptr->ai_next) {
        if (ptr->ai_addr->sa_family == AF_INET) {
            struct sockaddr_in* sin = (struct sockaddr_in*) ptr->ai_addr;

            void* addr = &(sin->sin_addr);
            inet_ntop(ptr->ai_family, addr, ipstr, sizeof(ipstr));
            printf("Ip of remote computer: %s\n", ipstr);
            return ptr;
        }
    }
    return NULL;
}

int setup(struct addrinfo* remoteSetupInfo)
{
    // List* pList_input    = List_create();
    // List* pList_recevied = List_create();

    pthread_create(&screen_in, NULL, keyboard, NULL);
    pthread_create(&network_out, NULL, sender, remoteSetupInfo);
    pthread_create(&network_in, NULL, receiver, remoteSetupInfo);
    pthread_create(&screen_out, NULL, printMessage, NULL);

    pthread_join(screen_in, NULL);
    pthread_join(network_out, NULL);
    pthread_join(network_in, NULL);
    pthread_join(screen_out, NULL);

    return 0;
}

void cleanupThreads()
{
    pthread_mutex_unlock(&inputListEmptyMutex);
    pthread_mutex_unlock(&receivedListEmptyMutex);
    pthread_mutex_unlock(&inputListEmptyMutex);
    pthread_mutex_unlock(&receivedListEmptyMutex);

    pthread_cond_destroy(&inputListEmptyCondVar);
    pthread_mutex_destroy(&inputListEmptyMutex);

    pthread_cond_destroy(&receivedListEmptyCondVar);
    pthread_mutex_destroy(&receivedListEmptyMutex);

    pthread_cond_destroy(&readInputListCondVar);
    pthread_mutex_destroy(&readInputListMutex);

    pthread_cond_destroy(&readReceivedListCondVar);
    pthread_mutex_destroy(&readReceivedListMutex);

    close(sockFD);
}

void shutdown_screen_in()
{
    sleep(1);
    pthread_cancel(screen_in);
}
void shutdown_network_out()
{
    sleep(1);
    pthread_cancel(network_out);
}
void shutdown_network_in()
{
    sleep(1);
    pthread_cancel(network_in);
}
void shutdown_screen_out()
{
    sleep(1);
    pthread_cancel(screen_out);
}
void listFreeFn(void* pItem)
{
    pItem = NULL;
}
