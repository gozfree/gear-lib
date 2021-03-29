#### linux
CROSS_PREFIX		:=
#OUTPUT			:= `pwd`/../output/linux/

TOOLCHAINS_ROOT		:= `$(CROSS_PREFIX)gcc -print-sysroot`
PLATFORM_LIB		:= $(TOOLCHAINS_ROOT)/usr/lib
MAKEDIR_OUTPUT		:=

CC			:= $(CROSS_PREFIX)gcc
CXX			:= $(CROSS_PREFIX)g++
LD			:= $(CROSS_PREFIX)ld
AR			:= $(CROSS_PREFIX)ar
STRIP			:= $(CROSS_PREFIX)strip

#static asan
#linux_LDFLAGS		:= -fsanitize=address -static-libasan

#shared asan
#run cmd on shell: `export LD_PRELOAD=libasan.so.4`
#linux_CFLAGS		:= -fsanitize=address
#linux_LDFLAGS		:= -lasan
