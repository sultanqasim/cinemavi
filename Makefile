CC = gcc -std=gnu11
CPP = g++ -std=c++11

ARAVIS_DIR = $(HOME)/Downloads/aravis
GLIB_DIR = /opt/homebrew/Cellar/glib/2.70.2

CFLAGS = -Wall -Wextra -I$(ARAVIS_DIR)/src -I$(ARAVIS_DIR)/build/src
CFLAGS += -I$(GLIB_DIR)/include/glib-2.0 -I$(GLIB_DIR)/lib/glib-2.0/include

LFLAGS_ARV = -L$(ARAVIS_DIR)/build/src -laravis-0.8
LFLAGS_ARV += -L$(GLIB_DIR)/lib -lgobject-2.0

ifeq ($(DEBUG), 1)
CFLAGS += -fsanitize=address -g -O0
LFLAGS += -fsanitize=address
else
CFLAGS += -O3 -ffast-math
endif

BINARIES = single_capture cmraw_process cmraw_to_dng camera_calibrator

all: $(BINARIES)

LIB_OBJS = dng.opp colour_xfrm.o debayer.o convolve.o noise_reduction.o gamma.o
LIB_OBJS += pipeline.o cmraw.o auto_exposure.o cm_cli_helper.o

single_capture: single_capture.o cm_camera_helper.o $(LIB_OBJS)
	$(CPP) $(LFLAGS) $(LFLAGS_ARV) $^ -o $@

cmraw_process: cmraw_process.o $(LIB_OBJS)
	$(CPP) $(LFLAGS) $^ -o $@

cmraw_to_dng: cmraw_to_dng.o $(LIB_OBJS)
	$(CPP) $(LFLAGS) $^ -o $@

camera_calibrator: camera_calibrator.o $(LIB_OBJS)
	$(CPP) $(LFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.opp: %.cpp
	$(CPP) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(BINARIES) $(wildcard *.o) $(wildcard *.opp)
