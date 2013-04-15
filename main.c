#include "crawler.h"
#include "crawler_util.h"
#include "crawler_dispatcher.h"
#include "crawler_config.h"

#include <glib-object.h>

#include <arpa/inet.h>
#include <getopt.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define THREADS_PER_PROCESS  500
#define SLEEP_TIMEOUT 60
#define ERROR_TIMEOUT 1000

static time_t old_time, start_time;
static unsigned old_counter;
static struct crawler *crawler;

static void help(const char *progname);
static void print_stats(int);
static void start_crawler_process(struct crawler *crawler);

/* Usage example: ./poppler -f paths -s 0.0.0.0 -e 255.255.255.255 -t 15000 */
int main (int argc, char *argv[]) {
    int opt, threads;
    unsigned processes, i, connect_timeout;
    uint16_t port;

    g_type_init();

    char *startip, *stopip, *filename;
    struct timeval tv;

    /* Only for statistics purposes */
    old_time = start_time = time(NULL);
    old_counter = 0;

    /* Ignore broken pipe errors */
    signal(SIGPIPE, SIG_IGN);

    threads = 1;
    connect_timeout = 4;
    port = 80;
    filename = NULL;
    startip = strdup("0.0.0.0");
    stopip = strdup("255.255.255.255");

    /* Argument parsing */
    while ((opt = getopt(argc, argv, "p:T:s:e:t:f:h")) != -1) {
        switch(opt) {
            case 'T':
                connect_timeout = strtol(optarg, NULL, 10);
                break;
            case 'p':
                port = strtol(optarg, NULL, 10);
                break;
            case 's':
                free(startip);
                startip = strdup(optarg);
                break;
            case 'e':
                free(stopip);
                stopip = strdup(optarg);
                break;
            case 'f':
                filename = strdup(optarg);
                break;
            case 't':
                threads = atoi(optarg);
                if (threads < 1) {
                    fprintf(stderr, "threads must be greater than 0\n");
                    exit(-1);
                }
                break;
            case 'h':
            default:
                help(argv[0]);
                exit(-1);
                break;
        }
    }

    if (filename == NULL) {
        fprintf(stderr, "You must pass -f switch\n");
        help(argv[0]);
        free(startip);
        free(stopip);
        exit(-1);
    }

    if (strcmp(startip, stopip) == 0) {
        fprintf(stderr, "-s and -e values can't be the same\n");
        free(startip);
        free(stopip);
        free(filename);
        help(argv[0]);
        exit(-1);
    }
 
    /* Structure with shared data between process and threads */
    crawler = crawler_new(startip, stopip, filename);
    if (crawler == NULL) {
        free(startip);
        free(stopip);
        free(filename);
        exit(-1);
    }

    crawler->connect_timeout = connect_timeout;
    crawler->port = port;

    /* This is not needed anymore */
    free(startip);
    free(stopip);
    free(filename);

    /* Calculating needed process and threads in each process */
    processes = threads / THREADS_PER_PROCESS;
    threads = threads % THREADS_PER_PROCESS;

    printf("Launching %d process with %d threads\n", processes + 1,
        THREADS_PER_PROCESS);

    /* Starting dispatcher */
    crawler_dispatcher_start(crawler);

    /* Linux performs better if combines processes + threads 
    so I'll start a process with crawlers threads */
    i = 0;
    while (i < processes) {
        start_crawler_process(crawler);
        i++;
    }

    /* Setting a random seed in main process */
    gettimeofday(&tv, NULL);
    srandom(tv.tv_usec);

    /* Start our own crawlers */
    for (i = 0; i < threads; i++) {
        crawler_start(crawler);
    }

    /* When a SIGUSR1 signal received main process will print statistics */
    signal(SIGUSR1, print_stats);

    /* If a child dies then I will trie to start another one */
    while(1) {
        if (wait(NULL) != -1) {
            start_crawler_process(crawler);
        }
    }

    return 0;
}

static void help(const char *progname) {
    fprintf(stderr, "Usage: %s -f <file> [-s <start ip>] [-e <stop ip>] [-t <threads>] [-T connection timeout] [-p port]\n", progname);
}

static void print_stats(int s) {
    if (s == SIGUSR1) {
        printf("Average IPs per minute: %d\n", (int) ((crawler->shared->total_requests - old_counter) / (time(NULL) - old_time)) * 60);
        printf("Success hits: %d\n", crawler->shared->hit_requests);
        printf("Timeouts: %d\n", crawler->shared->total_requests - crawler->shared->hit_requests);
        printf("Success ratio (per thousand): %2.2f\n", ((double) ((double)crawler->shared->hit_requests / (double) crawler->shared->total_requests) * 1000));
        printf("Total time: %d\n", (int) (time(NULL) - start_time));
        printf("Total IPs crawled: %d\n\n", crawler->shared->total_requests);
        old_time = time(NULL);
        old_counter = crawler->shared->total_requests;
    }
}

static void start_crawler_process(struct crawler *crawler) {
        pid_t child;
        struct timeval tv;
        unsigned threads;

        do {
            child = fork();
            if (child == 0) {
                /* Signal SIGUSR1 will be ignored in children processes */
                signal(SIGUSR1, SIG_IGN);

                /* Select a random seed for each process */
                gettimeofday(&tv, NULL);
                srandom(tv.tv_usec);

                /* Start crawler threads */
                threads = THREADS_PER_PROCESS;
                do {
                    crawler_start(crawler);
                } while (--threads);

                crawler_wait_and_exit(RANDOM_INT(600) + 300);
                exit(-1);
            } else if (child == -1) {
                perror("fork");
                usleep(ERROR_TIMEOUT);
            }
        } while (child == -1);
}
