#include "crawler_config.h"
#include "crawler_dispatcher.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>

struct crawler_path **
crawler_config_path_parse(const char *file) {
    struct crawler_path **paths;

    JsonParser *parser;
    JsonReader *reader;
    guint i, j;
    GError *error;

    parser = json_parser_new();
    error = NULL;
    json_parser_load_from_file(parser, file, &error);
    if (error) {
        fprintf(stderr, "%s\n", error->message);
        g_error_free(error);
        g_object_unref(parser);
        return NULL;
    }

    reader = json_reader_new(json_parser_get_root(parser));
    if (!json_reader_read_member(reader, "urls")) {
        printf("urls in config not found\n");
        g_object_unref(reader);
        g_object_unref(parser);
        return NULL;
    }

    paths = NULL;
    if (json_reader_is_array(reader)) {
        i = 0;
        /* URL array element */
        while (json_reader_read_element(reader, i++)) {
            paths = (struct crawler_path **) realloc(paths,
                sizeof(struct crawler_path *) * (i + 1));
            paths[i-1] = (struct crawler_path *) 
                malloc(sizeof(struct crawler_path));
            paths[i-1]->needle = NULL;
            paths[i-1]->module.name = NULL;
            paths[i-1]->module.args = NULL;
            paths[i] = NULL;

            /* URL path*/
            if (json_reader_read_member(reader, "path")) {
                paths[i-1]->str = strdup(json_reader_get_string_value(reader));
#ifdef DEBUG
                printf("[%s:%d] paths[%d]->str => %s\n", __FILE__, __LINE__, i-1, paths[i-1]->str);
#endif
                json_reader_end_member(reader);
            }
            
            /* Keywords */
            if (json_reader_read_member(reader, "keywords")) {
                if (json_reader_is_array(reader)) {
                    j=0;
                    while (json_reader_read_element(reader, j++)) {
                        paths[i-1]->needle = (char **) 
                            realloc(paths[i-1]->needle, sizeof(char *) * (j + 1));

                        paths[i-1]->needle[j-1] = strdup(json_reader_get_string_value(reader));
                        paths[i-1]->needle[j] = NULL;
#ifdef DEBUG
                            printf("\t[%s:%d] paths[%d]->needle[%d] => %s\n",
                                __FILE__, __LINE__, i-1, j-1,
                                paths[i-1]->needle[j-1]);
#endif
                        json_reader_end_element(reader);
                    }
                }
                json_reader_end_member(reader);
            }

            /* Module */
            if (json_reader_read_member(reader, "module")) {
                /* Name */
                if (json_reader_read_member(reader, "name")) {
                    paths[i-1]->module.name = strdup(json_reader_get_string_value(reader));
#ifdef DEBUG
                    printf("\t[%s:%d] paths[%d]->module.name => %s\n",
                        __FILE__, __LINE__, i-1, paths[i-1]->module.name);
#endif
                    json_reader_end_member(reader);
                    /* If adding a new module fails */
                    if (crawler_dispatcher_add_module(paths[i-1]->module.name) == -1) {
                        /* TODO: Print a message or something else */
                    }
                }

                /* Parameters */
                paths[i-1]->module.args = NULL;
                if (json_reader_read_member(reader, "params")) {
                    if (json_reader_is_array(reader)) {
                        j = 0;
                        while (json_reader_read_element(reader, j++)) {
                            paths[i-1]->module.args = (char **) 
                                realloc(paths[i-1]->module.args, sizeof(char *) * (j + 1));
                            paths[i-1]->module.args[j-1] = 
                                strdup(json_reader_get_string_value(reader));
#ifdef DEBUG
                            printf("\t[%s:%d] paths[%d]->module.args[%d] => %s\n",
                                __FILE__, __LINE__, i-1, j-1, paths[i-1]->module.args[j-1]);
#endif
                            paths[i-1]->module.args[j] = NULL;
                            json_reader_end_element(reader);
                        }
                    }
                    json_reader_end_member(reader);
                }
                json_reader_end_member(reader);
            }

            json_reader_end_element(reader);
        }
    }

    g_object_unref(reader);
    g_object_unref(parser);

    return paths;
}
