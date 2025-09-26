#include "client_handler.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "server.h"
#include <openssl/sha.h>   // Para hash SHA256

#define BUFFER_SIZE 256

// Usuarios permitidos (ejemplo de demo)
struct user_entry {
    const char *username;
    const char *role;
    const char *password_hash; // SHA256 en hex
};

static struct user_entry users[] = {
    {"root", "ADMIN",    "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8"}, // "password"
    {"juan", "OBSERVER", "5994471abb01112afcc18159f6cc74b4f511b99806da59b3caf5a9c173cacfc5"}, // "12345"
};
static int user_count = 2;

// Convierte binario -> hex
void to_hex(unsigned char *hash, char *output) {
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output + (i * 2), "%02x", hash[i]);
    }
    output[64] = '\0';
}

// Función para verificar credenciales
int check_credentials(const char *username, const char *role, const char *password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char hash_hex[65];

    SHA256((unsigned char *)password, strlen(password), hash);
    to_hex(hash, hash_hex);

    for (int i = 0; i < user_count; i++) {
        if (strcmp(users[i].username, username) == 0 &&
            strcmp(users[i].role, role) == 0 &&
            strcmp(users[i].password_hash, hash_hex) == 0) {
            return 1; // válido
        }
    }
    return 0; // no válido
}

void *handle_client(void *arg) {
    struct client_data *data = (struct client_data *)arg;
    int client_fd = data->socket_fd;

    // Inicializar sesión
    data->authenticated = 0;
    strcpy(data->role, "NONE");
    strcpy(data->username, "");

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(data->addr.sin_port);

    char buffer[BUFFER_SIZE];
    log_event("Cliente conectado desde %s:%d\n", client_ip, client_port);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received <= 0) {
            log_event("Cliente %s:%d desconectado.\n", client_ip, client_port);
            data->active = 0;
            remove_client(data);

            close(client_fd);
            free(data);
            return NULL;
        }

        buffer[bytes_received] = '\0';

        log_event("[%s:%d] %s\n", client_ip, client_port, buffer);

        // Procesar comando
        if (strncmp(buffer, "LOGIN", 5) == 0) {
            char role[16], username[32], password[64];
            if (sscanf(buffer, "LOGIN %15s %31s %63s", role, username, password) == 3) {
                if (check_credentials(username, role, password)) {
                    data->authenticated = 1;
                    strcpy(data->role, role);
                    strcpy(data->username, username);
                    send(client_fd, "200 OK\n", 7, 0);
                    log_event("Usuario %s (%s) autenticado\n", username, role);
                } else {
                    send(client_fd, "401 UNAUTHORIZED\n", 17, 0);
                    log_event("Fallo de login para %s (%s)\n", username, role);
                }
            } else {
                send(client_fd, "400 BAD REQUEST\n", 16, 0);
            }
        }
        else if (strncmp(buffer, "LOGOUT", 6) == 0) {
            data->authenticated = 0;
            strcpy(data->role, "NONE");
            strcpy(data->username, "");
            send(client_fd, "200 OK\n", 7, 0);
            log_event("Cliente %s:%d cerró sesión\n", client_ip, client_port);
        }
        else if (strncmp(buffer, "CMD", 3) == 0) {
            if (!data->authenticated) {
                send(client_fd, "401 UNAUTHORIZED\n", 17, 0);
                log_event("CMD rechazado: cliente no autenticado\n");
            } else if (strcmp(data->role, "ADMIN") != 0) {
                send(client_fd, "403 FORBIDDEN\n", 14, 0);
                log_event("CMD rechazado: %s no es ADMIN\n", data->username);
            } else {
                char action[32];
                if (sscanf(buffer, "CMD %31[^\n]", action) == 1) {
                    // Efectos sobre variables globales
                    if (strcasecmp(action, "SPEED UP") == 0) {
                        speed_offset += 5;
                    } else if (strcasecmp(action, "SLOW DOWN") == 0) {
                        speed_offset -= 5;
                    } else if (strcasecmp(action, "STOP") == 0) {
                        speed_offset = 0;
                    } else if (strcasecmp(action, "TURN LEFT") == 0) {
                        direction_state = 1;
                    } else if (strcasecmp(action, "TURN RIGHT") == 0) {
                        direction_state = 2;
                    } else if (strcasecmp(action, "FORWARD") == 0) {
                        direction_state = 0;
                    }

                    char response[64];
                    snprintf(response, sizeof(response), "200 OK CMD %s\n", action);
                    send(client_fd, response, strlen(response), 0);
                    log_event("CMD ejecutado por %s: %s\n", data->username, action);
                } else {
                    send(client_fd, "400 BAD REQUEST\n", 16, 0);
                }
            }
        }
        else {
            if (!data->authenticated) {
                send(client_fd, "401 UNAUTHORIZED\n", 17, 0);
            } else {
                // Por ahora solo eco si está autenticado
                send(client_fd, buffer, bytes_received, 0);
            }
        }
    }

    close(client_fd);
    free(data); // liberar al final
    return NULL;
}

