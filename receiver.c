#include "receiver.h"

#include <netdb.h>
#include <pthread.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "init.h"
#include "list.h"
#include "sender.h"

static List* pList_recevied = NULL;

static pthread_cond_t readReceivedListCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t readReceivedListMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_cond_t receivedListEmptyCondVar = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t receivedListEmptyMutex = PTHREAD_MUTEX_INITIALIZER;

void* receiver(void* remoteServer) {
  pList_recevied = List_create();
  // sleep(1);
  struct addrinfo* sinRemote = (struct addrinfo*)remoteServer;
  unsigned int sin_len = sizeof(sinRemote->ai_addr);

  int bytesRx = -1;

  char messageRx[MSG_MAX_LEN] = {'\0'};

  while (1) {
    memset(&messageRx, 0, sizeof(messageRx));
    bytesRx = recvfrom(sockFD, messageRx, MSG_MAX_LEN, 0, sinRemote->ai_addr,
                       &sin_len);
    char* dynamicBuffer = (char*)malloc(MSG_MAX_LEN);
    memcpy(dynamicBuffer, messageRx, MSG_MAX_LEN);

    int len_rx = strlen(dynamicBuffer);
    if (bytesRx != -1) {
      if (List_count(pList_recevied) >= LIST_MAX_NUM_NODES) {
        printf("Can not add more nodes to linked list\n");
        exit(0);
      }
      pthread_mutex_lock(&readReceivedListMutex);
      {
        List_add(pList_recevied, dynamicBuffer);
        pthread_cond_signal(&readReceivedListCondVar);
        // printf("added to receiver list: %d\n", List_count(pList_recevied));
      }
      pthread_mutex_unlock(&readReceivedListMutex);
      if (((dynamicBuffer[0] == '!') && (dynamicBuffer[1] == '\n')) ||
          (strstr(dynamicBuffer, "\n!\n")) ||
          (strstr((char*)&dynamicBuffer[len_rx - 2], "\n!"))) {
        isActiveSession = false;
        shutdown_network_in();
      }
    } else {
      printf("Error receiving message!\n");
      exit(0);
    }
  }
  return NULL;
}

void* printMessage(void* unused) {
  char* ret;

  while (1) {
    pthread_mutex_lock(&readReceivedListMutex);
    {
      pthread_cond_wait(&readReceivedListCondVar, &readReceivedListMutex);
      if (List_count(pList_recevied) > 0) {
        // printf("removed: %d\n", List_count(pList_recevied));
        ret = List_remove(pList_recevied);
        write(STDOUT_FILENO, ret, MSG_MAX_LEN);
        // printf("removing from receiver list\n");
      }
      // len_ret = strlen(ret);
    }
    pthread_mutex_unlock(&readReceivedListMutex);

    if ((((ret[0] == '!') && (ret[1] == '\n')) || !isActiveSession)) {
      printf("\n\nReceived ! -- Shutting down!!\n");
      free(ret);
      ret = NULL;
      isActiveSession = false;
      // sleep(1);
      shutdown_screen_in();
      shutdown_network_out();
      shutdown_screen_out();
      cleanupPthreads();
    }

    free(ret);
    ret = NULL;
  }
  return NULL;
}

void shutdown_network_in() {
  // sleep(1);
  if (pthread_cancel(network_in) != 0)
    ;
  // if (pthread_cancel(network_in) != 0){
  //     printf("error cancelling receiver\n");
  // }
  printf("cancelled print thread\n");
}

void shutdown_screen_out() {
  List_free(pList_recevied, listFreeFn);
  // sleep(1);
  if (pthread_cancel(screen_out) != 0)
    ;
  // if (pthread_cancel(screen_out) != 0) {
  //     printf("error cancelling print thread\n");
  // }
  printf("cancelled print thread\n");
}

void cleanupPthreads_receiver() {
  pthread_mutex_unlock(&receivedListEmptyMutex);
  pthread_mutex_unlock(&receivedListEmptyMutex);
  pthread_cond_signal(&receivedListEmptyCondVar);
  pthread_cond_signal(&readReceivedListCondVar);

  pthread_mutex_destroy(&receivedListEmptyMutex);
  pthread_mutex_destroy(&readReceivedListMutex);
  pthread_cond_destroy(&receivedListEmptyCondVar);
  pthread_cond_destroy(&readReceivedListCondVar);
}
