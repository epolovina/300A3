#ifndef _RECEIVER_H_
#define _RECEIVER_H_

#define MSG_MAX_LEN 512

void* receiver(void* remoteServer);
void* printMessage(void* unused);
void shutdown_network_in();
void shutdown_screen_out();
void cleanupPthreads_receiver();

#endif
