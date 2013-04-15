CC=gcc
CFLAGS=-lpthread -g -Wall -lpthread `pkg-config --cflags json-glib-1.0` -D_GNU_SOURCE -I .
#CFLAGS=-DDEBUG -lpthread -g -Wall -lpthread `pkg-config --cflags json-glib-1.0` -D_GNU_SOURCE -I .
CLIBS=`pkg-config --libs json-glib-1.0` -ldl
OBJECTS=crawler.o crawler_util.o crawler_dispatcher.o crawler_config.o main.o

poppler: $(OBJECTS)
	$(MAKE) -C plugins
	$(MAKE) -C plugins/nodejs-dispatcher
	$(CC) $(OBJECTS) $(CFLAGS) $(CLIBS) -o poppler

.PHONY: clean
clean:
	$(MAKE) -C plugins clean
	$(MAKE) -C plugins/nodejs-dispatcher clean
	$(RM) $(OBJECTS) poppler
