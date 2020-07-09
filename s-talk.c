#include "s-talk.h"
#include "list.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEBUG 0

pthread_t threadPool[MAX_NUM_THREADS];

static List* pList_input = NULL;
static List* pList_send  = NULL;

static pthread_cond_t  s_isInputListEmptyCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t s_isInputListEmptyMutex   = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t  s_syncOkToReadInputListCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t s_syncOkToReadInputListMutex   = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t  s_syncOkToReadSendListCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t s_syncOkToReadSendListMutex   = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t  s_isSendListEmptyCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t s_isSendListEmptyMutex   = PTHREAD_MUTEX_INITIALIZER;

int sockFD;

int main(int argc, char* argv[])
{
    if (argc != 4) {
        printf("Invalid arguments used!!\n");
        printf("To start s-talk use:\n");
        printf("s-talk [my port number] [remote machine name] [remote port number]\n");
        exit(1);
    }

    setupLocalServer(argv[1]);
    struct addrinfo* remoteSetupInfo = setupRemoteServer(argv[2], argv[3]);

    pList_input = List_create();

    pthread_create(&threadPool[0], NULL, keyboard, NULL);
    pthread_create(&threadPool[1], NULL, sender, remoteSetupInfo);
    pthread_create(&threadPool[2], NULL, receiver, remoteSetupInfo);
    pthread_create(&threadPool[3], NULL, printMessage, NULL);

    pthread_join(threadPool[0], NULL);
    pthread_join(threadPool[1], NULL);
    pthread_join(threadPool[2], NULL);
    pthread_join(threadPool[3], NULL);

    return 0;
}

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
        }
        if (bind(sockFD, ptr->ai_addr, ptr->ai_addrlen) == -1) {
            printf("Error: Could not bind socket\n");
            close(sockFD);
        }

        if (ptr->ai_addr->sa_family == AF_INET) {
            freeaddrinfo(res);
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
            freeaddrinfo(res);
            return ptr;
        }
    }
    return NULL;
}

void* keyboard(void* unused)
{
    char input[MSG_MAX_LEN];
    int  terminateIdx = 0;

    while (input[0] != '!') {
        // printf("Waiting for keyboard input\n");
        if (fgets(input, MSG_MAX_LEN, stdin) == NULL) {
            continue;
        }
        terminateIdx        = (strlen(input) < MSG_MAX_LEN) ? (strlen(input)) : MSG_MAX_LEN - 1;
        input[terminateIdx] = 0;

        pthread_mutex_lock(&s_syncOkToReadInputListMutex);
        {
            List_add(pList_input, input);
            // List_print(pList_input);
            loggerVal("from KB: %s\n", input);
            pthread_cond_signal(&s_syncOkToReadInputListCondVar);
        }
        pthread_mutex_unlock(&s_syncOkToReadInputListMutex);
        loggerString("Exited first kb lock\n");
        pthread_mutex_lock(&s_isInputListEmptyMutex);
        {
            pthread_cond_wait(&s_isInputListEmptyCondVar, &s_isInputListEmptyMutex);
            loggerString("Resetting input buffer\n");
            memset(&input, 0, sizeof(input));
            fflush(stdout);
        }
        pthread_mutex_unlock(&s_isInputListEmptyMutex);
        loggerString("Exited second kb lock\n");
    }

    return NULL;
}

void* sender(void* remoteServer)
{
    struct addrinfo*   sinRemote = (struct addrinfo*) remoteServer;
    struct sockaddr_in sin       = *(struct sockaddr_in*) sinRemote->ai_addr;

    char* ret;
    int   send = -1;

    while (1) {
        loggerString("Sender waiting...\n");
        pthread_mutex_lock(&s_syncOkToReadInputListMutex);
        {
            pthread_cond_wait(&s_syncOkToReadInputListCondVar, &s_syncOkToReadInputListMutex);
            loggerString("Entered sender lock\n");
            ret = NULL;
            // List_print(pList_input);
            ret = List_remove(pList_input);
            // pthread_cond_signal(&s_isInputListEmptyCondVar);
        }
        pthread_mutex_unlock(&s_syncOkToReadInputListMutex);

        if (ret[0] == '!') {
            return NULL;
        }

        int sin_len = sizeof(sin);

        pthread_mutex_lock(&s_syncOkToReadInputListMutex);
        {
            loggerVal("Sending: %s\n", ret);
            send = sendto(sockFD, ret, strlen(ret), 0, (struct sockaddr*) &sin, sin_len);
            if (send == -1) {
                printf("ERROR SENDING UDP PACKET\n");
            }
            pthread_cond_signal(&s_isInputListEmptyCondVar);
        }
        pthread_mutex_unlock(&s_syncOkToReadInputListMutex);
    }
    return NULL;
}

void* receiver(void* remoteServer)
{
    struct addrinfo* sinRemote = (struct addrinfo*) remoteServer;
    int              bytesRx   = -1;
    char             messageRx[MSG_MAX_LEN];

    pList_send = List_create();

    while (1) {
        unsigned int sin_len = sizeof(sinRemote->ai_addr);

        loggerString("Waiting for a message...\n");
        bytesRx = recvfrom(sockFD, messageRx, MSG_MAX_LEN, 0, sinRemote->ai_addr, &sin_len);

        if (bytesRx != -1) {
            loggerVal("Received: %s\n", messageRx);
            pthread_mutex_lock(&s_syncOkToReadSendListMutex);
            {
                List_add(pList_send, messageRx);
                loggerString("added message to send list\n");
                // List_print(pList_send);
                pthread_cond_signal(&s_syncOkToReadSendListCondVar);
            }
            pthread_mutex_unlock(&s_syncOkToReadSendListMutex);
        } else {
            if (messageRx[0] == '!') {
                return NULL;
            }
        }

        pthread_mutex_lock(&s_isSendListEmptyMutex);
        {
            loggerString("Receiver in mutex waiting\n");
            pthread_cond_wait(&s_isSendListEmptyCondVar, &s_isSendListEmptyMutex);
            loggerString("Receiver after waiting\n");
            memset(&messageRx, 0, sizeof(messageRx));
        }
        pthread_mutex_unlock(&s_isSendListEmptyMutex);
    }
    return NULL;
}

void* printMessage(void* unused)
{
    char* ret;
    while (1) {
        // if (List_count(pList_send) > 0) {
        pthread_mutex_lock(&s_syncOkToReadSendListMutex);
        {
            pthread_cond_wait(&s_syncOkToReadSendListCondVar, &s_syncOkToReadSendListMutex);
            ret = NULL;
            loggerString("Removing from the send list");
            ret = List_remove(pList_send);
            loggerVal("return value after remove: \n", ret);
            // List_print(pList_send);
        }
        pthread_mutex_unlock(&s_syncOkToReadSendListMutex);
        if (ret[0] == '!') {
            // break;
            return NULL;
        }

        pthread_mutex_lock(&s_isSendListEmptyMutex);
        {
            printf("%s", ret);
            pthread_cond_signal(&s_isSendListEmptyCondVar);
        }
        pthread_mutex_unlock(&s_isSendListEmptyMutex);
        fflush(stdout);
    }

    return NULL;
}

void loggerVal(char* log, char* msg)
{
    if (DEBUG) {
        printf(log, msg);
    }
}
void loggerString(char* log)
{
    if (DEBUG) {
        printf("%s\n", log);
    }
}