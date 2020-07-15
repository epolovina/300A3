#include "init.h"
#include "netdb.h"
#include <stdio.h>
#include <stdlib.h>

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

    cleanupPthreads();
    closeSocket();

    return 0;
}
