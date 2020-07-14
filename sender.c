#include "sender.h"
#include "init.h"
#include "list.h"
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

List* pList_input = NULL;

pthread_cond_t  inputListEmptyCondVar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t inputListEmptyMutex   = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t  readInputListCondVar = PTHREAD_COND_INITIALIZER;
pthread_mutex_t readInputListMutex   = PTHREAD_MUTEX_INITIALIZER;

void* keyboard(void* unused)
{
    pList_input = List_create();
    char input[MSG_MAX_LEN] = { '\0' };
    int  terminateIdx       = 0;
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
        } else if (readStdin == 0) {
            pthread_mutex_lock(&readInputListMutex);
            {
                char* terminate = "!\n";
                List_add(pList_input, terminate);
                pthread_cond_signal(&inputListEmptyCondVar);
            }
            pthread_mutex_unlock(&readInputListMutex);
            cleanupThreads();
            shutdown_network_in();
            shutdown_screen_out();
            shutdown_screen_in();
            shutdown_network_out();
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

    char* ret     = NULL;
    int   send    = -1;
    int   sin_len = sizeof(sin);

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
// void shutdown_screen_in()
// {
//     sleep(1);
//     pthread_cancel(screen_in);
// }

// void shutdown_network_out()
// {
//     sleep(1);
//     pthread_cancel(network_out);
// }