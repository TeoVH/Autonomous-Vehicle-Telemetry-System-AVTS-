#include "client_handler.h"
#include "logger.h"
#include "server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>
#include <strings.h>
#include <errno.h>
#include <sys/time.h>


#define BUFFER_SIZE 512
#define MAX_FAILED_ATTEMPTS 3
#define LOCK_SECONDS 300 // 5 minutos

static void trim_crlf(char *s) {
    size_t n = strlen(s);
    while (n>0 && (s[n-1]=='\r' || s[n-1]=='\n')) { s[--n]=0; }
}

static void to_upper(char *s) {
    for (; *s; ++s) *s = (char)toupper((unsigned char)*s);
}

static void send_line(int fd, const char *line) {
    send(fd, line, (int)strlen(line), 0);
    send(fd, "\n", 1, 0);
}

static int parse_login(const char *line, char *role, size_t role_sz,
                       char *user, size_t user_sz, char *pass, size_t pass_sz) {
    // Esperado: LOGIN <ROLE> <user> <pass>
    char cmd[16]={0}, rbuf[32]={0}, ubuf[64]={0}, pbuf[64]={0};
    int n = sscanf(line, "%15s %31s %63s %63s", cmd, rbuf, ubuf, pbuf);
    if (n != 4) return 0;
    for (char *p=cmd; *p; ++p) *p=(char)toupper(*p);
    if (strcmp(cmd, "LOGIN")!=0) return 0;
    for (char *p=rbuf; *p; ++p) *p=(char)toupper(*p);
    snprintf(role, role_sz, "%s", rbuf);
    snprintf(user, user_sz, "%s", ubuf);
    snprintf(pass, pass_sz, "%s", pbuf);
    return 1;
}

static int authenticate(const char *role, const char *user, const char *pass) {
    // TODO: Reemplaza con tu verificación real si la tienes (hash, db, etc.)
    if (strcmp(role, "ADMIN")==0) {
        return (strcmp(user,"root")==0 && strcmp(pass,"password")==0);
    } else if (strcmp(role, "OBSERVER")==0) {
        return (strcmp(user,"juan")==0 && strcmp(pass,"12345")==0);
    }
    return 0;
}

static const char* dir_to_str(int d) {
    switch (d) {
        case 1: return "LEFT";
        case 2: return "RIGHT";
        case 3: return "STOP";
        default: return "FORWARD";
    }
}

// --- Comandos ADMIN requeridos ---
static void handle_command_admin(struct client_data *data, const char *line) {
    char buf[BUFFER_SIZE]; snprintf(buf, sizeof(buf), "%s", line);
    trim_crlf(buf);

    char up[BUFFER_SIZE]; snprintf(up, sizeof(up), "%s", buf);
    to_upper(up);

    // SPEED UP (ya existía conceptualmente)
    if (strcmp(up, "SPEED UP")==0) {
        speed_offset += 1.0f;
        if (speed_offset < 0) speed_offset = 0;
        send_line(data->socket_fd, "OK\n");
        log_user_action(data->username, data->role, "SPEED UP", "speed_offset increased");
        return;
    }

    // TURN LEFT (existente)
    if (strcmp(up, "TURN LEFT")==0) {
        direction_state = 1;
        send_line(data->socket_fd, "OK\n");
        log_user_action(data->username, data->role, "TURN LEFT", "direction changed to LEFT");
        return;
    }

    // NUEVO: TURN RIGHT
    if (strcmp(up, "TURN RIGHT")==0) {
        direction_state = 2;
        send_line(data->socket_fd, "OK\n");
        log_user_action(data->username, data->role, "TURN RIGHT", "direction changed to RIGHT");
        return;
    }

    // NUEVO: BATTERY RESET
    if (strcmp(up, "BATTERY RESET")==0) {
        battery_level = 100;
        send_line(data->socket_fd, "OK\n");
        log_user_action(data->username, data->role, "BATTERY RESET", "battery level reset to 100%");
        return;
    }

    // NUEVO: STATUS
    if (strcmp(up, "STATUS")==0) {
        char out[128];
        int speed_int = (int)(speed_offset); // modelo simple
        snprintf(out, sizeof(out), "STATUS speed=%d battery=%d dir=%s temp=%d\n",
                 speed_int, battery_level, dir_to_str(direction_state), temp_c);
        send_line(data->socket_fd, out);
        log_user_action(data->username, data->role, "STATUS", "requested vehicle status");
        return;
    }

    send_line(data->socket_fd, "ERR unknown_cmd\n");
}

// Procesa UNA línea completa (sin \r\n) usando tu lógica existente
static void process_line(struct client_data *data, const char *line_in) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "%s", line_in);
    trim_crlf(buffer);

    const int client_fd = data->socket_fd;

    // ---- Fase de login ----
    if (!data->authenticated) {
        time_t now = time(NULL);
        if (data->locked && (data->locked_until == 0 || now < data->locked_until)) {
            send_line(client_fd, "ERR locked\n");
            return;
        } else if (data->locked && now >= data->locked_until) {
            data->locked = 0;
            data->failed_attempts = 0;
        }

        char role[16], user[64], pass[64];
        if (!parse_login(buffer, role, sizeof(role), user, sizeof(user), pass, sizeof(pass))) {
            send_line(client_fd, "ERR need_login\n");
            return;
        }

        int ok = authenticate(role, user, pass);
        if (!ok) {
            data->failed_attempts++;
            if (data->failed_attempts >= MAX_FAILED_ATTEMPTS) {
                data->locked = 1;
                data->locked_until = time(NULL) + LOCK_SECONDS;
                send_line(client_fd, "ERR locked\n");
            } else {
                char msg[64];
                snprintf(msg, sizeof(msg), "ERR invalid (%d/%d)\n", data->failed_attempts, MAX_FAILED_ATTEMPTS);
                send_line(client_fd, msg);
            }
            return;
        }

        // Login OK
        data->authenticated = 1;
        snprintf(data->role, sizeof(data->role), "%s", role);
        snprintf(data->username, sizeof(data->username), "%s", user);
        send_line(client_fd, "OK\n");
        
        // Log successful authentication
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(data->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(data->addr.sin_port);
        log_connection_event("LOGIN_SUCCESS", user, role, client_ip, client_port);
        return;
    }

    // ---- Fase autenticado ----
    if (strcasecmp(data->role, "ADMIN")==0) {
        handle_command_admin(data, buffer);
        return;
    }

    // OBSERVER: permitir STATUS
    if (strcasecmp(buffer, "STATUS")==0) {
        char out[128];
        int speed_int = (int)(speed_offset);
        snprintf(out, sizeof(out), "STATUS speed=%d battery=%d dir=%s temp=%d\n",
                 speed_int, battery_level, dir_to_str(direction_state), temp_c);
        send_line(data->socket_fd, out);
        log_user_action(data->username, data->role, "STATUS", "requested vehicle status");
    } else {
        send_line(client_fd, "ERR observer_only\n");
    }
}

void *handle_client(void *arg) {
    struct client_data *data = (struct client_data *)arg;
    const int client_fd = data->socket_fd;
    struct timeval timeout;

    data->authenticated = 0;
    data->active = 1;
    data->failed_attempts = 0;
    data->locked = 0;
    data->locked_until = 0;
    data->role[0] = 0;
    data->username[0] = 0;
    timeout.tv_sec = 120;
    timeout.tv_usec = 0;
    
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    // Log initial connection
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(data->addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(data->addr.sin_port);
    log_connection_event("CONNECT", "unknown", "NONE", client_ip, client_port);

    // Acumulador de entrada por líneas
    char inbuf[4096];
    size_t ilen = 0;

    while (data->active) {
        char chunk[1024];
        int n = recv(client_fd, chunk, sizeof(chunk), 0);
        if (n == 0) {
            // desconexión remota limpia
            // (log y break como ya tienes)
            break;
        } else if (n < 0) {
            // Timeout o error
            if (errno == EAGAIN || errno == EWOULDBLOCK
        #ifdef ETIMEDOUT
                || errno == ETIMEDOUT
        #endif
            ) {
                send_line(client_fd, "ERR timeout");
                log_connection_event("TIMEOUT", data->username, data->role, client_ip, client_port);
            } else {
                perror("recv");
                log_connection_event("RECV_ERROR", data->username, data->role, client_ip, client_port);
            }
            break;
        }

        // Acumular el chunk recibido
        if (ilen + (size_t)n >= sizeof(inbuf)) {
            // evitar overflow: descartamos buffer si se desborda
            ilen = 0;
        }
        memcpy(inbuf + ilen, chunk, (size_t)n);
        ilen += (size_t)n;

        // Extraer y procesar líneas completas (terminadas en '\n')
        size_t start = 0;
        for (size_t i = 0; i < ilen; ++i) {
            if (inbuf[i] == '\n') {
                size_t linelen = i - start;
                if (linelen > 0) {
                    char line[BUFFER_SIZE];
                    size_t cplen = (linelen >= sizeof(line)-1) ? sizeof(line)-1 : linelen;
                    memcpy(line, inbuf + start, cplen);
                    line[cplen] = '\0';

                    // Procesar UNA línea completa
                    process_line(data, line);
                }
                start = i + 1;
            }
        }

        // Mover remanente (fragmento incompleto) al inicio
        if (start > 0) {
            size_t rem = ilen - start;
            memmove(inbuf, inbuf + start, rem);
            ilen = rem;
        }
    }


    // Mark client as inactive and remove from server list
    data->active = 0;
    remove_client(data);
    
    log_event("[CLIENT %d] Desconectado\n", client_fd);
    close(client_fd);
    free(data);
    return NULL;
}


