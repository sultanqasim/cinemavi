CC = gcc -std=gnu11
CPP = g++ -std=c++11

ARAVIS_DIR = $(HOME)/Downloads/aravis
GLIB_DIR = /opt/homebrew/Cellar/glib/2.70.2

CFLAGS = -Wall -Wextra -O3 -ffast-math -I$(ARAVIS_DIR)/src -I$(ARAVIS_DIR)/build/src
CFLAGS += -I$(GLIB_DIR)/include/glib-2.0 -I$(GLIB_DIR)/lib/glib-2.0/include
LFLAGS = -L$(ARAVIS_DIR)/build/src -laravis-0.8
LFLAGS += -L$(GLIB_DIR)/lib -lgobject-2.0

all: single_capture

single_capture: single_capture.o dng.opp colour_xfrm.o
	$(CPP) $(LFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.opp: %.cpp
	$(CPP) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f single_capture $(wildcard *.o) $(wildcard *.opp)
