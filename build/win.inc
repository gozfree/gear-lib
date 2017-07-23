#### x86.conf
CROSS_PREFIX		:= x86_64-w64-mingw32-
#OUTPUT			:= `pwd`/../output/win/

TOOLCHAINS_ROOT		:= `$(CROSS_PREFIX)gcc -print-search-dirs | grep programs | awk -F=  '{print $2}'`
PLATFORM_LIB		:= $(TOOLCHAINS_ROOT)/usr/lib
MAKEDIR_OUTPUT		:=
INC_DIR			:= /usr/x86_64-w64-mingw32/include
LIB_DIR			:= /usr/x86_64-w64-mingw32/lib
