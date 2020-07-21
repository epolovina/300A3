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

    int terminateIdx = 0;
    int len_input;
    // sleep(1);

    while (isActiveSession) {
        printf("Message: \n");
        memset(&input, 0, sizeof(input));
        int readStdin = read(STDIN_FILENO, input, MSG_MAX_LEN);
        // printf("input: \"%s\"\n", input);
        
        len_input = strlen(input);
        if (readStdin > 0) {
            terminateIdx = (len_input < MSG_MAX_LEN) ? (len_input) : MSG_MAX_LEN - 1;

            input[terminateIdx] = 0;
            if (List_count(pList_input) >= LIST_MAX_NUM_NODES) {
                printf("Can not add more nodes to linked list\n");
                exit(0);
            }
            pthread_mutex_lock(&readInputListMutex);
            {
                List_add(pList_input, input);
                printf("signaling sender\n");
                pthread_cond_signal(&readInputListCondVar);
            }
            pthread_mutex_unlock(&readInputListMutex);
        } else if (readStdin == 0) {
            // EOF...
            printf("Reached end of file with no !\\n \n");
            printf("You can only receive now\n");
        }

        if (((input[0] == '!') && (input[1] == '\n')) || strstr(input, "\n!\n")
            || (strstr((char*) &input[len_input - 2], "\n!"))) {
            printf("\nEntered ! -- Shutting down!!\n");
            isActiveSession = false;
            shutdown_screen_in();
            // sleep(1);
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

    char* pfound_notEnd    = NULL;
    char* pfound_end       = NULL;
    char* pfound_beginning = NULL;
    bool  exitProgram      = false;

    int     len_ret;
    int     send;
    regex_t regex;

    while (isActiveSession) {
        pthread_mutex_lock(&inputListEmptyMutex);
        {
            ret = NULL;
            printf("waiting for signal\n");
            pthread_cond_wait(&readInputListCondVar, &inputListEmptyMutex);
            ret = List_remove(pList_input);
            printf("exiting sender lock\n");
        }
        pthread_mutex_unlock(&inputListEmptyMutex);

        if (ret != NULL) {
            len_ret = strlen(ret);

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

            if (pfound_notEnd == NULL && pfound_end == NULL && pfound_beginning == NULL) {
                send = sendto(sockFD, ret, strlen(ret), 0, (struct sockaddr*) &sin, sin_len);
            } else {
                char* truncStr = (char*) calloc(MSG_MAX_LEN, sizeof(char));

                int pos;
                if (pfound_beginning != NULL) {
                    pos = pfound_beginning - ret;
                    strncpy(truncStr, ret, pos + 2);
                    send =
                        sendto(sockFD, truncStr, strlen(ret), 0, (struct sockaddr*) &sin, sin_len);
                } else if (pfound_notEnd != NULL) {
                    pos = pfound_notEnd - ret;
                    strncpy(truncStr, ret, pos + 3);
                    send =
                        sendto(sockFD, truncStr, strlen(ret), 0, (struct sockaddr*) &sin, sin_len);
                } else {
                    send = sendto(sockFD, ret, strlen(ret), 0, (struct sockaddr*) &sin, sin_len);
                }
                free(truncStr);
                exitProgram = true;
            }

            if (send == -1) {
                printf("ERROR SENDING UDP PACKET\n");
                exit(0);
            } else if (((ret[0] == '!') && (ret[1] == '\n')) || exitProgram) {
                isActiveSession = false;
                // sleep(1);
                shutdown_network_in();
                shutdown_screen_out();
                shutdown_network_out();
                cleanupPthreads();
            }
            pthread_cond_signal(&inputListEmptyCondVar);
        }
    }
    return NULL;
}

void shutdown_screen_in()
{
    // sleep(1);
    pthread_cancel(screen_in);
}

void shutdown_network_out()
{
    List_free(pList_input, listFreeFn);
    // sleep(1);
    pthread_cancel(network_out);
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
