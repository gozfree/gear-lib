## libraries
  This is a collection of basic libraries.
  * All are written in POSIX C, aim to used compatibility on x86, arm, android, ios.
  * Each of library is as soon as independently, and the APIs are easily to use.
  * Depend on none of open source third party libraries.

### Include

  * [liblog](liblog/README.md)

  * [libconfig](libconfig/README.md)

  * [libdict](libdict/README.md)

  * [libhash](libhash/README.md)

  * [libptcp](libptcp/README.md)


### How To Build
  * linux platform (32/64 bit)

   `$ cd libraries`

   `$ ./build.sh`

   `$ sudo ./build.sh install`

  * host(linux) target(rasberrypi)

    need toolchain of rasberrypi

   `$ ./build.sh all pi`

  * host(rasberrypi board)

   `$ ./build.sh`

   `$ sudo ./build install`

  * android arm cross compile

    need toolchain of arm-android

   `$ ./build.sh all android`

  * android naitve develop

    need ndk tools

   `$ cd android_jni_libs`

   `$ ndk-build`

   libxxx.h, libxxx.so or libxxx.a of libraries are in ./output/$(ARCH)

### How To Autogen C code
  `$ ./autogen_lib.sh libfoo`

### About Build
  * by default, build x86 on linux, and libxxx folder can be built independently
  * if only "libxxx folder" is checkout without "build folder", you can only build x86 and no color set
  * with "build folder", you can build x86, pi, android, ios and color is also set
  * "sudo make install" is needed when build libxxx alone.


