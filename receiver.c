#include "receiver.h"
#include "init.h"
#include "list.h"
#include "sender.h"
#include <netdb.h>
#include <pthread.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static List* pList_recevied = NULL;

static pthread_cond_t  readReceivedListCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t readReceivedListMutex   = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t  receivedListEmptyCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t receivedListEmptyMutex   = PTHREAD_MUTEX_INITIALIZER;

void* receiver(void* remoteServer)
{
    pList_recevied = List_create();
    // sleep(1);
    struct addrinfo* sinRemote = (struct addrinfo*) remoteServer;
    unsigned int     sin_len   = sizeof(sinRemote->ai_addr);

    int bytesRx = -1;
    int len_rx;

    char messageRx[MSG_MAX_LEN] = { '\0' };

    while (isActiveSession) {
        bytesRx = recvfrom(sockFD, messageRx, MSG_MAX_LEN, 0, sinRemote->ai_addr, &sin_len);
        len_rx  = strlen(messageRx);
        if (bytesRx != -1) {
            if (List_count(pList_recevied) >= LIST_MAX_NUM_NODES) {
                printf("Can not add more nodes to linked list\n");
                exit(0);
            }
            pthread_mutex_lock(&readReceivedListMutex);
            {
                List_add(pList_recevied, messageRx);
                pthread_cond_signal(&readReceivedListCondVar);
            }
            pthread_mutex_unlock(&readReceivedListMutex);
            if (((messageRx[0] == '!') && (messageRx[1] == '\n')) || (strstr(messageRx, "\n!\n"))
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
    char* pfound_notEnd    = NULL;
    char* pfound_end       = NULL;
    char* pfound_beginning = NULL;
    bool  exitProgram      = false;
    int   len_ret;

    regex_t regex;

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
            pfound_notEnd    = strstr(ret, "\n!\n");
            pfound_end       = strstr((char*) &ret[len_ret - 2], "\n!");
            pfound_beginning = strstr((char*) &ret[0], "!\n");

            // adapted from
            // https://stackoverflow.com/questions/1085083/regular-expressions-in-c-examples
            int compiledRegexStr = regcomp(&regex, "^!\n", REG_EXTENDED);
            if (compiledRegexStr) {
                fprintf(stderr, "Could not compile regex\n");
                exit(1);
            }

            compiledRegexStr = regexec(&regex, ret, 0, NULL, 0);
            if (compiledRegexStr == REG_NOMATCH) {
                pfound_beginning = NULL;
            }

            /* Free memory allocated to the pattern buffer by regcomp() */
            regfree(&regex);
            printf("\nReceived: \n");
            if (pfound_notEnd == NULL && pfound_end == NULL) {
                write(STDOUT_FILENO, ret, MSG_MAX_LEN);
            } else {
                char* truncatedString = (char*) calloc(MSG_MAX_LEN, sizeof(char));
                int   pos;
                if (pfound_beginning != NULL) {
                    pos = pfound_beginning - ret;
                    strncpy(truncatedString, ret, pos + 2);
                    write(STDOUT_FILENO, truncatedString, MSG_MAX_LEN);
                } else if (pfound_notEnd != NULL) {
                    pos = pfound_notEnd - ret;
                    strncpy(truncatedString, ret, pos + 3);
                    write(STDOUT_FILENO, truncatedString, MSG_MAX_LEN);
                } else {
                    write(STDOUT_FILENO, ret, MSG_MAX_LEN);
                }
                exitProgram = true;
                free(truncatedString);
            }
            pthread_cond_signal(&receivedListEmptyCondVar);
        }
        pthread_mutex_unlock(&receivedListEmptyMutex);

        if ((((ret[0] == '!') && (ret[1] == '\n')) || exitProgram)) {
            printf("\n\nReceived ! -- Shutting down!!\n");
            isActiveSession = false;
            // sleep(1);
            shutdown_screen_in();
            shutdown_network_out();
            shutdown_screen_out();
            cleanupPthreads();
        }
    }
    return NULL;
}

void shutdown_network_in()
{
    // sleep(1);
    pthread_cancel(network_in);
}

void shutdown_screen_out()
{
    List_free(pList_recevied, listFreeFn);
    // sleep(1);
    pthread_cancel(screen_out);
}

void cleanupPthreads_receiver()
{
    pthread_mutex_unlock(&receivedListEmptyMutex);
    pthread_mutex_unlock(&receivedListEmptyMutex);
    pthread_cond_signal(&receivedListEmptyCondVar);
    pthread_cond_signal(&readReceivedListCondVar);

    pthread_mutex_destroy(&receivedListEmptyMutex);
    pthread_mutex_destroy(&readReceivedListMutex);
    pthread_cond_destroy(&receivedListEmptyCondVar);
    pthread_cond_destroy(&readReceivedListCondVar);
}
