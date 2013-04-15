#ifndef _CRAWLER_DISPATCHER_H_
#define _CRAWLER_DISPATCHER_H_

#include "crawler.h"

int crawler_dispatcher_add_module(char *name);
void crawler_dispatcher_start(struct crawler *);

#endif /* _CRAWLER_DISPATCHER_H_ */
