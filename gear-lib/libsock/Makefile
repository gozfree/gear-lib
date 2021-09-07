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

include $(ARCH_INC)
include $(COLOR_INC)

CC_V		?= $(CC)
CXX_V		?= $(CXX)
LD_V		?= $(LD)
AR_V		?= $(AR)
CP_V		?= $(CP)
RM_V		?= $(RM)

###############################################################################
# target and object
###############################################################################
ENABLE_SOCK_EXT = 1
ENABLE_PTCP = 0
LIBNAME		= libsock
VER_TAG		= $(shell echo ${LIBNAME} | tr 'a-z' 'A-Z')
VER		= $(shell awk '/'"${VER_TAG}_VERSION"'/{print $$3}' ${LIBNAME}.h)
TGT_LIB_H	= $(LIBNAME).h
ifeq ($(ENABLE_SOCK_EXT), 1)
TGT_LIB_H	+= libsock_ext.h
endif
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_LIB_SO_VER	= $(TGT_LIB_SO).${VER}
TGT_UNIT_TEST	= test_$(LIBNAME)

OBJS_LIB	= $(LIBNAME).o
ifeq ($(ENABLE_SOCK_EXT), 1)
OBJS_LIB	+= libsock_ext.o
endif
OBJS_UNIT_TEST	= test_$(LIBNAME).o

###############################################################################
# cflags and ldflags
###############################################################################
ifeq ($(MODE), release)
CFLAGS  := -O2 -Wall -Werror -fPIC
LTYPE   := release
else
CFLAGS  := -g -Wall -Werror -fPIC
LTYPE   := debug
endif
ifeq ($(OUTPUT),/usr/local)
OUTLIBPATH :=/usr/local
else
OUTLIBPATH :=$(OUTPUT)/$(LTYPE)
endif

#CFLAGS  += -fsanitize=address -fno-omit-frame-pointer -static-libasan
CFLAGS	+= $($(ARCH)_CFLAGS)
CFLAGS	+= -I$(OUTPUT)/include/gear-lib
ifeq ($(ENABLE_SOCK_EXT), 1)
CFLAGS	+= -DENABLE_SOCK_EXT
endif
ifeq ($(ENABLE_PTCP), 1)
CFLAGS	+= -DENABLE_PTCP
endif
SHARED	:= -shared

LDFLAGS	:= $($(ARCH)_LDFLAGS)
LDFLAGS	+= -pthread
ifeq ($(ENABLE_SOCK_EXT), 1)
LDFLAGS	+= -L$(OUTLIBPATH)/lib/gear-lib -lgevent -lthread
endif
ifeq ($(ENABLE_PTCP), 1)
LDFLAGS	+= -L$(OUTLIBPATH)/lib/gear-lib -lptcp
endif

#LDFLAGS += -fsanitize=address -static-libasan

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
	$(CC_V) -o $@ $^ $(TGT_LIB_A) $(LDFLAGS) -L$(OUTLIBPATH)/lib/gear-lib -lgevent -lthread -ldarray

clean:
	$(RM_V) -f $(OBJS)
	$(RM_V) -f $(TGT)
	$(RM_V) -f version.h
	$(RM_V) -f $(TGT_LIB_SO)*
	$(RM_V) -f $(TGT_LIB_SO_VER)

install:
	$(MAKEDIR_OUTPUT)
	@if [ "$(MODE)" = "release" ];then $(STRIP) $(TGT); fi
	$(CP_V) -r $(TGT_LIB_H)  $(OUTPUT)/include/gear-lib
	$(CP_V) -r $(TGT_LIB_A)  $(OUTLIBPATH)/lib/gear-lib
	$(CP_V) -r $(TGT_LIB_SO) $(OUTLIBPATH)/lib/gear-lib
	$(CP_V) -r $(TGT_LIB_SO_VER) $(OUTLIBPATH)/lib/gear-lib

uninstall:
	cd $(OUTPUT)/include/gear-lib && rm -f $(TGT_LIB_H)
	$(RM_V) -f $(OUTLIBPATH)/lib/gear-lib/$(TGT_LIB_A)
	$(RM_V) -f $(OUTLIBPATH)/lib/gear-lib/$(TGT_LIB_SO)
	$(RM_V) -f $(OUTLIBPATH)/lib/gear-lib/$(TGT_LIB_SO_VER)
