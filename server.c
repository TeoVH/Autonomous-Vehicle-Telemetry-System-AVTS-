#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "server.h"
#include "client_handler.h"
#include "logger.h"


#define MAX_CLIENTS 100

// ----------------- Estado global del servidor -----------------
static struct client_data *clients[MAX_CLIENTS];
static int client_count = 0;
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

float speed_offset = 0;
int   direction_state = 0;   // 0:FORWARD, 1:LEFT, 2:RIGHT, 3:STOP
int   battery_level  = 100;
int   temp_c         = 30;

static int server_fd = -1;            // para cierre limpio en SIGINT
static volatile int running = 1;      // bandera de ciclo principal

// ----------------- Helpers de lista de clientes -----------------
void add_client(struct client_data *client) {
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_CLIENTS) {
        clients[client_count++] = client;
        client->active = 1;
    }
    pthread_mutex_unlock(&clients_mutex);
}

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

// ----------------- Hilo de telemetría -----------------
static void *telemetry_thread(void *arg) {
    (void)arg;
    srand((unsigned)time(NULL));

    const char *dirs[] = {"FORWARD", "LEFT", "RIGHT", "STOP"};

    while (running) {
        sleep(10);

        // Velocidad base simulada + offset administrable
        float base_speed = (float)(rand() % 101);
        float speed = base_speed + speed_offset;
        if (speed < 0) speed = 0;

        // Temperatura simulada (guardar en global para STATUS)
        temp_c = 20 + (rand() % 21); // 20–40 °C enteros

        // Batería global baja 5% por tick
        battery_level -= 5;
        if (battery_level <= 0) battery_level = 100;

        int dir_idx = direction_state;
        if (dir_idx < 0 || dir_idx > 3) dir_idx = 0;

        // Mensaje de broadcast (con \n porque es framing del protocolo)
        char msg[128];
        snprintf(msg, sizeof(msg), "DATA %.1f %d %s %d\n",
                 speed, battery_level, dirs[dir_idx], temp_c);

        // Enviar a todos los clientes autenticados
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < client_count; i++) {
            struct client_data *c = clients[i];
            if (c->authenticated) {
                ssize_t r = send(c->socket_fd, msg, strlen(msg), 0);
                if (r < 0) {
                    log_event("Error enviando DATA a %s\n", c->username);
                }
            }
        }
        pthread_mutex_unlock(&clients_mutex);

        log_event("Broadcast DATA: %s", msg);
    }
    return NULL;
}

// ----------------- Handler de SIGINT -----------------
static void handle_sigint(int sig) {
    (void)sig;
    log_event("SIGINT recibido, cerrando servidor...\n");
    running = 0;

    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }

    // Cerrar todos los clientes activos
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; i++) {
        if (clients[i]) {
            close(clients[i]->socket_fd);
            clients[i]->active = 0;
            free(clients[i]);
            clients[i] = NULL;
        }
    }
    client_count = 0;
    pthread_mutex_unlock(&clients_mutex);

    close_logger();
    // No exit() aquí: dejamos que start_server termine y cierre ordenado.
}

// ----------------- Arranque del servidor -----------------
void start_server(int port) {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;

    // Señales: SIGINT para cierre, SIGPIPE ignore para evitar kill en send()
    signal(SIGINT,  handle_sigint);
    signal(SIGPIPE, SIG_IGN);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(port);

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
    pthread_t tel_tid;
    if (pthread_create(&tel_tid, NULL, telemetry_thread, NULL) == 0) {
        pthread_detach(tel_tid);
    } else {
        log_event("No se pudo crear hilo de telemetría\n");
    }

    // Loop principal de aceptación de clientes
    while (running) {
        client_len = sizeof(client_addr); // ¡re-inicializar SIEMPRE antes de accept!
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (!running) break;

        if (client_fd < 0) {
            if (errno == EINTR) {
                // Interrumpido por señal; reintentar si seguimos corriendo
                continue;
            }
            perror("Error en accept");
            continue;
        }

        struct client_data *data = (struct client_data *)malloc(sizeof(struct client_data));
        if (!data) {
            perror("malloc");
            close(client_fd);
            continue;
        }

        data->socket_fd     = client_fd;
        data->addr          = client_addr;
        data->authenticated = 0;
        data->active        = 1;
        data->role[0]       = '\0';
        data->username[0]   = '\0';

        add_client(data);

        pthread_t client_tid;
        if (pthread_create(&client_tid, NULL, handle_client, data) != 0) {
            perror("Error creando hilo de cliente");
            close(client_fd);
            free(data);
        } else {
            pthread_detach(client_tid);
        }
    }

    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
    }
    log_event("Servidor detenido.\n");
}
