#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFSZ 512

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Uso: %s <ip> <puerto> <role>\n", argv[0]);
        return 1;
    }
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char *role = argv[3];

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) != 1) {
        perror("inet_pton");
        return 1;
    }

    if (connect(fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        return 1;
    }

    char buf[BUFSZ];
    if (strcmp(role,"ADMIN")==0) {
        snprintf(buf,sizeof(buf),"LOGIN ADMIN root password\n");
    } else {
        snprintf(buf,sizeof(buf),"LOGIN OBSERVER juan 12345\n");
    }
    send(fd, buf, strlen(buf), 0);

    // lee respuestas y manda algunos comandos de prueba
    while (1) {
        int n = recv(fd, buf, sizeof(buf)-1, 0);
        if (n <= 0) break;
        buf[n]=0;
        printf("<< %s", buf);

        if (strstr(buf,"OK")) {
            send(fd, "STATUS\n", 7, 0);
            usleep(300000);
            if (strcmp(role,"ADMIN")==0) {
                send(fd, "SPEED UP\n", 9, 0);
                usleep(300000);
                send(fd, "TURN RIGHT\n", 11, 0);
            } else {
                send(fd, "STATUS\n", 7, 0);
            }
        }
    }

    close(fd);
    return 0;
}
