## libdebug
This is a simple libdebug library.

NOTE: arm-gcc seems can't show backtrace, arm-g++ is ok

## How to get gcc all inner macro
$ gcc -E -dM a.c

## How to pass SIGINT to program

```
=====================================
(gdb) handle SIGINT nostop print pass
SIGINT is used by the debugger.
Are you sure you want to change it? (y or n) y

Signal Stop Print Pass to program Description
SIGINT No Yes Yes Interrupt
(gdb)
=====================================
```
