###############################################################################
#  Copyright (C) 2014-2015
#  file:    Makefile
#  author:  gozfree <gozfree@163.com>
#  created: 2016-07-21 15:12
#  updated: 2016-07-21 15:12
###############################################################################

###############################################################################
# common
###############################################################################
#ARCH: linux/pi/android/ios/
ARCH		?= linux
CROSS_PREFIX	?=
OUTPUT		?= /usr/local
BUILD_DIR	:= $(shell pwd)/build/
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
LIBNAME		= liblightlib
VERSION_SH	= $(shell pwd)/version.sh $(LIBNAME)
VER		= $(shell $(VERSION_SH); awk '/define\ $(LIBNAME)_version/{print $$3}' version.h)
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_LIB_SO_VER	= $(TGT_LIB_SO).${VER}
TGT_UNIT_TEST	= test_$(LIBNAME)

TGT_LIB_H	= $(LIBNAME).h \
		  libatomic/libatomic.h \
		  libatomic/libatomic_gcc.h \
		  libdebug/libdebug.h \
		  libdict/libdict.h \
		  libfile/libfile.h \
		  libgevent/libgevent.h \
		  libhash/libhash.h \
		  libipc/libipc.h \
		  liblock/liblock.h \
		  liblog/liblog.h \
		  libmacro/libmacro.h \
		  libmacro/kernel_list.h \
		  libosal/libosal.h \
		  librbtree/librbtree.h \
		  libringbuffer/libringbuffer.h \
		  librpc/librpc.h \
		  libskt/libskt.h \
		  libsort/libsort.h \
		  libthread/libthread.h \
		  libtime/libtime.h \
		  libvector/libvector.h \
		  libworkq/libworkq.h

LIBATOMIC_O	= libatomic/libatomic.o
LIBDEBUG_O	= libdebug/libdebug.o
LIBDICT_O	= libdict/libdict.o
LIBFILE_O	= libfile/libfile.o \
		  libfile/io.o \
		  libfile/fio.o
LIBGEVENT_O	= libgevent/libgevent.o \
		  libgevent/epoll.o \
		  libgevent/poll.o \
		  libgevent/select.o
LIBHASH_O	= libhash/libhash.o
LIBIPC_O	= libipc/libipc.o \
		  libipc/msgq_posix.o \
		  libipc/msgq_sysv.o \
		  libipc/netlink.o \
		  libipc/shm.o
LIBLOCK_O	= liblock/liblock.o
LIBLOG_O	= liblog/liblog.o
LIBMACRO_O	= libmacro/libmacro.o
LIBOSAL_O	= libosal/libosal.o
LIBRBTREE_O	= librbtree/librbtree.o
LIBRINGBUFFER_O	= libringbuffer/libringbuffer.o
LIBRPC_O	= librpc/librpc.o
LIBSKT_O	= libskt/libskt.o
LIBSORT_O	= libsort/libsort.o
LIBTHREAD_O	= libthread/libthread.o
LIBTIME_O	= libtime/libtime.o
LIBVECTOR_O	= libvector/libvector.o
LIBWORKQ_O	= libworkq/libworkq.o


OBJS_LIB	= $(LIBATOMIC_O) \
		  $(LIBDEBUG_O) \
		  $(LIBDICT_O) \
		  $(LIBFILE_O) \
		  $(LIBGEVENT_O) \
		  $(LIBHASH_O) \
		  $(LIBIPC_O) \
		  $(LIBLOCK_O) \
		  $(LIBLOG_O) \
		  $(LIBMACRO_O) \
		  $(LIBOSAL_O) \
		  $(LIBRBTREE_O) \
		  $(LIBRINGBUFFER_O) \
		  $(LIBRPC_O) \
		  $(LIBSKT_O) \
		  $(LIBSORT_O) \
		  $(LIBTHREAD_O) \
		  $(LIBTIME_O) \
		  $(LIBVECTOR_O) \
		  $(LIBWORKQ_O)

OBJS_UNIT_TEST	= test_$(LIBNAME).o

###############################################################################
# cflags and ldflags
###############################################################################
CFLAGS	:= -g -Wall -Werror -fPIC
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I./ \
	   -I./libatomic \
	   -I./libdebug \
	   -I./libdict \
	   -I./libfile \
	   -I./libgevent \
	   -I./libhash \
	   -I./libipc \
	   -I./liblock \
	   -I./liblog \
	   -I./libmacro \
	   -I./libosal \
	   -I./librbtree \
	   -I./libringbuffer \
	   -I./librpc \
	   -I./libskt \
	   -I./libsort \
	   -I./libthread \
	   -I./libtime \
	   -I./libvector \
	   -I./libworkq

SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS	+= -pthread -ldl -lrt

###############################################################################
# target
###############################################################################
.PHONY : all clean

TGT	:= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_UNIT_TEST)

OBJS	:= $(OBJS_LIB) $(OBJS_UNIT_TEST)

all: $(TGT)

%.o:%.c
	$(CC_V) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	$(AR_V) rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(CC_V) -o $@ $^ $(SHARED)
	@mv $(TGT_LIB_SO) $(TGT_LIB_SO_VER)
	@ln -sf $(TGT_LIB_SO_VER) $(TGT_LIB_SO)

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST) $(ANDROID_MAIN_OBJ)
	$(CC_V) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	$(RM_V) -f $(OBJS)
	$(RM_V) -f $(TGT)
	$(RM_V) -f version.h
	$(RM_V) -f $(TGT_LIB_SO)*
	$(RM_V) -f $(TGT_LIB_SO_VER)

install:
	$(CP_V) -r $(TGT_LIB_H)  $(OUTPUT)/include/
	$(CP_V) -r $(TGT_LIB_A)  $(OUTPUT)/lib/
	$(CP_V) -r $(TGT_LIB_SO) $(OUTPUT)/lib/
	$(CP_V) -r $(TGT_LIB_SO_VER) $(OUTPUT)/lib/

uninstall:
	$(RM_V) -f $(OUTPUT)/include/$(LIBNAME)/$(TGT_LIB_H)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_A)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_SO)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_SO_VER)
