#include "crawler.h"
#include "crawler_util.h"
#include "crawler_config.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>

/* Allowed sockets errors before go to next ip */
#define ERROR_COUNT_LIMIT   20
#define RANDOM(s1, s2)  s1 + (random() % (s2 - s1))
#define PUSH_MESSAGE(crawler, message)  do {\
                err = crawler_message_push(crawler, &message);\
                if (err == -1)\
                    RANDOM_SLEEP(3);\
            } while(err == -1);


static void *_crawler_start(void *);
static void crawler_check_paths(struct crawler *crawler, struct sockaddr_in *saddr);
static uint32_t ip2int(const char *ip);


int crawler_start(struct crawler *crawler) {
    pthread_t thid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, 16384);
    thid = pthread_create(&thid, &attr, _crawler_start, crawler);
    pthread_attr_destroy(&attr);
    if (thid == -1)
        return -1;

    /* Fire & forget */
    pthread_detach(thid);

    return 0;
}

static void *_crawler_start(void *user_data) {
    struct crawler *crawler = (struct crawler *) user_data;
    uint32_t s1, s2, r;
    /* server and client */
    struct sockaddr_in saddr;
    int sfd;

    /* timeout */
    fd_set fdset;
    struct timeval tv;

    /* Determine which ip is greater than other */
    s2 = crawler->startip <= crawler->stopip ? crawler->stopip : crawler->startip;
    s1 = crawler->startip > crawler->stopip ? crawler->stopip : crawler->startip;

    memset(&saddr, 0, sizeof(struct sockaddr_in));
    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(crawler->port);

    RANDOM_SLEEP(10);

    while(1) {
        crawler->shared->total_requests++;

        sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sfd == -1)
            continue;

        fcntl(sfd, F_SETFL, O_NONBLOCK);

        /* Select a random IP to check */
        r = RANDOM(s1, s2);
        saddr.sin_addr.s_addr = htonl(r);


        /* Asynchronous connect, only to set a timeout */
        FD_ZERO(&fdset);
        FD_SET(sfd, &fdset);
        tv.tv_sec = crawler->connect_timeout;
        tv.tv_usec = 0;

        connect(sfd, (struct sockaddr *) &saddr, sizeof(struct sockaddr_in));
        if (select(sfd + 1, NULL, &fdset, NULL, &tv) == 1) {
            int so_error;
            socklen_t len = sizeof(int);
            getsockopt(sfd, SOL_SOCKET, SO_ERROR, &so_error, &len);
            if (so_error == 0) {
                close(sfd);
                /* Port opened */
#ifdef DEBUG
                printf("[%s:%d] %s:%d opened\n", __FILE__, __LINE__,
                    inet_ntoa(saddr.sin_addr), ntohs(saddr.sin_port));
#endif
                crawler_check_paths(crawler, &saddr);
            }
        }
        close(sfd);
    }

    return NULL;
}

static uint32_t ip2int(const char *ip) {
    struct in_addr in;
    uint32_t r;

    if (inet_aton(ip, &in) == -1) {
        fprintf(stderr, "Invalid IP: %s\n", ip);
        exit(-1);
    }

    r = htonl(in.s_addr);

    return r;
}

static void crawler_check_paths(struct crawler *crawler, struct sockaddr_in *saddr) {
   char buf[2048];
   struct crawler_path *path;
   struct crawler_message message;
   unsigned i, j, sfd, errcount, err;

    /* Start static-crawling */
    for (errcount = 0, i = 0; crawler->path[i] != NULL; i++) {
        err = sfd = socket(AF_INET, SOCK_STREAM, 0);
        if (err == -1) {
            errcount += 1;
            if (errcount > ERROR_COUNT_LIMIT)
                break;
            continue;
        }

        err = connect(sfd, (struct sockaddr *) saddr, sizeof(struct sockaddr_in));
        if (err == -1) {
            errcount += 1;
            close(sfd);
            if (errcount > ERROR_COUNT_LIMIT)
                break;
            continue;
        }
        path = crawler->path[i];
        snprintf(buf, 2048, "GET %s HTTP/1.0\r\n\r\n", path->str);
        err = write(sfd, buf, strlen(buf));
        if (err == -1) {
            errcount += 1;
            close(sfd);
            if (errcount > ERROR_COUNT_LIMIT)
                break;
            continue;
        }

        err = read(sfd, buf, 2048);
        if (err == -1) {
            errcount += 1;
            close(sfd);
            if (errcount > ERROR_COUNT_LIMIT)
                break;
            continue;
        }

        crawler->shared->hit_requests++;

        if (strstr(buf, "200 OK") != NULL) {
#ifdef DEBUG
            printf("\t[%s,%d] Server status 200 OK\n", __FILE__, __LINE__);
#endif
            message.path_ndx = i;
            message.kw_ndx = -1;
            message.in_addr = saddr->sin_addr;
            message.port = crawler->port;
            /* We need to match some words before throw an alert */
            if (path->needle) {
                for(j = 0; path->needle[j]; j++) {
                    if (strcasestr(buf, path->needle[j]) != NULL) {
                        message.kw_ndx = j;
#ifdef DEBUG
                        printf("\t\t[%s,%d] Keyword %s found, pushing message into the queue\n",
                            __FILE__, __LINE__, path->needle[j]);
#endif
                        PUSH_MESSAGE(crawler, message);
                        break;
                    }
                }
            } else {
#ifdef DEBUG
                printf("\t\t[%s,%d] Path found. No keywords. Pushing message into the queue\n",
                    __FILE__, __LINE__);
#endif
                PUSH_MESSAGE(crawler, message);
            }
        }

        close(sfd);
   }
}

void crawler_free(struct crawler *crawler) {
    unsigned i, j;

    if (crawler == NULL)
        return;

    /* Cleaning paths */
    for (i = 0; crawler->path[i] != NULL; i++) {
        free(crawler->path[i]->str);
        free(crawler->path[i]->module.name);
        if (crawler->path[i]->module.args) {
            for (j = 0; crawler->path[i]->module.args[j]; j++) {
                free(crawler->path[i]->module.args[j]);
            }
            free(crawler->path[i]->module.args);
        }

        for (j = 0; crawler->path[i]->needle[j] != NULL; j++) {
            free(crawler->path[i]->needle[j]);
        }
        free(crawler->path[i]->needle);
        free(crawler->path[i]);
    }
    free(crawler->path);

    /* Cleaning semaphore */
    sem_destroy(&crawler->shared->sem);
    munmap(crawler->shared, sizeof(struct crawler_shared));

    free(crawler);
}

struct crawler *crawler_new(const char *start_ip, const char *stop_ip,
    const char *file) {
    int error;
    struct crawler *crawler;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    srandom(tv.tv_usec);

    crawler = (struct crawler *) malloc(sizeof(struct crawler));
    if (crawler == NULL)
        return NULL;

    crawler->startip = ip2int(start_ip);
    crawler->stopip = ip2int(stop_ip);

    crawler->shared = (struct crawler_shared *) 
        mmap(NULL, sizeof(struct crawler_shared), PROT_READ | PROT_WRITE, 
            MAP_ANONYMOUS | MAP_SHARED, -1, 0);
    if (crawler->shared == NULL) {
        perror("mmap");
        free(crawler);
    }

    crawler->shared->total_requests = 0;
    crawler->shared->hit_requests = 0;
    crawler->shared->message_pos = 0;

    error = sem_init(&crawler->shared->sem, 1, 1);
    if (error == -1) {
        perror("sem_init");
        munmap(crawler->shared, sizeof(struct crawler_shared));
        free(crawler);
    }

    crawler->path = crawler_config_path_parse(file);
    if (crawler->path == NULL) {
        sem_destroy(&crawler->shared->sem);
        munmap(&crawler->shared->sem, sizeof(struct crawler_shared));
        free(crawler);
        return NULL;
    }

    return crawler;
}

int crawler_message_push(struct crawler *crawler, 
    struct crawler_message *message) {

    /* Critical section */
    sem_wait(&crawler->shared->sem);

    /* If queue is full */
    if (crawler->shared->message_pos >= MAX_MESSAGES) {
        printf("[%s:%d] Queue is full\n", __FILE__, __LINE__);
        sem_post(&crawler->shared->sem);
        return -1;
    }

    memcpy(&crawler->shared->message[crawler->shared->message_pos], message,
        sizeof(struct crawler_message));
    crawler->shared->message_pos++;

    sem_post(&crawler->shared->sem);
    /*********************/

    return 0;
}

struct crawler_message *
crawler_message_pop(const struct crawler *crawler) {
    struct crawler_message *message;

#ifdef DEBUG
	printf("[%s:%d] Popping a message from the queue\n", __FILE__, __LINE__);
#endif
    /* Critical section */
    sem_wait(&crawler->shared->sem);
    if (crawler->shared->message_pos == 0) {
#ifdef DEBUG
		printf("[%s:%d] No new messages\n", __FILE__, __LINE__);
#endif
        sem_post(&crawler->shared->sem);
        return NULL;
    }

   message = (struct crawler_message *) malloc(sizeof(struct crawler_message));
    memcpy(message, &crawler->shared->message[crawler->shared->message_pos-1],
        sizeof(struct crawler_message));

    crawler->shared->message_pos--;

    sem_post(&crawler->shared->sem);
    /*********************/

#ifdef DEBUG
		printf("[%s:%d] New message found\n", __FILE__, __LINE__);
#endif
 
    return message;
}
