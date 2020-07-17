#ifndef _SENDER_H_
#define _SENDER_H_

#define MSG_MAX_LEN 512

void* keyboard(void* unused);
void* sender(void* remoteServer);
void  shutdown_screen_in();
void  shutdown_network_out();
void  cleanupPthreads_sender();

#endif
