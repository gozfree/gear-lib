#!/bin/bash

CMD=$0
MODULE=$1
ARCH=$2

#default is x86
case $# in
0)
  MODULE=all;
  ARCH=x86;;
1)
  ARCH=x86;;
esac

#add supported platform to here
PLATFORM="[x86|pi|android]"

#basic libraries
BASIC_LIBS="libgzf liblog libgevent libworkq libdict"
NETWORK_LIBS="libskt"

usage()
{
  echo "==== build usage:"
  echo "$CMD <module> [platform]"
  echo "<module>: library to compile or all library, must needed"
  echo "[platform]: x86, raspberrypi or android, default is x86, optional"
  echo ""
  echo "./build.sh all $PLATFORM"
  echo "./build.sh basic_libs $PLATFORM"
  echo "./build.sh network_libs $PLATFORM"
  echo ""
  echo "basic libraries (must):"
  for item in $BASIC_LIBS
  do
    echo "$CMD $item $PLATFORM"
  done
  echo ""
  echo "network libraries (optional):"
  for item in $NETWORK_LIBS
  do
    echo "$CMD $item $PLATFORM"
  done
  exit
}

build_all()
{
  for item in $BASIC_LIBS $NETWORK_LIBS
  do
    MODULE="$item"
    build_module
  done
}

build_network_libs()
{
  for item in $NETWORK_LIBS
  do
    MODULE="$item"
    build_module
  done
}

config_x86()
{
  CROSS_PREFIX=
}

config_pi()
{
  CROSS_PREFIX=arm-linux-gnueabihf-
}

config_android()
{
  CROSS_PREFIX=arm-hisiv100nptl-linux-
}

config_common()
{
  STRIP=${CROSS_PREFIX}strip
  LIBS_DIR=`pwd`
  CPUS=`nproc`
  MAKE="make ARCH=${ARCH} OUTPUT=${LIBS_DIR}/output"
  echo "$MAKE"
}

build_module()
{
  if [ ! -d "${LIBS_DIR}/${MODULE}" ]; then
    echo "==== build ${MODULE} failed!"
    echo "     dir \"${LIBS_DIR}/${MODULE}\" is not exist"
    return;
  fi
  cd ${LIBS_DIR}/${MODULE}/

  make clean > /dev/null
  if [[ ${ARCH} == "x86" || ${ARCH} == "pi" ]]; then
    ${MAKE} > /dev/null
  else
    make -f Makefile.${ARCH} > /dev/null
  fi
  ${MAKE} install > /dev/null
  if [ $? -ne 0 ]; then
    echo "==== build ${MODULE} failed"
    return;
  else
    echo "==== build ${MODULE} ${ARCH} done."
  fi
}

build_liblog()
{
  build_module
}

case $ARCH in
"pi")
  config_pi;;
"android")
  config_android;;
"x86")
  config_x86;;
*)
  usage;;
esac

config_common

case $MODULE in
"all")
  build_all;;
"basic")
  build_ipcam_libs;;
"network")
  build_sys_libs;;
"liblog")
  build_liblog;;
"libskt")
  build_libskt;;
*)
  echo "$MODULE not found"
  usage;;
esac
