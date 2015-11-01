##liblog
This is a simple log library.

The name of android version is called libglog because -llog is used in original android :-(

Support linux(x86, arm), android, ios, and support write log to stderr, file or rsyslog.

##How To Use
For console usage, no need to rebuild for setting log level, only to set bash env as follows:

 $ `export LIBLOG_LEVEL=verbose`

```javascript
  [timestamp][prog_name pid tid][level][tag][file:line: func] message
```

 $ `export LIBLOG_LEVEL=info`

 all log which level <= info will be output

 $ `export LIBLOG_TIMESTAMP=1`

 enable timestamp

##How To Build
  $ `make clean`

  $ `make ARCH=x86`  (ARCH=pi, or ARCH=android)

  $ `sudo make install`

* for x86 and arm liblog.a liblog.so liblog.h will be installed in /usr/local/ by default

  and for android the liblog will be installed in ../output/android/

* liblog is built on rasbarrypi for arm platform test.

##Stability
  $ `valgrind --leak-check=full ./test_liblog`

  $ `valgrind --tool=helgrind ./test_liblog`

