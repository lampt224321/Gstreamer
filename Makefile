CC = gcc
CFLAGS = $(shell pkg-config --cflags gstreamer-1.0 gstreamer-video-1.0)
LDFLAGS = $(shell pkg-config --libs gstreamer-1.0 gstreamer-video-1.0)

all: gstbrightnessfilter.so

gstbrightnessfilter.so: gstbrightnessfilter.o
	$(CC) -shared -o $@ $^ $(LDFLAGS)

gstbrightnessfilter.o: gstbrightnessfilter.c
	$(CC) $(CFLAGS) -fPIC -c -o $@ $<

clean:
	rm -f *.o *.so

install:
	cp gstbrightnessfilter.so $(shell pkg-config --variable=pluginsdir gstreamer-1.0)
