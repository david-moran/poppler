CC=gcc
CFLAGS=-fPIC -I..
PLUGINS=libcrawlerprint.so libcrawlersocket.so

all: $(PLUGINS)

libcrawlerprint.so: crawlerprint.o
	$(CC) -shared -Wl,-soname,libcrawlerprint.so -o libcrawlerprint.so crawlerprint.o

libcrawlersocket.so: crawlersocket.o
	$(CC) -shared -Wl,-soname,libcrawlersocket.so -o libcrawlersocket.so crawlersocket.o

.PHONY: clean
clean:
	$(RM) $(PLUGINS) *.o
