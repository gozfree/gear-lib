## libraries
This is a collection of basic libraries.


### Features
  * All are written in POSIX C, aim to used compatibility on x86, arm, android, ios.
  * Each of library is as soon as independently, and the APIs are easily to use.
  * Depend on none of open source third party libraries.
  * ...

### About Build
  * by default, build x86 on linux, and libxxx folder can be built independently
  * if only "libxxx folder" is checkout without "build folder", you can only build x86 and no color set
  * with "build folder", you can build x86, pi, android, ios and color is also set
  * "sudo make install" is needed when build libxxx alone.

### How To Build
  $ `./build clean`

  $ `./build` (default x86)

  $ `./build all android` (for android)

  $ `./build all pi` (for rasberrypi)

  $ `./build install` (may need sudo)

  libxxx.h, libxxx.so or libxxx.a of libraries are in ./output/$(ARCH)

### How To ndk-build (for android ndk develop)
  $ `cd android_jni_libs`

  $ `ndk-build`

### How To Autogen C code
  $ `./autogen_lib.sh libfoo`
