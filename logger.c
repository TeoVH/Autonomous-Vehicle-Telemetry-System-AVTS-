#include "logger.h"
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>

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

