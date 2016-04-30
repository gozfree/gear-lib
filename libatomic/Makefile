###############################################################################
#  Copyright (C) 2014-2015
#  file:    Makefile
#  author:  gozfree <gozfree@163.com>
#  created: 2016-04-24 16:22:04
#  updated: 2016-04-24 16:22:04
###############################################################################

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
LIBNAME		= libatomic
VERSION_SH	= $(shell pwd)/version.sh $(LIBNAME)
VER		= $(shell $(VERSION_SH); awk '/define\ $(LIBNAME)_version/{print $$3}' version.h)
TGT_LIB_H	= $(LIBNAME).h
TGT_LIB_H	+= libatomic_gcc.h
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_LIB_SO_VER	= $(TGT_LIB_SO).${VER}
TGT_UNIT_TEST	= test_$(LIBNAME)

OBJS_LIB	= $(LIBNAME).o
OBJS_UNIT_TEST	= test_$(LIBNAME).o

###############################################################################
# cflags and ldflags
###############################################################################
CFLAGS	:= -g -Wall -Werror -fPIC
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I$(OUTPUT)/include

SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS	+= -pthread

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
	$(LD_V) -o $@ $^ $(SHARED)
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
	$(MAKEDIR_OUTPUT)
	$(CP_V) -r $(TGT_LIB_H)  $(OUTPUT)/include
	$(CP_V) -r $(TGT_LIB_A)  $(OUTPUT)/lib
	$(CP_V) -r $(TGT_LIB_SO) $(OUTPUT)/lib
	$(CP_V) -r $(TGT_LIB_SO_VER) $(OUTPUT)/lib

uninstall:
	$(RM_V) -f $(OUTPUT)/include/$(TGT_LIB_H)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_A)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_SO)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_SO_VER)
