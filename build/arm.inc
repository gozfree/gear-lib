#### arm.conf
CROSS_PREFIX		:= arm-linux-gnueabihf-
OUTPUT			:= `pwd`/../output/arm/

TOOLCHAINS_ROOT		:= `$(CROSS_PREFIX)gcc -print-sysroot`
PLATFORM_LIB		:= $(TOOLCHAINS_ROOT)/usr/lib
MAKEDIR_OUTPUT		:= @mkdir -p $(OUTPUT)/lib -p $(OUTPUT)/include
