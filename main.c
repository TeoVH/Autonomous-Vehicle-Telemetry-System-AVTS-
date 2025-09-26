#include "server.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <puerto> <archivo_log>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[1]);
    const char *log_filename = argv[2];

    init_logger(log_filename);

    start_server(port);

    close_logger();
    return 0;
}

