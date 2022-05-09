#### arm.conf
CROSS_PREFIX		:= arm-linux-gnueabihf-
OUTPUT			:= `pwd`/../output/pi/

TOOLCHAINS_ROOT		:= `$(CROSS_PREFIX)gcc -print-sysroot`
PLATFORM_LIB		:= $(TOOLCHAINS_ROOT)/usr/lib
MAKEDIR_OUTPUT		:= @mkdir -p $(OUTPUT)/lib -p $(OUTPUT)/include

CC			:= $(CROSS_PREFIX)gcc
CXX			:= $(CROSS_PREFIX)g++
LD			:= $(CROSS_PREFIX)ld
AR			:= $(CROSS_PREFIX)ar
STRIP			:= $(CROSS_PREFIX)strip
