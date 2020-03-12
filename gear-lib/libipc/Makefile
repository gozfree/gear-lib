##############################################################################
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
STRIP   = $(CROSS_PREFIX)strip

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
LIBNAME		= libipc
VERSION_SH	= $(shell pwd)/version.sh $(LIBNAME)
VER		= $(shell $(VERSION_SH); awk '/define\ $(LIBNAME)_version/{print $$3}' version.h)
TGT_LIB_H	= $(LIBNAME).h
TGT_LIB_H	+= libipc_stub.h
TGT_LIB_A	= $(LIBNAME).a
TGT_LIB_SO	= $(LIBNAME).so
TGT_LIB_SO_VER	= $(TGT_LIB_SO).${VER}
TGT_UNIT_TEST	= test_$(LIBNAME)
KERNEL_MODULE	= netlink_driver.ko
KERNEL_DIR	= /lib/modules/$(shell uname -r)/build

OBJS_LIB	= $(LIBNAME).o
OBJS_LIB	+= msgq_posix.o msgq_sysv.o netlink.o shm.o unix_socket.o
OBJS_UNIT_TEST	= test_$(LIBNAME).o

###############################################################################
# cflags and ldflags
###############################################################################
ifeq ($(MODE), release)
EXTRA_CFLAGS	:= -O0 -Wall -Werror 
LTYPE   := release
else
EXTRA_CFLAGS	:= -g -Wall -Werror 
LTYPE   := debug
endif
ifeq ($(OUTPUT),/usr/local)
OUTLIBPATH :=/usr/local
else
OUTLIBPATH :=$(OUTPUT)/$(LTYPE)
endif
ifneq ($(driver), y)
EXTRA_CFLAGS	+= -fPIC
endif
EXTRA_CFLAGS	+= $($(ARCH)_CFLAGS)
EXTRA_CFLAGS	+= -I$(OUTPUT)/include
EXTRA_CFLAGS	+= -I../libmacro/

SHARED	:= -shared

EXTRA_LDFLAGS	:= $($(ARCH)_LDFLAGS)
EXTRA_LDFLAGS	+= -L$(OUTLIBPATH)/lib/gear-lib -ldict -lgevent -ldarray
EXTRA_LDFLAGS	+= -pthread -lrt


###############################################################################
# target
###############################################################################
.PHONY : all clean

TGT	:=
ifneq ($(driver), y)
TGT	+= $(TGT_LIB_A)
TGT	+= $(TGT_LIB_SO)
TGT	+= $(TGT_UNIT_TEST)
else
TGT	+= driver
endif

OBJS	:= $(OBJS_LIB) $(OBJS_UNIT_TEST)

all: $(TGT)

%.o:%.c
	$(CC_V) -c $(EXTRA_CFLAGS) $< -o $@

$(TGT_LIB_A): $(OBJS_LIB)
	$(AR_V) rcs $@ $^

$(TGT_LIB_SO): $(OBJS_LIB)
	$(CC_V) -o $@ $^ $(SHARED)
	@mv $(TGT_LIB_SO) $(TGT_LIB_SO_VER)
	@ln -sf $(TGT_LIB_SO_VER) $(TGT_LIB_SO)

$(TGT_UNIT_TEST): $(OBJS_UNIT_TEST) $(ANDROID_MAIN_OBJ)
	$(CC_V) -o $@ $^ $(TGT_LIB_A) $(EXTRA_LDFLAGS)

driver: $(KERNEL_MODULE)

rmmod:
	rmmod netlink_driver
insmod:
	insmod netlink_driver.ko


$(KERNEL_MODULE):
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) modules

ifeq ($(driver), y)
obj-m	:= netlink_driver.o
endif

clean:
	$(RM_V) -f $(OBJS)
	$(RM_V) -f $(TGT)
	$(RM_V) -f version.h
	$(RM_V) -f $(TGT_LIB_SO)*
	$(RM_V) -f $(TGT_LIB_SO_VER)
	$(RM_V) -f $(KERNEL_MODULE) *.mod.c *.mod.o *.order Module*

install:
	$(MAKEDIR_OUTPUT)
	@if [ "$(MODE)" = "release" ];then $(STRIP) $(TGT); fi
	$(CP_V) -r $(TGT_LIB_H)  $(OUTPUT)/include/gear-lib
	$(CP_V) -r $(TGT_LIB_A)  $(OUTLIBPATH)/lib/gear-lib
	$(CP_V) -r $(TGT_LIB_SO) $(OUTLIBPATH)/lib/gear-lib
	$(CP_V) -r $(TGT_LIB_SO_VER) $(OUTLIBPATH)/lib/gear-lib

driver_install:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) modules_install


uninstall:
	cd $(OUTPUT)/include/gear-lib/ && rm -f $(TGT_LIB_H)
	$(RM_V) -f $(OUTLIBPATH)/lib/gear-lib/$(TGT_LIB_A)
	$(RM_V) -f $(OUTLIBPATH)/lib/gear-lib/$(TGT_LIB_SO)
	$(RM_V) -f $(OUTLIBPATH)/lib/gear-lib/$(TGT_LIB_SO_VER)
