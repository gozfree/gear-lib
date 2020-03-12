#!/bin/bash
set -e

CMD=$0
MODULE=$1
ARCH=$2
MODE=$3

#default is linux
case $# in
0)
	MODULE=all;
	ARCH=linux;
	MODE=debug;;
1)
	ARCH=linux;
	MODE=debug;;
2)
	MODE=debug;;

esac


#add supported platform to here
PLATFORM="[linux|pi|android|ios]"

#basic libraries
BASIC_LIBS="libmacro libtime liblog libdarray libgevent libworkq libdict libhash libsort \
	    librbtree libringbuffer libthread libvector libbase64 libmedia-io \
            libdebug libfile libuvc libmp4parser libqueue libplugin libhal libsubmask"
FRAMEWORK_LIBS="libipc"
NETWORK_LIBS="libskt librpc "

usage()
{
	echo "==== usage ===="
	echo "$CMD <module> [platform] [mode]"
	echo "<module>: library to compile or all library, must needed"
	echo "[platform]: linux, raspberrypi or android, default is linux, optional"
	echo "[mode]: debug or release, default is debug, optional"
	echo ""
	echo "./build.sh all $PLATFORM [debug|release]"
	echo "./build.sh basic_libs $PLATFORM [debug|release]"
	echo "./build.sh network_libs $PLATFORM [debug|release]"
	echo ""
	echo "basic libraries (must):"
	for item in $BASIC_LIBS; do
		echo "$CMD $item $PLATFORM [debug|release]";
	done
	echo ""
	echo "network libraries (optional):"
	for item in $NETWORK_LIBS; do
		echo "$CMD $item $PLATFORM [debug|release]";
	done
	exit
}

config_linux()
{
	CROSS_PREFIX=
}

config_pi()
{
	CROSS_PREFIX=arm-linux-gnueabihf-
}

config_android()
{
	CROSS_PREFIX=arm-linux-androideabi-
}

config_ios()
{
	echo "need a mac computer, who can help me :-)"
	exit 0;
}

config_common()
{
	STRIP=${CROSS_PREFIX}strip
	LIBS_DIR=`pwd`/gear-lib
	OUTPUT=${LIBS_DIR}/output/${ARCH}/
}

config_arch()
{
	case $ARCH in
	"pi")
		config_pi;;
	"android")
		config_android;;
	"linux")
		config_linux;;
	"ios")
		config_ios;;
	*)
		usage;;
	esac
}

check_output()
{
	if [ ! -d "${OUTPUT}/include/gear-lib" ]; then
		mkdir -p ${OUTPUT}/include/gear-lib
	fi
	if [ ! -d "${OUTPUT}/{release,debug}/lib/gear-lib" ]; then
		mkdir -p ${OUTPUT}/{release,debug}/lib/gear-lib
	fi
}

install_dep()
{
	return
}

build_module()
{
	MODULE_DIR=${LIBS_DIR}/$1
	ACTION=$2
	if [ ! -d "${MODULE_DIR}" ]; then
		echo "==== build ${ARCH} ${MODULE} failed!"
		echo "     dir \"${MODULE_DIR}\" is not exist"
		return
	fi
	cd ${LIBS_DIR}/${MODULE}/

	case $ACTION in
	"clean")
		make clean > /dev/null
		echo "==== clean ${ARCH} ${MODULE} done."
		return
		;;
	"install")
		MAKE="make ARCH=${ARCH} MODE=${MODE}"
		${MAKE} install > /dev/null
		if [ $? -ne 0 ]; then
			echo "==== install ${ARCH} ${MODULE} failed"
			return;
		else
			echo "==== install ${ARCH} ${MODULE} done."
		fi
		;;
	"uninstall")
		MAKE="make ARCH=${ARCH} MODE=${MODE}"
		${MAKE} uninstall > /dev/null
		if [ $? -ne 0 ]; then
			echo "==== uninstall ${ARCH} ${MODULE} failed"
			return;
		else
			echo "==== uninstall ${ARCH} ${MODULE} done."
		fi
		;;
	*)
		MAKE="make ARCH=${ARCH} OUTPUT=${OUTPUT} MODE=${MODE}"
		if [[ ${ARCH} == "linux" || ${ARCH} == "pi" || ${ARCH} == "android" ]]; then
			${MAKE}  > /dev/null
		else
			echo "${ARCH} not support now" #make -f Makefile.${ARCH} > /dev/null
		fi
		if [ $? -ne 0 ]; then
			echo "==== build ${ARCH} ${MODULE} failed"
			return;
		else
			echo "==== build ${ARCH} ${MODULE} done."
		fi
		${MAKE} install > /dev/null
		if [ $? -ne 0 ]; then
			echo "==== install ${ARCH} ${MODULE} failed"
			return;
		fi
		;;
	esac
}

build_all()
{
	for item in $BASIC_LIBS $NETWORK_LIBS $FRAMEWORK_LIBS; do
		MODULE="$item"
		ARG2=$1
		build_module $MODULE $ARG2
	done
}

do_build()
{
	case $MODULE in
	"all")
		rm -rf output
		check_output
		build_all clean;
		build_all;;
	"clean")
		build_all clean;;
	"dep")
		install_dep;;
	"install")
		build_all install;;
	"uninstall")
		build_all uninstall;;
	"help")
		usage;;
	*)
		build_module $MODULE;;
	esac
}

config_arch
config_common
check_output
do_build
