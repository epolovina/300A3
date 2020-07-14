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

static pthread_t screen_in;
static pthread_t network_out;
static pthread_t network_in;
static pthread_t screen_out;

static List* pList_input    = NULL;
static List* pList_recevied = NULL;

static void listFreeFn(void* pItem);
static int  listFreeCounter = 0;

static pthread_cond_t  inputListEmptyCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t inputListEmptyMutex   = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t  readInputListCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t readInputListMutex   = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t  readReceivedListCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t readReceivedListMutex   = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t  receivedListEmptyCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t receivedListEmptyMutex   = PTHREAD_MUTEX_INITIALIZER;

int sockFD;

int main(int argc, char* argv[])
{
    if (argc != 4) {
        printf("Invalid arguments used!!\n");
        printf("To start s-talk use:\n");
        printf("s-talk [my port number] [remote machine name] [remote port number]\n");
        exit(1);
    }
    struct addrinfo* localSetupInfo = setupLocalServer(argv[1]);
    if (localSetupInfo == NULL) {
        return -1;
    }
    struct addrinfo* remoteSetupInfo = setupRemoteServer(argv[2], argv[3]);


    if (setup(remoteSetupInfo) == -1) {
        return -1;
    }

    freeaddrinfo(localSetupInfo);
    freeaddrinfo(remoteSetupInfo);

    cleanupThreads();

    return 0;
}

int setup(struct addrinfo* remoteSetupInfo)
{
    pList_input    = List_create();
    pList_recevied = List_create();

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

void* keyboard(void* unused)
{
    char input[MSG_MAX_LEN] = {'\0'};
    int  terminateIdx = 0;
    sleep(1);

    while (1) {
        printf("Message: \n");
        int readStdin = read(STDIN_FILENO, input, MSG_MAX_LEN);

        if (readStdin > 0) {
            terminateIdx = (strlen(input) < MSG_MAX_LEN) ? (strlen(input)) : MSG_MAX_LEN - 1;

            input[terminateIdx] = 0;
            pthread_mutex_lock(&readInputListMutex);
            {
                List_add(pList_input, input);
                pthread_cond_signal(&inputListEmptyCondVar);
            }
            pthread_mutex_unlock(&readInputListMutex);
        }

        if ((strlen(input) == 2) && (input[0] == '!')) {
            cleanupThreads();
            shutdown_screen_in();
            sleep(1);
        }

        pthread_mutex_lock(&inputListEmptyMutex);
        {
            pthread_cond_wait(&inputListEmptyCondVar, &inputListEmptyMutex);
            memset(&input, 0, sizeof(input));
        }
        pthread_mutex_unlock(&inputListEmptyMutex);
    }

    return NULL;
}

void* sender(void* remoteServer)
{
    struct addrinfo*   sinRemote = (struct addrinfo*) remoteServer;
    struct sockaddr_in sin       = *(struct sockaddr_in*) sinRemote->ai_addr;

    char* ret = NULL;

    int send    = -1;
    int sin_len = sizeof(sin);

    while (1) {
        pthread_mutex_lock(&inputListEmptyMutex);
        {
            pthread_cond_wait(&inputListEmptyCondVar, &inputListEmptyMutex);
        }
        pthread_mutex_unlock(&inputListEmptyMutex);

        pthread_mutex_lock(&readInputListMutex);
        {
            ret = NULL;

            for (int i = 0; i < List_count(pList_input); i++) {
                ret = List_remove(pList_input);

                send = sendto(sockFD, ret, strlen(ret), 0, (struct sockaddr*) &sin, sin_len);
                if (send == -1) {
                    printf("ERROR SENDING UDP PACKET\n");
                } else if (ret[0] == '!' && strlen(ret) == 2) {
                    List_free(pList_input, listFreeFn);
                    List_free(pList_recevied, listFreeFn);
                    shutdown_network_in();
                    shutdown_screen_out();
                    shutdown_network_out();
                }
                pthread_cond_signal(&inputListEmptyCondVar);
            }
        }
        pthread_mutex_unlock(&readInputListMutex);
    }
    return NULL;
}

void* receiver(void* remoteServer)
{
    sleep(1);
    struct addrinfo* sinRemote = (struct addrinfo*) remoteServer;

    int  bytesRx = -1;
    char messageRx[MSG_MAX_LEN] = {'\0'};

    while (1) {
        unsigned int sin_len = sizeof(sinRemote->ai_addr);

        bytesRx = recvfrom(sockFD, messageRx, MSG_MAX_LEN, 0, sinRemote->ai_addr, &sin_len);

        if (bytesRx != -1) {
            pthread_mutex_lock(&readReceivedListMutex);
            {
                List_add(pList_recevied, messageRx);
                pthread_cond_signal(&readReceivedListCondVar);
            }
            pthread_mutex_unlock(&readReceivedListMutex);
            if (messageRx[0] == '!' && strlen(messageRx) == 2) {
                shutdown_network_in();
                sleep(2);
            }
        } else {
            printf("Error receiving message!\n");
        }

        pthread_mutex_lock(&receivedListEmptyMutex);
        {
            pthread_cond_wait(&receivedListEmptyCondVar, &receivedListEmptyMutex);
            memset(&messageRx, 0, sizeof(messageRx));
        }
        pthread_mutex_unlock(&receivedListEmptyMutex);
    }
    return NULL;
}

void* printMessage(void* unused)
{
    char* ret;
    while (1) {
        pthread_mutex_lock(&readReceivedListMutex);
        {
            pthread_cond_wait(&readReceivedListCondVar, &readReceivedListMutex);
            ret = List_remove(pList_recevied);
        }
        pthread_mutex_unlock(&readReceivedListMutex);

        pthread_mutex_lock(&receivedListEmptyMutex);
        {
            printf("Received: \n");
            write(STDOUT_FILENO, ret, MSG_MAX_LEN);
            pthread_cond_signal(&receivedListEmptyCondVar);
        }
        pthread_mutex_unlock(&receivedListEmptyMutex);
        if (ret[0] == '!' && strlen(ret) == 2) {
            listFreeCounter = 0;
            List_free(pList_input, listFreeFn);
            List_free(pList_recevied, listFreeFn);
            pthread_cancel(screen_in);
            shutdown_screen_in();
            shutdown_network_out();
            shutdown_screen_out();
        }
        fflush(stdout);
    }
    return NULL;
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
void shutdown_network_in()
{
    sleep(1);
    pthread_cancel(network_in);
}
void shutdown_network_out()
{
    sleep(1);
    pthread_cancel(network_out);
}
void shutdown_screen_out()
{
    sleep(1);
    pthread_cancel(screen_out);
}

static void listFreeFn(void* pItem)
{
    listFreeCounter++;
}

void loggerVal(char* log, int msg)
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
