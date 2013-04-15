#include "crawler_dispatcher.h"
#include "plugins/crawler_plugin.h"

#include <pthread.h>
#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>

#define DISPATCH_SLEEP  1000
#define MAX_MODULES     32 
#define MODULE_BASE     "plugins"
#define MODULE_PATH_SZ  1024

struct plugin {
    void *dlhandle;
    crawler_plugin_func f;
};

static unsigned first_module = 0;
static struct hsearch_data hsearch_data;

static void *dispatcher_start(void *);
static void crawler_dispatcher_modules_free();

void crawler_dispatcher_start(struct crawler *crawler) {
    pthread_t thid;
    pthread_create(&thid, NULL, dispatcher_start, crawler);
    pthread_detach(thid);
}

static void *dispatcher_start(void *user) {
    struct crawler *crawler = (struct crawler *) user;
    struct crawler_message *message;
    ENTRY item, *retval;
    char *module;
    struct plugin *plugin;

    while (1) {
        while ((message = crawler_message_pop(crawler)) != NULL) {
            module = crawler->path[message->path_ndx]->module.name;
            if (module != NULL) {
                item.key = module;                
                if(hsearch_r(item, FIND, &retval, 
                    &hsearch_data) != 0) {
                    plugin = (struct plugin *) retval->data;
#ifdef DEBUG
                    printf("[%s:%d] Calling module %s:%p\n", __FILE__, __LINE__, 
                        module, plugin->f);
#endif
                    plugin->f(crawler, message);
                }
            }
            free(message);
        }
        usleep(DISPATCH_SLEEP);
    }
    
    crawler_dispatcher_modules_free();
    hdestroy_r(&hsearch_data);
    return NULL;
}

int crawler_dispatcher_add_module(char *name) {
    ENTRY item, *retval;
    struct plugin *plugin;
    char buf[MODULE_PATH_SZ], *err;

    if (first_module == 0) {
        memset(&hsearch_data, 0, sizeof(struct hsearch_data));
        hcreate_r(MAX_MODULES, &hsearch_data);
        first_module = 1;
    }
    item.key = name;

    /* If module not found */
    hsearch_r(item, FIND, &retval, &hsearch_data);
    if(retval == NULL) {
        plugin = (struct plugin *) malloc(sizeof(struct plugin));

        /* Load library */
        snprintf(buf, MODULE_PATH_SZ, "./%s/lib%s.so", MODULE_BASE, name);
        plugin->dlhandle = dlopen(buf, RTLD_LAZY);
        if ((err = dlerror()) != NULL) {
            fprintf(stderr, "%s\n", err);
            free(plugin);
            return -1;
        }

        /* Search the symbol */
        plugin->f = (crawler_plugin_func) dlsym(plugin->dlhandle, name);
        if ((err = dlerror()) != NULL) {
            fprintf(stderr, "%s\n", err);
            dlclose(plugin->dlhandle);
            free(plugin);
            return -1;
        }
#ifdef DEBUG
        printf("[%s:%d] Symbol %s found at %s:%p\n", __FILE__, __LINE__, 
            name, buf, plugin->f);
#endif

        /* Add to the hash */
        item.key = name;
        item.data = plugin;
        if (hsearch_r(item, ENTER, &retval, &hsearch_data) == 0) {
            fprintf(stderr, "Error inserting module in hash table");
            dlclose(plugin->dlhandle);
            free(plugin);
            return -1;
        }
    }

    return 0;
}

/* TODO: Dynamic library loading cleanup */
static void crawler_dispatcher_modules_free() {
}
