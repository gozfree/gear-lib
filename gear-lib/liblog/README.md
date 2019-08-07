## liblog
This is a simple log library.

Support linux(x86, arm), android, ios, and support write log to stderr, file or rsyslog.

## How To Use
For console usage, no need to rebuild for setting log level, only to set bash env as follows:

 $ `export LIBLOG_LEVEL=verbose`

```javascript
  [timestamp][prog_name pid tid][level][tag][file:line: func] message
```

 $ `export LIBLOG_LEVEL=info`

 all log which level <= info will be output

 $ `export LIBLOG_TIMESTAMP=1`

 enable timestamp

## How To Build
* x86/arm build
  $ `make clean`

  $ `make` (or `make ARCH=pi`)

  $ `sudo make install`

* android build
  $ `cd ../android_jni_libs/jni`

  $ `ndk-build glog`

  The name of android version is called `libglog` because -llog is already used in original android :-(

* for x86 and arm liblog.a liblog.so liblog.h will be installed in /usr/local/ by default

  and for android the liblog will be installed in ../android_jni_libs/obj/local/armeabi/

## Code Parser
  $ `sparse *.c`

## Stability
  $ `valgrind --leak-check=full ./test_liblog`

  $ `valgrind --tool=helgrind ./test_liblog`

