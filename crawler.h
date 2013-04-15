#ifndef _CRAWLER_H_
#define _CRAWLER_H_

#include <semaphore.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_MESSAGES            64

struct crawler_message {
    struct in_addr in_addr;
    uint16_t port;

    unsigned path_ndx;
    int kw_ndx;
};

struct crawler_shared {
    sem_t sem;
    
    unsigned message_pos;
    struct crawler_message message[MAX_MESSAGES];

    unsigned total_requests;
    unsigned hit_requests;
};

struct crawler_module {
    char *name;
    char **args;
};

struct crawler_path {
    struct crawler_module module;
    char *str;
    char **needle;
};

struct crawler {
    struct crawler_shared *shared;

    uint16_t port;
    uint32_t connect_timeout;
    uint32_t startip;
    uint32_t stopip;
    struct crawler_path **path;
};

int crawler_start(struct crawler *crawler);
struct crawler *crawler_new(const char *start_ip, const char *stop_ip,
    const char *file);
void crawler_free(struct crawler *crawler);

int crawler_message_push(struct crawler *crawler, 
    struct crawler_message *message);

struct crawler_message *
crawler_message_pop(const struct crawler *crawler);

#endif /* _CRAWLER_H_ */
