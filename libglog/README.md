##libglog
This is a simple log library.

Call libglog is only because -llog is used in android :-(

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

* for x86 and arm libglog.a libglog.so libglog.h will be installed in /usr/local/ by default

  and for android the libglog will be installed in ../output/android/

* libglog is built on rasbarrypi for arm platform test.

##Stability
  $ `valgrind --leak-check=full ./test_libglog`
  $ `valgrind --tool=helgrind ./test_libglog`

