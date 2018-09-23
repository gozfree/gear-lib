libraries [中文说明](README.cn.md) [![Build Status](https://travis-ci.org/gozfree/libraries.svg?branch=master)](https://travis-ci.org/gozfree/libraries) [![Release](https://img.shields.io/github/release/gozfree/libraries.svg)](https://github.com/gozfree/libraries/releases) [![License](https://img.shields.io/github/license/gozfree/libraries.svg)](https://github.com/gozfree/libraries/blob/master/LICENSE.LGPL)
=========
This is a collection of basic libraries.
* All are written in POSIX C, aim to used compatibility on x86, arm, android, ios.
* Each of library is as independently as possible, and the APIs are easily to use.
* Depend on none of open source third party libraries.
* Aim to reduce duplication of the wheel.

|name|descript|name|descript|name|descript|
|----|--------|----|--------|----|--------|
|[libatomic](libatomic)|Atomic operation|[libbase64](libbase64)|Base64/32 encode/decode|[libcmd](libcmd)|Based on readline like bash
|[libconfig](libconfig)|Support ini/json|[libdebug](libdebug)|Help to trace crash like gdb|[libdict](libdict)|Hash key-value dictonary
|[libfilewatcher](libfilewatcher)|Watch file/dir on event|[libfile](libfile)|File operations|[libgevent](libgevent) | Reactor event, like libevent
|[libhal](libhal)|hardware Abstraction Layer|[libhash](libhash)|Hash key-value based on hlist|[libipc](libipc)|Support mqueue/netlink/shm
|[liblog](liblog)|Support console/file/rsyslog|[libmacro](libmacro)|Basic Macro define|[libmp4parser](libmp4parser)|MP4 format parser
|[liblock](liblock)|Lock/mutex/sem wrapper|[libthread](libthread)|Thread wrapper|[libp2p](libp2p)|p2p punch hole and transfer
|[libplugin](libplugin)|Dynamic link plugin|[libptcp](libptcp)|Pseudo Tcp Socket over UDP|[librbtree](librbtree)|linux kernel rbtree
|[librpc](librpc)|Remote Procedure Call|[librtsp](librtsp)|Rtsp wrapper|[libskt](libskt)|Socket wrapper
|[libstun](libstun)|STUN protocol wrapper|[libtime](libtime)|Time wrapper|[libqueue](libqueue)|support memory hook
|[libringbuffer](libringbuffer)|c ringbuffer|[libworkq](libworkq)|Work queue in userspace|[libvector](libvector)|c vector
|[libuvc](libuvc)|USB video class (V4L2)

## How To Build
Recommend Ubuntu14.04 gcc-4.8.4+
  * linux platform (32/64 bit)  
   `$ cd libraries`  
   `$ ./build.sh`  
   `$ sudo ./build.sh install`

  * host(linux) target(rasberrypi)  
    (you need download [toolchain of rasberrypi](https://github.com/raspberrypi/tools.git))  
   `$ ./build.sh all pi`

  * host(rasberrypi board)  
   `$ ./build.sh`  
   `$ sudo ./build.sh install`  

  * android arm cross compile  
   (you need download [android-ndk-r9-linux-x86_64.tar.bz2](http://dl.google.com/android/ndk/android-ndk-r9-linux-x86_64.tar.bz2))  
   `$ ./build.sh all android`  

  * android naitve develop  
    also need ndk tools  
   `$ cd android_jni_libs`  
   `$ ndk-build`  

   After install, the libxxx.xx will be installed in /usr/local/lib/.  
   libxxx.h, libxxx.so or libxxx.a of libraries are also in ./output/$(ARCH)  

## How To Autogen C code
   If you want to add your own library into the build script, no need repeat the same code, only auto gen libxxx framework.  
  `$ ./autogen_lib.sh libfoo`

## About Build
  * by default, build x86 on linux, and libxxx folder can be built independently
  * if only "libxxx folder" is checkout without "build folder", you can only build x86 and no color set
  * with "build folder", you can build x86, pi, android, ios and color is also set
  * "sudo make install" is needed when build libxxx alone.

## License
LGPL/GPLv3. Please refer to the LICENSE file for detailed information.

## Author & Contributing
Welcome pull request to the libraries.  

|                                               |                                               |
|-----------------------------------------------|-----------------------------------------------|
| [CMShuyuhui](https://github.com/CMShuyuhui)   | [core1011](https://github.com/core1011)       |
| [elfring](https://github.com/elfring)         | [ktsaou](https://github.com/ktsaou)           |
| [zh794390558](https://github.com/zh794390558) | [gozfree](https://github.com/gozfree)         |
