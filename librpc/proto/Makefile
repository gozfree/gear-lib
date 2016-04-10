###############################################################################
#  Copyright (C) 2014-2015
#  file:    Makefile
#  author:  gozfree <gozfree@163.com>
#  created: 2016-04-10 16:26
# updated: 2016-04-10 16:26
###############################################################################

###############################################################################
# common
###############################################################################
#ARCH: linux/pi/android/ios/
ARCH		?= linux
CROSS_PREFIX	?=
OUTPUT		?= /usr/local
BUILD_DIR	:= $(shell pwd)/../../build/
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
TGT_LIB_A	= ../librpc.a
TGT_UNIT_TEST	= test_proto
SRC	:= $(wildcard *.c)
SRC	+= $(wildcard *.cc)
SRC	+= $(wildcard *.cpp)

OBJS	:= $(patsubst %.c,%.o,$(SRC))
OBJS	+= $(patsubst %.cc,%.o,$(SRC))
OBJS	+= $(patsubst %.cpp,%.o,$(SRC))



###############################################################################
# cflags and ldflags
###############################################################################
CFLAGS	:= -g -Wall -Werror
CFLAGS  += -I../

LDFLAGS	:= -lpthread
LDFLAGS	+= -lprotobuf
LDFLAGS	+= -llog -lgevent -lskt -lthread -ldict

###############################################################################
# target
###############################################################################
.PHONY : all clean

TGT	:= $(TGT_UNIT_TEST)

OBJS_UNIT_TEST	= proto_librpc.o ../librpc.o \
                  librpc.pb.cc \
                  hello.pb.cc

all: autogen $(TGT)

autogen:
ifneq (librpc.pb.h, $(wildcard librpc.pb.h))
	protoc -I=./ --cpp_out=./ \
		./librpc.proto \
		./hello.proto
endif

%.o:%.cc
	$(CXX_V) -c $(CFLAGS) $< -o $@

%.o:%.cpp
	$(CXX_V) -c $(CFLAGS) $< -o $@


$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST)
	$(CXX_V) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

install:

clean:
	$(RM_V) -f *.o *.pb.h *.pb.cc
	$(RM_V) -f $(TGT)
