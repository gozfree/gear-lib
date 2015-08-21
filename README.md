## libraries
This is a collection of basic libraries.


## Features
  * All are written in POSIX C, aim to used compatibility on x86, arm, android, ios.
  * Each of library is as soon as independently, and the APIs are easily to use.
  * Depend on none of open source third party libraries.
  * ...

## How To Build
  $ `./build clean`

  $ `./build` (default x86)

  $ `./build all android` (for android)

  $ `./build all pi` (for rasberrypi)

  $ `./build install` (may need sudo)

  libxxx.h, libxxx.so or libxxx.a of libraries are in ./output/$(ARCH)
