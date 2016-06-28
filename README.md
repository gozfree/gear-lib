libraries [![Build Status](https://travis-ci.org/gozfree/libraries.svg?branch=master)](https://travis-ci.org/gozfree/libraries)
=========
This is a collection of basic libraries.
* All are written in POSIX C, aim to used compatibility on x86, arm, android, ios.
* Each of library is as independently as possible, and the APIs are easily to use.
* Depend on none of open source third party libraries.
* Aim to reduce duplication of the wheel.

## Include

| Library                 | Note                                                       |
|-------------------------|------------------------------------------------------------|
|  [libgzf](libgzf)       | The most basically header file of all libraries.           |
|  [libatomic](libatomic) | Atomic operation library.                                  |
|  [libcmd](libcmd)       | Based on readline, a bash like library.                    |
|  [libconfig](libconfig) | Configure library, support ini, json.                      |
|  [libdict](libdict)     | Hash key-value dictonary library.                          |
|  [libdlmod](libdlmod)   | Dynamic linking loader wrapper library.                    |
|  [libgevent](libgevent) | Reactor event library, like libevent                       |
|  [libhash](libhash)     | Hash key-value library based on hlist from kernel.         |
|  [libipc](libipc)       | Inter-Process Communication, support mqueue/netlink/shm.   |
|  [liblog](liblog)       | Log library, support console/file/rsyslog.                 |
|  [libosal](libosal)     | OSAL(Operating System Abstraction Layer) library.          |
|  [libp2p](libp2p)       | High level p2p punch hole library, easy API to use.        |
|  [libptcp](libptcp)     | Pseudo Tcp Socket over UDP, rewrite with C from libjingle. |
|  [librbtree](librbtree) | Librbtree comes from linux kernel rbtree.                  |
|  [librpc](librpc)       | Remote Procedure Call library.                             |
|  [libskt](libskt)       | Socket wrapper library for easy use.                       |
|  [libstun](libstun)     | STUN protocol wrapper library.                             |
|  [libtime](libtime)     | Time wrapper library for easy use.                         |
|  [liblock](liblock)     | Lock/mutex/sem wrapper library for easy use.               |
|  [libthread](libthread) | Thread wrapper library for easy use.                       |
|  [libworkq](libworkq)   | Work queue in userspace like work-queue/tasklet in kernel. |

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
GPL. Please refer to the LICENSE file for detailed information.

## Author & Contributing
Welcome pull request to the libraries.  
gozfree <gozfree@163.com>
