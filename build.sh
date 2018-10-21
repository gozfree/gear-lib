#!/bin/bash

CMD=$0
MODULE=$1
ARCH=$2

#default is linux
case $# in
0)
	MODULE=all;
	ARCH=linux;;
1)
	ARCH=linux;;
esac

#add supported platform to here
PLATFORM="[linux|pi|android|ios]"

#basic libraries
BASIC_LIBS="libmacro libatomic libtime liblog libgevent libworkq libdict libsort \
	    librbtree libringbuffer liblock libthread libconfig libvector libbase64 \
            libdebug libfile libuvc libmp4parser libfilewatcher libqueue "
FRAMEWORK_LIBS="libipc"
NETWORK_LIBS="libskt libstun libptcp librpc libp2p librtsp"

usage()
{
	echo "==== usage ===="
	echo "$CMD <module> [platform]"
	echo "<module>: library to compile or all library, must needed"
	echo "[platform]: linux, raspberrypi or android, default is linux, optional"
	echo ""
	echo "./build.sh all $PLATFORM"
	echo "./build.sh basic_libs $PLATFORM"
	echo "./build.sh network_libs $PLATFORM"
	echo ""
	echo "basic libraries (must):"
	for item in $BASIC_LIBS; do
		echo "$CMD $item $PLATFORM";
	done
	echo ""
	echo "network libraries (optional):"
	for item in $NETWORK_LIBS; do
		echo "$CMD $item $PLATFORM";
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
	LIBS_DIR=`pwd`
	OUTPUT=${LIBS_DIR}/output/${ARCH}
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
	if [ ! -d "${OUTPUT}" ]; then
		mkdir -p ${OUTPUT}/include
		mkdir -p ${OUTPUT}/lib
	fi
}

install_dep()
{
	sudo apt-get install libjansson-dev
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
		MAKE="make ARCH=${ARCH}"
		${MAKE} install > /dev/null
		if [ $? -ne 0 ]; then
			echo "==== install ${ARCH} ${MODULE} failed"
			return;
		else
			echo "==== install ${ARCH} ${MODULE} done."
		fi
		;;
	"uninstall")
		MAKE="make ARCH=${ARCH}"
		${MAKE} uninstall > /dev/null
		if [ $? -ne 0 ]; then
			echo "==== uninstall ${ARCH} ${MODULE} failed"
			return;
		else
			echo "==== uninstall ${ARCH} ${MODULE} done."
		fi
		;;
	*)
		MAKE="make ARCH=${ARCH} OUTPUT=${OUTPUT}"
		if [[ ${ARCH} == "linux" || ${ARCH} == "pi" || ${ARCH} == "android" ]]; then
			${MAKE} > /dev/null
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
