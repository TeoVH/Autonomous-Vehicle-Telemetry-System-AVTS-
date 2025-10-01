#include "logger.h"
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

void init_logger(const char *filename) {
    log_file = fopen(filename, "a");
    if (!log_file) {
        perror("Error abriendo archivo de log");
        exit(EXIT_FAILURE);
    }
}

void close_logger() {
    if (log_file) fclose(log_file);
}

void log_event(const char *fmt, ...) {
    char timebuf[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S", t);

    pthread_mutex_lock(&log_mutex);

    fprintf(stdout, "[%s] ", timebuf);
    if (log_file) fprintf(log_file, "[%s] ", timebuf);

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout, fmt, args);
    va_end(args);

    if (log_file) {
        va_list args_copy;
        va_start(args_copy, fmt);
        vfprintf(log_file, fmt, args_copy);
        va_end(args_copy);
    }

    fflush(stdout);
    if (log_file) fflush(log_file);

    pthread_mutex_unlock(&log_mutex);
}

void log_user_action(const char *username, const char *role, const char *action, const char *details) {
    char timebuf[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S", t);

    pthread_mutex_lock(&log_mutex);

    // Formato: [timestamp] USER_ACTION [username:role] action: details
    fprintf(stdout, "[%s] USER_ACTION [%s:%s] %s", timebuf, username, role, action);
    if (details && strlen(details) > 0) {
        fprintf(stdout, ": %s", details);
    }
    fprintf(stdout, "\n");

    if (log_file) {
        fprintf(log_file, "[%s] USER_ACTION [%s:%s] %s", timebuf, username, role, action);
        if (details && strlen(details) > 0) {
            fprintf(log_file, ": %s", details);
        }
        fprintf(log_file, "\n");
    }

    fflush(stdout);
    if (log_file) fflush(log_file);

    pthread_mutex_unlock(&log_mutex);
}

void log_connection_event(const char *event_type, const char *username, const char *role, const char *ip, int port) {
    char timebuf[32];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%dT%H:%M:%S", t);

    pthread_mutex_lock(&log_mutex);

    // Formato: [timestamp] CONNECTION [event_type] [username:role] from ip:port
    fprintf(stdout, "[%s] CONNECTION [%s] [%s:%s] from %s:%d\n", 
            timebuf, event_type, username, role, ip, port);

    if (log_file) {
        fprintf(log_file, "[%s] CONNECTION [%s] [%s:%s] from %s:%d\n", 
                timebuf, event_type, username, role, ip, port);
    }

    fflush(stdout);
    if (log_file) fflush(log_file);

    pthread_mutex_unlock(&log_mutex);
}

