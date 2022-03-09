CC = gcc -std=gnu11
CPP = g++ -std=c++11

CFLAGS = -Wall -Wextra

ifeq ($(OS),Windows_NT)
    PLATFORM = Windows
else
    PLATFORM = $(shell sh -c 'uname 2>/dev/null || echo Unknown')
endif

ifeq ($(PLATFORM), Darwin)
    DEPS_DIR = /opt/homebrew
else
    DEPS_DIR = /usr
endif

CFLAGS += -I$(DEPS_DIR)/include/aravis-0.8
CFLAGS += -I$(DEPS_DIR)/include/glib-2.0 -I$(DEPS_DIR)/lib/glib-2.0/include

LFLAGS_ARV += -L$(DEPS_DIR)/lib -lgobject-2.0 -laravis-0.8

ifeq ($(DEBUG), 1)
    CFLAGS += -fsanitize=address -g -O0
    LFLAGS += -fsanitize=address
else
    CFLAGS += -O3 -ffast-math
endif

BINARIES = single_capture cmraw_process cmraw_to_dng camera_calibrator

all: $(BINARIES)

LIB_OBJS = dng.opp colour_xfrm.o debayer.o convolve.o noise_reduction.o gamma.o
LIB_OBJS += pipeline.o cmraw.o auto_exposure.o cm_cli_helper.o cm_calibrations.o

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
