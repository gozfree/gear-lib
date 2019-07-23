# Gear Libraries

English | [简体中文](README.cn.md)

[![Build](https://travis-ci.org/gozfree/gear-lib.svg?branch=master)](https://travis-ci.org/gozfree/gear-lib)
[![Release](https://img.shields.io/github/release/gozfree/gear-lib.svg)](https://github.com/gozfree/gear-lib/releases)
[![License](https://img.shields.io/github/license/gozfree/gear-lib.svg)](https://github.com/gozfree/gear-lib/blob/master/LICENSE.MIT)

This is a collection of basic libraries.
* All are written in POSIX C, aim to used compatibility on x86, arm, android, ios.
* Each of library is independent project, only include the needed library to your project instead of the whole libraries
* Aim to reuse for embedded and network service development

|type|name|
|----|----|
|data struct|dict hash ringbuffer queue rbtree sort vector macro
|network|rtsp rtmp skt p2p rpc ipc
|async|gevent workq thread
|I/O parser|base64 config log file filewatcher strex submask
|multi-media|uvc jpeg-ex mp4parser
|misc|debug hal plugin time posix4win

## How To Build

### Windows
  * Windows7 install "Microsoft Visual Studio 10.0"  
    open cmd.exe  
   `> "D:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\vcvars32.bat"`  
   `> cd libraries\libposix4win\`  
   `> nmake /f Makefile.nmake clean`  
   `> nmake /f Makefile.nmake`  
   default debug version, compiler release version  
   `> nmake /f Makefile.nmake clean`  
   `> nmake /f Makefile.nmake MODE=release`
   
### Linux
  * linux platform (>= Ubuntu14.04 >= gcc-4.8.4 32/64 bit)  
   `$ cd libraries`  
   `$ ./build.sh`     
   `$ sudo ./build.sh install`  
   default debug version，compiler release version  
   `$ ./build.sh {all|libxxx} linux release`   
   `$ sudo ./build.sh install linux release`  
   
  * host(linux) target(rasberrypi)  
    (you need download [toolchain of rasberrypi](https://github.com/raspberrypi/tools.git))  
   `$ ./build.sh all pi`  
   default debug version，compiler release version  
   `$ ./build.sh {all|libxxx} pi release`  

  * host(rasberrypi board)  
   `$ ./build.sh`  
   `$ sudo ./build.sh install`  
   default debug version，compiler release version  
   `$ ./build.sh {all|libxxx} linux release`  
   `$ sudo ./build.sh install linux release`  

  * android arm cross compile  
   (you need download [android-ndk-r9-linux-x86_64.tar.bz2](http://dl.google.com/android/ndk/android-ndk-r9-linux-x86_64.tar.bz2))  
   `$ ./build.sh all android`  
   default debug version，compiler release version  
   `$ ./build.sh {all|libxxx} android release`  
   
  * android naitve develop  
    also need ndk tools  
   `$ cd android_jni_libs`  
   `$ ndk-build`  
   default debug version，compiler release version  
   `$ ndk-build MODE=release`  
   After install, the libxxx.xx will be installed in /usr/local/lib/.  
   libxxx.h, libxxx.so or libxxx.a of libraries are also in ./output/$(ARCH)  


## How To Autogen C template code
   If you want to add your own library into the build script, no need repeat the same code, only auto gen libxxx framework.  
  `$ ./build/autogen_lib.sh libfoo`

## About Build
  * by default, build x86 on linux, and libxxx folder can be built independently
  * if only "libxxx folder" is checkout without "build folder", you can only build x86 and no color set
  * with "build folder", you can build x86, pi, android, ios and color is also set
  * "sudo make install" is needed when build libxxx alone.

## License
Please refer to the LICENSE file for detailed information.

## Author & Contributing
Welcome pull request to the libraries.  

