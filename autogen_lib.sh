#!/bin/bash

CMD=$0
MODULE=$1

MAKEFILE=Makefile
LIBFOO_C=${MODULE}.c
LIBFOO_H=${MODULE}.h
TEST_LIBFOO_C=test_${MODULE}.c
README_MD=README.md
ANDROID_MK=Android.mk
VERSION_SH=version.sh

LIBFOO_MACRO=`echo ${MODULE} | tr 'a-z' 'A-Z'`

YEAR=$(date '+%Y')
S='$'
exclamation='!'

LGPL_HEADER=\
"/******************************************************************************
 * Copyright (C) 2014-${YEAR} Zhifeng Gong <gozfree@163.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libraries; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 ******************************************************************************/"

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
${LGPL_HEADER}
#ifndef ${LIBFOO_MACRO}_H
#define ${LIBFOO_MACRO}_H

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
${LGPL_HEADER}
#include "${LIBFOO_H}"
#include <stdio.h>
#include <stdlib.h>

!
}

autogen_test_libfoo_c()
{
cat > ${TEST_LIBFOO_C} <<!
${LGPL_HEADER}
#include "${LIBFOO_H}"
#include <stdio.h>
#include <stdlib.h>

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
# common
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

###############################################################################
# target and object
###############################################################################
LIBNAME		= ${MODULE}
VERSION_SH	= ${S}(shell pwd)/version.sh ${S}(LIBNAME)
VER		= ${S}(shell ${S}(VERSION_SH); awk '/define\ ${S}(LIBNAME)_version/{print ${S}${S}3}' version.h)
TGT_LIB_H	= ${S}(LIBNAME).h
TGT_LIB_A	= ${S}(LIBNAME).a
TGT_LIB_SO	= ${S}(LIBNAME).so
TGT_LIB_SO_VER	= ${S}(TGT_LIB_SO).${S}{VER}
TGT_UNIT_TEST	= test_${S}(LIBNAME)

OBJS_LIB	= ${S}(LIBNAME).o
OBJS_UNIT_TEST	= test_${S}(LIBNAME).o

###############################################################################
# cflags and ldflags
###############################################################################
CFLAGS	:= -g -Wall -Werror -fPIC
CFLAGS	+= ${S}(${S}(ARCH)_CFLAGS)
CFLAGS	+= -I${S}(OUTPUT)/include

SHARED	:= -shared

LDFLAGS	:= ${S}(${S}(ARCH)_LDFLAGS)
LDFLAGS	+= -pthread

###############################################################################
# target
###############################################################################
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
	@mv ${S}(TGT_LIB_SO) ${S}(TGT_LIB_SO_VER)
	@ln -sf ${S}(TGT_LIB_SO_VER) ${S}(TGT_LIB_SO)

${S}(TGT_UNIT_TEST): ${S}(OBJS_UNIT_TEST) ${S}(ANDROID_MAIN_OBJ)
	${S}(CC_V) -o ${S}@ $^ ${S}(TGT_LIB_A) ${S}(LDFLAGS)

clean:
	${S}(RM_V) -f ${S}(OBJS)
	${S}(RM_V) -f ${S}(TGT)
	${S}(RM_V) -f version.h
	${S}(RM_V) -f ${S}(TGT_LIB_SO)*
	${S}(RM_V) -f ${S}(TGT_LIB_SO_VER)

install:
	${S}(MAKEDIR_OUTPUT)
	${S}(CP_V) -r ${S}(TGT_LIB_H)  ${S}(OUTPUT)/include
	${S}(CP_V) -r ${S}(TGT_LIB_A)  ${S}(OUTPUT)/lib
	${S}(CP_V) -r ${S}(TGT_LIB_SO) ${S}(OUTPUT)/lib
	${S}(CP_V) -r ${S}(TGT_LIB_SO_VER) ${S}(OUTPUT)/lib

uninstall:
	${S}(RM_V) -f ${S}(OUTPUT)/include/${S}(TGT_LIB_H)
	${S}(RM_V) -f ${S}(OUTPUT)/lib/${S}(TGT_LIB_A)
	${S}(RM_V) -f ${S}(OUTPUT)/lib/${S}(TGT_LIB_SO)
	${S}(RM_V) -f ${S}(OUTPUT)/lib/${S}(TGT_LIB_SO_VER)
!
}

autogen_android_mk()
{
cat > ${ANDROID_MK} <<!
LOCAL_PATH := ${S}(call my-dir)

include ${S}(CLEAR_VARS)

LOCAL_MODULE := ${MODULE}

LIBRARIES_DIR	:= ${S}(LOCAL_PATH)/../

LOCAL_C_INCLUDES := ${S}(LOCAL_PATH)

# Add your application source files here...
LOCAL_SRC_FILES := ${LIBFOO_C}

include ${S}(BUILD_SHARED_LIBRARY)
!
}

autogen_readme_md()
{
cat > ${README_MD} <<!
## ${MODULE}
This is a simple ${MODULE} library.

!
}

autogen_version_sh()
{
cat > ${VERSION_SH} <<!
#!/bin/sh
major=0
minor=1
patch=0

libname=\$1
output=version.h

LIBNAME=\`echo \${libname} | tr 'a-z' 'A-Z'\`
export version=\${major}.\${minor}.\${patch}
export buildid=\`git log -1 --pretty=format:"git-%cd-%h" --date=short .\`
autogen_version_h()
{
cat > version.h <<!
/* Automatically generated by version.sh - do not modify! */
#ifndef \${LIBNAME}_VERSION_H
#define \${LIBNAME}_VERSION_H

#define \${libname}_version \${version}
#define \${libname}_buildid \${buildid}

#endif

${exclamation}
}

autogen_version_h
!

chmod a+x ${VERSION_SH}
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
autogen_version_sh
autogen_android_mk
autogen_readme_md

