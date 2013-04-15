#ifndef _CRAWLER_UTIL_H_
#define _CRAWLER_UTIL_H_

#include <stdlib.h>
#include <unistd.h>

#define RANDOM_INT(s)   (random() % s)
#define RANDOM_SLEEP(s) usleep((RANDOM_INT(s) * 1000000) + (RANDOM_INT(10) * 1000))

char **crawler_readfile(const char *file);
void crawler_wait_forever(void);
void crawler_wait_and_exit(unsigned seconds);

#endif /* _CRAWLER_UTIL_H_ */
