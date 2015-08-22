#### x86.conf
CROSS_PREFIX		:=
#OUTPUT			:= `pwd`/../output/x86/

TOOLCHAINS_ROOT		:= `$(CROSS_PREFIX)gcc -print-sysroot`
PLATFORM_LIB		:= $(TOOLCHAINS_ROOT)/usr/lib
MAKEDIR_OUTPUT		:=
