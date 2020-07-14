#ifndef _RECEIVER_H_
#define _RECEIVER_H_

#define MSG_MAX_LEN 512

void* receiver(void* remoteServer);
void* printMessage(void* unused);

#endif