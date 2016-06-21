
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

CP	= cp
RM	= rm

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

########
TGT_APP 	= echod
OBJS_APP    	= ${TGT_APP}.o

CFLAGS	:= -g -Wall -Werror -fPIC
CFLAGS	+= -I$(OUTPUT)/include -I./

LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS	+= -pthread
LDFLAGS	+= -lgcc_s -lc
LDFLAGS += -L$(OUTPUT)/lib
LDFLAGS	+= -lgzf
LDFLAGS	+= -llog
LDFLAGS	+= -lgevent
LDFLAGS	+= -lskt
LDFLAGS	+= -ldict
LDFLAGS	+= -lworkq
LDFLAGS	+= -losal
LDFLAGS	+= -lthread

.PHONY : all clean

TGT	:= $(TGT_APP)

OBJS	:= $(OBJS_APP)

all: $(TGT)

%.o:%.c
	$(CC_V) -c $(CFLAGS) $< -o $@

$(TGT_APP): $(OBJS)
	$(CC_V) -o $@ $^ $(LDFLAGS)

clean:
	$(RM_V) -f $(OBJS)
	$(RM_V) -f $(TGT)

install:
	$(CP_V) -f $(TGT_APP) ${OUTPUT}/bin

uninstall:
	$(RM_V) -f ${OUTPUT}/bin/$(TGT_APP)
