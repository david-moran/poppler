#ifndef _CRAWLER_PLUGIN_H_
#define _CRAWLER_PLUGIN_H_

#include "crawler.h"

typedef int (*crawler_plugin_func)(struct crawler *,
    struct crawler_message *);

#endif /* _CRAWLER_PLUGIN_H_ */
