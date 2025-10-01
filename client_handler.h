#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <netinet/in.h>
#include <time.h>
#include "config.h"

struct client_data {
    int socket_fd;
    struct sockaddr_in addr;

    int authenticated;       // 0/1
    char role[16];           // "ADMIN"/"OBSERVER"
    char username[32];
    int active;

    // Seguridad / login
    int failed_attempts;     // intentos fallidos consecutivos
    int locked;              // 0/1 si está bloqueado
    time_t locked_until;     // si locked==1 y now<locked_until -> sigue bloqueado
};

void *handle_client(void *arg);

#endif
