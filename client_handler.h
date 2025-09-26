#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <netinet/in.h>

struct client_data {
    int socket_fd;
    struct sockaddr_in addr;

    int authenticated;
    char role[16];
    char username[32];
    int active;
};

void *handle_client(void *arg);

#endif

