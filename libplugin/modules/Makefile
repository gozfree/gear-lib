###############################################################################
# common
###############################################################################
#ARCH: linux/pi/android/ios/
ARCH		?= linux
CROSS_PREFIX	?=
OUTPUT		?= /usr/local
BUILD_DIR	:= $(shell pwd)/../build/
ARCH_INC	:= $(BUILD_DIR)/$(ARCH).inc
COLOR_INC	:= $(BUILD_DIR)/color.inc

ifeq ($(ARCH_INC), $(wildcard $(ARCH_INC)))
include $(ARCH_INC)
endif

CC	= $(CROSS_PREFIX)gcc
CXX	= $(CROSS_PREFIX)g++
LD	= $(CROSS_PREFIX)ld
AR	= $(CROSS_PREFIX)ar

ifeq ($(COLOR_INC), $(wildcard $(COLOR_INC)))
include $(COLOR_INC)
else
CC_V	= $(CC)
CXX_V	= $(CXX)
LD_V	= $(LD)
AR_V	= $(AR)
CP_V	= $(CP)
RM_V	= $(RM)
endif

###############################################################################
# target and object
###############################################################################
LIBNAME		= libplugin
VERSION_SH	= $(shell pwd)/version.sh $(LIBNAME)
VER		= $(shell $(VERSION_SH); awk '/define\ $(LIBNAME)_version/{print $$3}' version.h)
TGT_LIB_H	= $(LIBNAME).h
TGT_LIB_SO	= plugin_log.so plugin_ipc.so plugin_skt.so

OBJS_LIB	= plugin_log.o plugin_skt.o plugin_ipc.o

###############################################################################
# cflags and ldflags
###############################################################################
CFLAGS	:= -g -Wall -Werror -fPIC
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I$(OUTPUT)/include

SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS	+= -pthread -ldl

###############################################################################
# target
###############################################################################
.PHONY : all clean

TGT	:= $(TGT_LIB_SO)

OBJS	:= $(OBJS_LIB)

all: $(TGT)

%.o:%.c
	$(CC_V) -c $(CFLAGS) $< -o $@

plugin_log.so: plugin_log.o
	$(LD_V) -o $@ $^ $(SHARED)

plugin_ipc.so: plugin_ipc.o
	$(LD_V) -o $@ $^ $(SHARED)

plugin_skt.so: plugin_skt.o
	$(LD_V) -o $@ $^ $(SHARED)
clean:
	$(RM_V) -f $(OBJS)
	$(RM_V) -f $(TGT)
	$(RM_V) -f version.h
