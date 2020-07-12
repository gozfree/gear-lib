#### android
CROSS_PREFIX		:= arm-linux-androideabi-
OUTPUT			:= `pwd`/../output/android/

TOOLCHAINS_ROOT		:= $(ANDROID_NDK)/toolchains/arm-linux-androideabi-4.6/prebuilt/linux-x86_64
TOOLCHAINS_PREFIX	:= $(TOOLCHAINS_ROOT)/bin/arm-linux-androideabi
TOOLCHAINS_INCLUDE	:= $(TOOLCHAINS_ROOT)/lib/gcc/arm-linux-androideabi/4.6/include
TOOLCHAINS_INCLUDE_FIXED:= $(TOOLCHAINS_ROOT)/lib/gcc/arm-linux-androideabi/4.6/include-fixed
STL_PATH		:= $(ANDROID_NDK)/sources/cxx-stl/gnu-libstdc++/4.6/libs/armeabi-v7a
PLATFORM_ROOT		:= $(ANDROID_NDK)/platforms/android-18/arch-arm
PLATFORM_INCLUDE	:= $(PLATFORM_ROOT)/usr/include
PLATFORM_LIB		:= $(PLATFORM_ROOT)/usr/lib
ANDROID_INC_PATH	:= -I$(TOOLCHAINS_INCLUDE) \
			   -I$(TOOLCHAINS_INCLUDE_FIXED) \
			   -I$(PLATFORM_INCLUDE)

ANDROID_MAIN_OBJ	:= $(PLATFORM_LIB)/crtbegin_dynamic.o \
			   $(PLATFORM_LIB)/crtend_android.o

#__ANDROID_APP__ is binary file
#__ANDROID_APK__ is apk file
ANDROID_CFLAGS		:= $(ANDROID_INC_PATH) \
			   -D__ANDROID_APP__ \
			   -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables

ANDROID_LDFLAGS		:= -L$(PLATFORM_LIB) \
			   -L$(STL_PATH)/thumb/ -lgnustl_static \
			   -L$(STL_PATH) -lsupc++ -nostdlib -Bdynamic -lstdc++ -lc -lgcc

MAKEDIR_OUTPUT		:= @mkdir -p $(OUTPUT)/include -p $(OUTPUT)/lib
android_CFLAGS		:= $(ANDROID_CFLAGS)
android_LDFLAGS		:= $(ANDROID_LDFLAGS)

CC			:= $(CROSS_PREFIX)gcc
CXX			:= $(CROSS_PREFIX)g++
LD			:= $(CROSS_PREFIX)ld
AR			:= $(CROSS_PREFIX)ar
STRIP			:= $(CROSS_PREFIX)strip
