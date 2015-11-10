#!/bin/bash

CMD=$0
MODULE=$1

MAKEFILE=Makefile
LIBFOO_C=${MODULE}.c
LIBFOO_H=${MODULE}.h
TEST_LIBFOO_C=test_${MODULE}.c
README_MD=README.md
ANDROID_MK=Android.mk

LIBFOO_MACRO=`echo ${MODULE} | tr 'a-z' 'A-Z'`

DATE=$(date '+%Y-%m-%d %H:%M:%S')
S='$'

usage()
{
	echo "==== usage ===="
	echo "$CMD <module>"
	echo "auto generate c code and Makefile of <module>"
}

mkdir_libfoo()
{
	mkdir ${MODULE}
	cd ${MODULE}
}

autogen_libfoo_h()
{
cat > ${LIBFOO_H} <<!
/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    ${LIBFOO_H}
 * author:  gozfree <gozfree@163.com>
 * created: ${DATE}
 * updated: ${DATE}
 *****************************************************************************/
#ifndef _${LIBFOO_MACRO}_H_
#define _${LIBFOO_MACRO}_H_

#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif
#endif
!
}

autogen_libfoo_c()
{
cat > ${LIBFOO_C} <<!
/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    ${LIBFOO_C}
 * author:  gozfree <gozfree@163.com>
 * created: ${DATE}
 * updated: ${DATE}
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <${LIBFOO_H}>

!
}

autogen_test_libfoo_c()
{
cat > ${TEST_LIBFOO_C} <<!
/******************************************************************************
 * Copyright (C) 2014-2015
 * file:    ${TEST_LIBFOO_C}
 * author:  gozfree <gozfree@163.com>
 * created: ${DATE}
 * updated: ${DATE}
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <${LIBFOO_H}>

int main(int argc, char **argv)
{
    return 0;
}
!
}


autogen_makefile()
{
cat > ${MAKEFILE} <<!
###############################################################################
## Copyright (C) 2014-2015
## file:    ${MAKEFILE}
## author:  gozfree <gozfree@163.com>
## created: ${DATE}
## updated: ${DATE}
###############################################################################

#ARCH: linux/pi/android/ios/
ARCH		?= linux
CROSS_PREFIX	?=
OUTPUT		?= /usr/local
BUILD_DIR	:= ${S}(shell pwd)/../build/
ARCH_INC	:= ${S}(BUILD_DIR)/${S}(ARCH).inc
COLOR_INC	:= ${S}(BUILD_DIR)/color.inc

ifeq (${S}(ARCH_INC), ${S}(wildcard ${S}(ARCH_INC)))
include ${S}(ARCH_INC)
endif

CC	= ${S}(CROSS_PREFIX)gcc
CXX	= ${S}(CROSS_PREFIX)g++
LD	= ${S}(CROSS_PREFIX)ld
AR	= ${S}(CROSS_PREFIX)ar

ifeq (${S}(COLOR_INC), ${S}(wildcard ${S}(COLOR_INC)))
include ${S}(COLOR_INC)
else
CC_V	= ${S}(CC)
CXX_V	= ${S}(CXX)
LD_V	= ${S}(LD)
AR_V	= ${S}(AR)
CP_V	= ${S}(CP)
RM_V	= ${S}(RM)
endif

########
LIBNAME		= ${MODULE}
TGT_LIB_H	= ${S}(LIBNAME).h
TGT_LIB_A	= ${S}(LIBNAME).a
TGT_LIB_SO	= ${S}(LIBNAME).so
TGT_UNIT_TEST	= test_${S}(LIBNAME)

OBJS_LIB	= ${S}(LIBNAME).o
OBJS_UNIT_TEST	= test_${S}(LIBNAME).o

CFLAGS	:= -g -Wall -fPIC
CFLAGS	+= ${S}(${S}(ARCH)_CFLAGS)
CFLAGS	+= -I${S}(OUTPUT)/include

SHARED	:= -shared

LDFLAGS	:= ${S}(${S}(ARCH)_LDFLAGS)
LDFLAGS	+= -pthread

.PHONY : all clean

TGT	:= ${S}(TGT_LIB_A)
TGT	+= ${S}(TGT_LIB_SO)
TGT	+= ${S}(TGT_UNIT_TEST)

OBJS	:= ${S}(OBJS_LIB) ${S}(OBJS_UNIT_TEST)

all: ${S}(TGT)

%.o:%.c
	${S}(CC_V) -c ${S}(CFLAGS) $< -o ${S}@

${S}(TGT_LIB_A): ${S}(OBJS_LIB)
	${S}(AR_V) rcs ${S}@ $^

${S}(TGT_LIB_SO): ${S}(OBJS_LIB)
	${S}(LD_V) -o ${S}@ $^ ${S}(SHARED)

${S}(TGT_UNIT_TEST): ${S}(OBJS_UNIT_TEST) ${S}(ANDROID_MAIN_OBJ)
	${S}(CC_V) -o ${S}@ $^ ${S}(TGT_LIB_A) ${S}(LDFLAGS)

clean:
	${S}(RM_V) -f ${S}(OBJS)
	${S}(RM_V) -f ${S}(TGT)

install:
	${S}(MAKEDIR_OUTPUT)
	${S}(CP_V) -r ${S}(TGT_LIB_H)  ${S}(OUTPUT)/include
	${S}(CP_V) -r ${S}(TGT_LIB_A)  ${S}(OUTPUT)/lib
	${S}(CP_V) -r ${S}(TGT_LIB_SO) ${S}(OUTPUT)/lib

uninstall:
	${S}(RM_V) -f ${S}(OUTPUT)/include/${S}(TGT_LIB_H)
	${S}(RM_V) -f ${S}(OUTPUT)/lib/${S}(TGT_LIB_A)
	${S}(RM_V) -f ${S}(OUTPUT)/lib/${S}(TGT_LIB_SO)
!
}

autogen_android_mk()
{
cat > ${ANDROID_MK} <<!
LOCAL_PATH := ${S}(call my-dir)

include ${S}(CLEAR_VARS)

# because liblog is used by android default, so change to libglog
LOCAL_MODULE := ${MODULE}

LIBRARIES_DIR	:= ${S}(LOCAL_PATH)/../

LIBGZF_INC := ${S}(LIBRARIES_DIR)/libgzf/

LOCAL_C_INCLUDES := ${S}(LOCAL_PATH) \
                    ${S}(LIBGZF_INC)

# Add your application source files here...
LOCAL_SRC_FILES := ${LIBFOO_C}

include ${S}(BUILD_SHARED_LIBRARY)
!
}

autogen_readme_md()
{
cat > ${README_MD} <<!
##${MODULE}
This is a simple ${MODULE} library.

!
}

case $# in
0)
	usage;
	exit;;
1)
	echo "";;
esac

mkdir_libfoo
autogen_libfoo_c
autogen_libfoo_h
autogen_test_libfoo_c
autogen_makefile
autogen_android_mk
autogen_readme_md

