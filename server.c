#include "server.h"
#include "client_handler.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define MAX_CLIENTS 100

static struct client_data *clients[MAX_CLIENTS];
static int client_count = 0;
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
float speed_offset = 0;
int direction_state = 0;

// Agregar cliente
void add_client(struct client_data *client) {
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_CLIENTS) {
        clients[client_count++] = client;
        client->active = 1;
    }
    pthread_mutex_unlock(&clients_mutex);
}

// Eliminar cliente
void remove_client(struct client_data *client) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i] == client) {
            clients[i] = clients[client_count - 1];
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *telemetry_thread(void *arg) {
    srand(time(NULL));
    int battery = 100;

    const char *dirs[] = {"FORWARD", "LEFT", "RIGHT"};

    while (1) {
        sleep(10);

        // Velocidad base simulada
        float base_speed = (float)(rand() % 101);
        float speed = base_speed + speed_offset;
        if (speed < 0) speed = 0;

        // Temperatura simulada
        float temp = 20.0 + (rand() % 2000) / 100.0; // 20–40 ºC

        // Batería baja 5% cada tick
        battery -= 5;
        if (battery <= 0) battery = 100;

        // Dirección depende de los comandos CMD
        char msg[128];
        snprintf(msg, sizeof(msg), "DATA %.1f %d %s %.1f\n",
                 speed, battery, dirs[direction_state], temp);

        // Enviar a todos los clientes autenticados
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < client_count; i++) {
            struct client_data *c = clients[i];
            if (c->authenticated) {
                if (send(c->socket_fd, msg, strlen(msg), 0) < 0) {
                    log_event("Error enviando DATA a %s\n", c->username);
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        // Registrar broadcast en logs
        log_event("Broadcast DATA: %s", msg);
    }
    return NULL;
}

void start_server(int port) {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error en bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Error en listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    log_event("Servidor escuchando en puerto %d\n", port);

    // Lanzar hilo de telemetría
    pthread_t tid;
    pthread_create(&tid, NULL, telemetry_thread, NULL);
    pthread_detach(tid);

    while (1) {
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("Error en accept");
            continue;
        }

        struct client_data *data = malloc(sizeof(struct client_data));
        data->socket_fd = client_fd;
        data->addr = client_addr;
        data->authenticated = 0;
        strcpy(data->role, "NONE");
        strcpy(data->username, "");
        data->active = 1;

        add_client(data);

        pthread_t client_tid;
        if (pthread_create(&client_tid, NULL, handle_client, data) != 0) {
            perror("Error creando hilo");
            close(client_fd);
            free(data);
        } else {
            pthread_detach(client_tid);
        }
    }

    close(server_fd);
}

