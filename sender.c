#include "sender.h"
#include "init.h"
#include "list.h"
#include "receiver.h"
#include <netdb.h>
#include <pthread.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static List* pList_input = NULL;

static pthread_cond_t  inputListEmptyCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t inputListEmptyMutex   = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t  readInputListCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t readInputListMutex   = PTHREAD_MUTEX_INITIALIZER;

void* keyboard(void* unused)
{
    pList_input = List_create();

    char input[MSG_MAX_LEN] = { '\0' };

    char* pfound_notEnd    = NULL;
    char* pfound_end       = NULL;
    char* pfound_beginning = NULL;

    int     terminateIdx = 0;
    int     len_input;
    regex_t regex;
    // sleep(1);

    while (1) {
        printf("Message: \n");
        memset(&input, 0, sizeof(input));
        int   readStdin     = read(STDIN_FILENO, input, MSG_MAX_LEN);
        char* dynamicBuffer = (char*) malloc(MSG_MAX_LEN);
        memcpy(dynamicBuffer, input, MSG_MAX_LEN);
        // printf("input: \"%s\"\n", input);

        len_input = strlen(dynamicBuffer);
        if (readStdin > 0) {
            terminateIdx = (len_input < MSG_MAX_LEN) ? (len_input) : MSG_MAX_LEN - 1;

            dynamicBuffer[terminateIdx] = 0;
            if (List_count(pList_input) >= LIST_MAX_NUM_NODES) {
                printf("Can not add more nodes to linked list\n");
                exit(0);
            }

            int len_ret = strlen(dynamicBuffer);

            if (len_ret != 1 && dynamicBuffer[0] != '\n') {
                // printf("checking term!\n");
                pfound_notEnd    = strstr(dynamicBuffer, "\n!\n");
                pfound_end       = strstr((char*) &dynamicBuffer[len_ret - 2], "\n!");
                pfound_beginning = strstr((char*) &dynamicBuffer[0], "!\n");
            }

            // adapted from
            // https://stackoverflow.com/questions/1085083/regular-expressions-in-c-examples
            int compiledRegexStr = regcomp(&regex, "^!\n", REG_EXTENDED);
            if (compiledRegexStr) {
                fprintf(stderr, "Could not compile regex\n");
                exit(1);
            }
            compiledRegexStr = regexec(&regex, dynamicBuffer, 0, NULL, 0);
            if (compiledRegexStr == REG_NOMATCH) {
                pfound_beginning = NULL;
            }

            /* Free memory allocated to the pattern buffer by regcomp() */
            regfree(&regex);

            if (pfound_notEnd == NULL && pfound_end == NULL && pfound_beginning == NULL) {
                pthread_mutex_lock(&readInputListMutex);
                {
                    List_add(pList_input, dynamicBuffer);
                    // printf("List count() %d\n", List_count(pList_input));
                    // printf("added: %s", dynamicBuffer);
                    pthread_cond_signal(&readInputListCondVar);
                    // printf("Finished adding to sender list\n");
                }
                pthread_mutex_unlock(&readInputListMutex);
                // printf("sent normally\n");
            } else {
                char* truncStr = (char*) calloc(MSG_MAX_LEN, sizeof(char));

                int pos;
                if (pfound_beginning != NULL) {
                    pos = pfound_beginning - dynamicBuffer;
                    strncpy(truncStr, dynamicBuffer, pos + 2);
                    free(dynamicBuffer);
                    dynamicBuffer = NULL;
                    pthread_mutex_lock(&readInputListMutex);
                    {
                        List_add(pList_input, truncStr);
                        // printf("List count() %d\n", List_count(pList_input));
                        // printf("added: %s", dynamicBuffer);
                        pthread_cond_signal(&readInputListCondVar);
                        // printf("Finished adding to sender list\n");
                    }
                    pthread_mutex_unlock(&readInputListMutex);
                    printf("\nEntered ! -- Shutting down!!\n");
                    isActiveSession = false;
                    // pthread_cond_signal(&readInputListCondVar);
                    shutdown_screen_in();
                } else if (pfound_notEnd != NULL) {
                    pos = pfound_notEnd - dynamicBuffer;
                    strncpy(truncStr, dynamicBuffer, pos + 3);
                    free(dynamicBuffer);
                    dynamicBuffer = NULL;
                    pthread_mutex_lock(&readInputListMutex);
                    {
                        List_add(pList_input, truncStr);
                        // printf("List count() %d\n", List_count(pList_input));
                        // printf("added: %s", dynamicBuffer);
                        pthread_cond_signal(&readInputListCondVar);
                        // printf("Finished adding to sender list\n");
                    }
                    pthread_mutex_unlock(&readInputListMutex);
                    // printf("sent not end\n");
                    isActiveSession = false;
                    printf("\nEntered ! -- Shutting down!!\n");
                    // pthread_cond_signal(&readInputListCondVar);
                    shutdown_screen_in();
                } else {
                    pthread_mutex_lock(&readInputListMutex);
                    {
                        List_add(pList_input, dynamicBuffer);
                        // printf("List count() %d\n", List_count(pList_input));
                        // printf("added: %s", dynamicBuffer);
                        pthread_cond_signal(&readInputListCondVar);
                        // printf("Finished adding to sender list\n");
                    }
                    pthread_mutex_unlock(&readInputListMutex);
                    // printf("sent end\n");
                    isActiveSession = false;
                    printf("\nEntered ! -- Shutting down!!\n");
                    // pthread_cond_signal(&readInputListCondVar);
                    shutdown_screen_in();
                }
                // free(truncStr);
            }
        } else if (readStdin == 0) {
            // EOF...
            printf("Reached end of file with no !\\n \n");
            printf("You can only receive now\n");
            // exit(0);
        }
    }

    return NULL;
}

void* sender(void* remoteServer)
{
    struct addrinfo*   sinRemote = (struct addrinfo*) remoteServer;
    struct sockaddr_in sin       = *(struct sockaddr_in*) sinRemote->ai_addr;

    int   sin_len = sizeof(sin);
    char* ret     = NULL;


    int send;
    // regex_t regex;

    while (1) {
        pthread_mutex_lock(&readInputListMutex);
        // pthread_mutex_lock(&inputListEmptyMutex);
        {
            // pthread_cond_signal(&inputListEmptyCondVar);
            // printf("waiting in sender....\n");
            pthread_cond_wait(&readInputListCondVar, &readInputListMutex);
            // printf("removing from list\n");
            // if (List_count(pList_input) > 0) {
            while (List_count(pList_input) > 0) {
                ret = List_remove(pList_input);
                // printf("\nremoved: %s\n", ret);
                send = sendto(sockFD, ret, strlen(ret), 0, (struct sockaddr*) &sin, sin_len);

                printf("sent %d bytes\n", send);
                if (send == -1) {
                    printf("ERROR SENDING UDP PACKET\n");
                    exit(0);
                } else if (((ret[0] == '!') && (ret[1] == '\n')) || !isActiveSession) {
                    isActiveSession = false;
                    // sleep(1);
                    free(ret);
                    ret = NULL;
                    shutdown_network_in();
                    shutdown_screen_out();
                    shutdown_network_out();
                    cleanupPthreads();
                }
                free(ret);
                ret = NULL;
                // pthread_cond_signal(&inputListEmptyCondVar);
            }
            pthread_mutex_unlock(&readInputListMutex);
            // pthread_mutex_unlock(&inputListEmptyMutex);
        }
    }
    return NULL;
}

void shutdown_screen_in()
{
    // sleep(1);
    // shutdowncount++;
    pthread_cancel(screen_in);
    printf("screen in shutdown\n");
}

void shutdown_network_out()
{
    List_free(pList_input, listFreeFn);
    // sleep(1);
    // shutdowncount++;
    pthread_cancel(network_out);
    printf("sender in shutdown\n");
}

void cleanupPthreads_sender()
{
    pthread_mutex_unlock(&inputListEmptyMutex);
    pthread_mutex_unlock(&inputListEmptyMutex);
    pthread_cond_signal(&inputListEmptyCondVar);
    pthread_cond_signal(&readInputListCondVar);

    pthread_mutex_destroy(&inputListEmptyMutex);
    pthread_mutex_destroy(&readInputListMutex);
    pthread_cond_destroy(&inputListEmptyCondVar);
    pthread_cond_destroy(&readInputListCondVar);
}
