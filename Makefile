CPP = g++

ARAVIS_DIR = $(HOME)/Downloads/aravis
GLIB_DIR = /opt/homebrew/Cellar/glib/2.70.2

CFLAGS = -Wall -Wextra -std=c++11 -I$(ARAVIS_DIR)/src -I$(ARAVIS_DIR)/build/src
CFLAGS += -I$(GLIB_DIR)/include/glib-2.0 -I$(GLIB_DIR)/lib/glib-2.0/include
LFLAGS = -L$(ARAVIS_DIR)/build/src -laravis-0.8
LFLAGS += -L$(GLIB_DIR)/lib -lgobject-2.0

all: single_capture

single_capture: single_capture.cpp
	$(CPP) $(CFLAGS) $(LFLAGS) $< -o $@

.PHONY: clean
clean:
	rm single_capture
