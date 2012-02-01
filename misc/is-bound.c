#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* This program is useful for checking whether a socket is currently bound. */

#define DIE(args...) \
    do { \
        fprintf(stderr, args); \
        exit(1); \
    } while (0)

int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in addr = {};
    int port;

    if (argc < 2 || argc > 3
        || (argc == 2 && (port = atoi(argv[1])) == 0)
        || (argc == 3 && (port = atoi(argv[2])) == 0)) {
        DIE("Usage: %s [interface] port\n", argv[0]);
    }

    addr.sin_family = AF_INET;
    if (argc == 3) {
        if (inet_pton(AF_INET, argv[1], &(addr.sin_addr)) != 1) {
            DIE("Invalid interface %s.\n", argv[1]);
        }
    } else {
        addr.sin_addr.s_addr = INADDR_ANY;
    }
    addr.sin_port = htons(port);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        DIE("Could not create socket.\n");
    }
    /* Bind should fail if someone else has already bound this socket or if we
     * don't have permission to bind on this port.
     */
    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "Could not bind socket.\n");
        close(sockfd);
        return errno;
    }
    printf("Socket bound successfully.\n");
    close(sockfd);
    return 0;
}
