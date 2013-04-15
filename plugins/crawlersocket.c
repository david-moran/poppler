#include "crawler.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SZ  64

extern int crawlersocket(struct crawler *crawler,
    struct crawler_message *message);

extern int crawlersocket(struct crawler *crawler,
    struct crawler_message *message) {
    char *socket_address, *socket_port, *type, buf[BUF_SZ];
    int sockfd, err;
    struct sockaddr_in saddr;

    memset(&saddr, 0, sizeof(struct sockaddr_in));

    socket_address = crawler->path[message->path_ndx]->module.args[0];
    socket_port = crawler->path[message->path_ndx]->module.args[1];
    type = crawler->path[message->path_ndx]->module.args[2];

    err = sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (err == -1) {
        fprintf(stderr, "[%s:%d] socket(): %s\n", __FILE__, __LINE__,
            strerror(errno));
        return -1;
    }

    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(atoi(socket_port));
    inet_aton(socket_address, &saddr.sin_addr);

    err = connect(sockfd, (struct sockaddr *) &saddr,
        sizeof(struct sockaddr_in));
    if (err == -1) {
        close(sockfd);
        perror("connect()");
        fprintf(stderr, "[%s:%d] connect(): %s\n", __FILE__, __LINE__,
            strerror(errno));
    }

    snprintf(buf, BUF_SZ, "%s:%s:%d", type, inet_ntoa(message->in_addr), message->port);
    write(sockfd, buf, strlen(buf));
    close(sockfd);

    return 0;
}
