#include "crawler.h"

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern int crawlerprint(struct crawler *crawler,
    struct crawler_message *message);

extern int crawlerprint(struct crawler *crawler,
    struct crawler_message *message) {
    printf("http://%s%s", inet_ntoa(message->in_addr), 
        crawler->path[message->path_ndx]->str);
    if (message->port != 80) {
        printf(":%d", message->port);
    }
    if (message->kw_ndx != -1) {
        printf(" [%s]", 
            crawler->path[message->path_ndx]->needle[message->kw_ndx]);
    }
    printf("\n");
    return 0;
}
