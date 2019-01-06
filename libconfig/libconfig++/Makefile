###############################################################################
#  Copyright (C) 2014-2015
#  file:    Makefile
#  author:  gozfree <gozfree@163.com>
#  created: 2017-03-30 15:00:24
#  updated: 2017-03-30 15:00:24
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
LIBNAME		= libgconfig
VERSION_SH	= $(shell pwd)/version.sh $(LIBNAME)
VER		= $(shell $(VERSION_SH); awk '/define\ $(LIBNAME)_version/{print $$3}' version.h)
TGT_LIB_H	= $(LIBNAME).h \
		  lua/lua_config.h lua/luatables.h lua/utils/serialize.lua.h \
		  json/json_config.h
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_LIB_SO_VER	= $(TGT_LIB_SO).${VER}
TGT_UNIT_TEST	= test_$(LIBNAME)

OBJS_LUA	= lua/luatables.o \
		  lua/lua_config.o

OBJS_JSON	= json/json_config.o


OBJS_LIB	= $(LIBNAME).o \
		  $(OBJS_LUA) \
		  $(OBJS_JSON)

OBJS_UNIT_TEST	= test_$(LIBNAME).o

###############################################################################
# cflags and ldflags
###############################################################################
CFLAGS_LUA	= `pkg-config --cflags lua5.3`
CFLAGS_JSON	= `pkg-config --cflags jsoncpp`
CFLAGS	:= -g -Wall -Werror -fPIC
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I$(OUTPUT)/include
CFLAGS	+= $(CFLAGS_LUA)
CFLAGS	+= -std=c++11

SHARED	:= -shared

LDFLAGS_LUA	= `pkg-config --libs lua5.3`
LDFLAGS_JSON	= `pkg-config --libs jsoncpp`
LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS += -L$(OUTPUT)/lib -llog
LDFLAGS	+= -lpthread
LDFLAGS	+= $(LDFLAGS_LUA)
LDFLAGS	+= $(LDFLAGS_JSON)

###############################################################################
# target
###############################################################################
.PHONY : all clean

TGT	:= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_UNIT_TEST)

OBJS	:= $(OBJS_LIB) $(OBJS_UNIT_TEST)

all: $(TGT)

%.o:%.cpp
	$(CXX_V) -c $(CFLAGS) $< -o $@

%.o:%.cc
	$(CXX_V) -c $(CFLAGS) $< -o $@

%.o:%.c
	$(CC_V) -c $(CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	$(AR_V) rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(CXX_V) -o $@ $^ $(SHARED)
	@mv $(TGT_LIB_SO) $(TGT_LIB_SO_VER)
	@ln -sf $(TGT_LIB_SO_VER) $(TGT_LIB_SO)

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST) $(ANDROID_MAIN_OBJ)
	$(CXX_V) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	$(RM_V) -f $(OBJS)
	$(RM_V) -f $(TGT)
	$(RM_V) -f version.h
	$(RM_V) -f $(TGT_LIB_SO)*
	$(RM_V) -f $(TGT_LIB_SO_VER)

install:
	$(MAKEDIR_OUTPUT)
	mkdir -p $(OUTPUT)/include/libgconfig
	$(CP_V) -dpRf --parents $(TGT_LIB_H)  $(OUTPUT)/include/libgconfig
	$(CP_V) -r $(TGT_LIB_A)  $(OUTPUT)/lib
	$(CP_V) -r $(TGT_LIB_SO) $(OUTPUT)/lib
	$(CP_V) -r $(TGT_LIB_SO_VER) $(OUTPUT)/lib

uninstall:
	$(RM_V) -f $(OUTPUT)/include/libgconfig/$(TGT_LIB_H)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_A)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_SO)
	$(RM_V) -f $(OUTPUT)/lib/$(TGT_LIB_SO_VER)
