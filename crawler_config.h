#ifndef _CRAWLER_CONFIG_H_
#define _CRAWLER_CONFIG_H_

#include "crawler.h"

struct crawler_path **
crawler_config_path_parse(const char *file);

#endif /* _CRAWLER_CONFIG_H_ */
