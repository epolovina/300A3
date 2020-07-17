#include "receiver.h"
#include "init.h"
#include "list.h"
#include "sender.h"
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

List* pList_recevied = NULL;

pthread_cond_t  readReceivedListCondVar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t readReceivedListMutex   = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t  receivedListEmptyCondVar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t receivedListEmptyMutex   = PTHREAD_MUTEX_INITIALIZER;

void* receiver(void* remoteServer)
{
    pList_recevied = List_create();
    sleep(1);
    struct addrinfo* sinRemote = (struct addrinfo*) remoteServer;
    unsigned int     sin_len   = sizeof(sinRemote->ai_addr);

    int bytesRx = -1;
    int len_rx;

    char messageRx[MSG_MAX_LEN] = { '\0' };

    while (isActiveSession) {
        bytesRx = recvfrom(sockFD, messageRx, MSG_MAX_LEN, 0, sinRemote->ai_addr, &sin_len);
        len_rx  = strlen(messageRx);
        if (bytesRx != -1) {
            pthread_mutex_lock(&readReceivedListMutex);
            {
                List_add(pList_recevied, messageRx);
                pthread_cond_signal(&readReceivedListCondVar);
            }
            pthread_mutex_unlock(&readReceivedListMutex);
            if ((messageRx[0] == '!' && len_rx == 2)
                || (strstr((char*) &messageRx[len_rx - 2], "\n!"))) {
                isActiveSession = false;
                shutdown_network_in();
            }
        } else {
            printf("Error receiving message!\n");
            exit(0);
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
    int   len_ret;
    while (isActiveSession) {
        pthread_mutex_lock(&readReceivedListMutex);
        {
            pthread_cond_wait(&readReceivedListCondVar, &readReceivedListMutex);
            ret     = List_remove(pList_recevied);
            len_ret = strlen(ret);
        }
        pthread_mutex_unlock(&readReceivedListMutex);

        pthread_mutex_lock(&receivedListEmptyMutex);
        {
            printf("Received: \n");
            write(STDOUT_FILENO, ret, MSG_MAX_LEN);
            pthread_cond_signal(&receivedListEmptyCondVar);
        }
        pthread_mutex_unlock(&receivedListEmptyMutex);
        if ((ret[0] == '!' && len_ret == 2) || (strstr((char*) &ret[len_ret - 2], "\n!"))) {
            printf("\n\nReceived ! -- Shutting down!!\n");
            List_free(pList_recevied, listFreeFn);
            isActiveSession = false;
            sleep(1);
            shutdown_screen_in();
            shutdown_network_out();
            shutdown_screen_out();
            cleanupPthreads();
        }
        fflush(stdout);
    }
    return NULL;
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

void cleanupPthreads_receiver()
{
    pthread_mutex_unlock(&receivedListEmptyMutex);
    pthread_mutex_unlock(&receivedListEmptyMutex);

    pthread_mutex_destroy(&receivedListEmptyMutex);
    pthread_mutex_destroy(&readReceivedListMutex);
    pthread_cond_destroy(&receivedListEmptyCondVar);
    pthread_cond_destroy(&readReceivedListCondVar);
}
