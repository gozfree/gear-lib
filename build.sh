#!/bin/bash
set -e

CMD=$0
MODULE=all
ARCH=linux
MODE=debug
ASAN=0


#add supported platform to here
PLATFORM="[linux|pi|android|ios]"

#basic libraries
BASIC_LIBS="libposix libtime liblog libdarray libthread libgevent libworkq libdict libhash libsort \
	    librbtree libringbuffer libvector libstrex libmedia-io \
            libdebug libfile libqueue libplugin libhal libsubmask"
MEDIA_LIBS="libavcap libmp4"
FRAMEWORK_LIBS="libipc"
NETWORK_LIBS="libsock libptcp librpc librtsp librtmpc"



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
	echo "framework libraries (optional):"
	for item in $FRAMEWORK_LIBS; do
		echo "$CMD $item $PLATFORM [debug|release]";
	done
	echo ""
	echo "network libraries (optional):"
	for item in $NETWORK_LIBS; do
		echo "$CMD $item $PLATFORM [debug|release]";
	done
	echo ""
	echo "media libraries (optional):"
	for item in $MEDIA_LIBS; do
		echo "$CMD $item $PLATFORM [debug|release]";
	done
	exit
}

#-o或--options选项后面接可接受的短选项，如ab:c::，表示可接受的短选项为-a -b -c，其中-a选项不接参数，-b选项后必须接参数，-c选项的参数为可选的
#-l或--long选项后面接可接受的长选项，用逗号分开，冒号的意义同短选项。
#-n选项后接选项解析错误时提示的脚本名字
ARGS=`getopt -o a:m:h --long arch:,module:,help,asan: -n 'build.sh' -- "$@"`
if [ $? != 0 ]; then
    echo "Terminating..."
    exit 1
fi

#将规范化后的命令行参数分配至位置参数（$1,$2,...)
eval set -- "${ARGS}"

while true
do
    case "$1" in
        -h|--help)
            usage;
            shift
            ;;
        -a|--arch)
            ARCH=$2
            shift 2
            ;;
        -m|--mode)
            MODE=$2
            shift 2
            ;;
        --module)
            MODULE=$2
            shift 2
            ;;
        --asan)
            ASAN=$2
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "invalid arguments: $@"
            exit 1
            ;;
    esac
done

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
		CROSS_PREFIX=arm-linux-gnueabihf-
		;;
	"android")
		CROSS_PREFIX=arm-linux-androideabi-
		;;
	"linux")
		CROSS_PREFIX=
		;;
	"ios")
		echo "not support cross compile, should compile native on Mac"
		exit 0;
		;;
	*)
		echo "arch: $ARCH not supported"
		;;
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

check_install()
{
	if [ ! -d "/usr/local/include/gear-lib" ]; then
		mkdir -p /usr/local/include/gear-lib
	fi
	if [ ! -d "/usr/local/lib/gear-lib" ]; then
		mkdir -p /usr/local/lib/gear-lib
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
		echo "==== build ${ARCH} ${MODULE} start..."
		MAKE="make ARCH=${ARCH} OUTPUT=${OUTPUT} MODE=${MODE} ASAN=${ASAN}"
		${MAKE}  > /dev/null
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
		check_install
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
