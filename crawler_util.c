#include "crawler_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char **crawler_readfile(const char *file) {
    char **lines = NULL;
    char *trim;
    size_t s;
    int i, err;
    FILE *fp;

    fp = fopen(file, "r");
    if (fp == NULL) {
        perror("fopen()");
        return NULL;
    }

    err = i = 0;
    while(err != -1) {
        lines = (char**) realloc(lines, sizeof(char*) * (i+1));
        err = getline(&lines[i], &s, fp);
        if (err != -1) {
            trim = strchr(lines[i], '\n');
            if (trim)
                trim[0] = '\x0';
            i++;
        }
    }
    lines = (char**) realloc(lines, sizeof(char*) * (i+1));
    lines[i] = NULL;

    fclose(fp);

    return lines;
}

void crawler_wait_forever(void) {
    while(1) {
        sleep(3600);
    }
}

void crawler_wait_and_exit(unsigned seconds) {
    sleep(seconds);
    exit(0);
}
