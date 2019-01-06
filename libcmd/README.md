## libcmd
This is a simple interactive mode command library.

## How To Build
* x86/arm build

  $ `make clean`

  $ `make` (or `make ARCH=pi`)

  $ `sudo make install`

## Code Parser
  $ `sparse *.c`

## Stability
  $ `valgrind --leak-check=full ./test_libcmd`

  $ `valgrind --tool=helgrind ./test_libcmd`

