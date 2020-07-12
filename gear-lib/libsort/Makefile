
#ARCH: linux/arm/android/ios/win
ARCH		?= linux
OUTPUT		?= /usr/local
BUILD_DIR	:= $(shell pwd)/../../build/
ARCH_INC	:= $(BUILD_DIR)/$(ARCH).inc
COLOR_INC	:= $(BUILD_DIR)/color.inc

include $(ARCH_INC)
include $(COLOR_INC)

CC_V		?= $(CC)
CXX_V		?= $(CXX)
LD_V		?= $(LD)
AR_V		?= $(AR)
CP_V		?= $(CP)
RM_V		?= $(RM)


LIBNAME		= libsort
VERSION_SH	= $(shell pwd)/version.sh $(LIBNAME)
TGT_LIB_H	= $(LIBNAME).h
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_LIB_SO_VER	= $(TGT_LIB_SO).${VER}
TGT_UNIT_TEST	= test_$(LIBNAME)

OBJS_LIB	= $(LIBNAME).o
OBJS_LIB	+= bubble_sort.o
OBJS_LIB	+= heap_sort.o
OBJS_LIB	+= quick_sort.o
OBJS_LIB	+= select_sort.o

OBJS_UNIT_TEST	= test_$(LIBNAME).o

ifeq ($(MODE), release)
CFLAGS	:= -O2 -Wall -Werror -fPIC
LTYPE   := release
else
CFLAGS	:= -g -Wall -Werror -fPIC
LTYPE   := debug
endif
ifeq ($(OUTPUT),/usr/local)
OUTLIBPATH :=/usr/local
else
OUTLIBPATH :=$(OUTPUT)/$(LTYPE)
endif
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I$(OUTPUT)/include

SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS)

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

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST) $(ANDROID_MAIN_OBJ)
	$(CC_V) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS)

clean:
	$(RM_V) -f $(OBJS)
	$(RM_V) -f $(TGT)

install:
	$(MAKEDIR_OUTPUT)
	if [ "$(MODE)" = "release" ];then $(STRIP) $(TGT); fi
	$(CP_V) -r $(TGT_LIB_H)  $(OUTPUT)/include/gear-lib
	$(CP_V) -r $(TGT_LIB_A)  $(OUTLIBPATH)/lib/gear-lib
	$(CP_V) -r $(TGT_LIB_SO) $(OUTLIBPATH)/lib/gear-lib

uninstall:
	cd $(OUTPUT)/include/gear-lib/ && rm -f $(TGT_LIB_H)
	$(RM_V) -f $(OUTLIBPATH)/lib/gear-lib/$(TGT_LIB_A)
	$(RM_V) -f $(OUTLIBPATH)/lib/gear-lib/$(TGT_LIB_SO)
	$(RM_V) -f $(OUTLIBPATH)/lib/gear-lib/$(TGT_LIB_SO_VER)
