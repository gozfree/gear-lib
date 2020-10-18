COMPONENT_ADD_INCLUDEDIRS = . ./libposix ./libmacro ./libthread ./libdarray ./libmedia-io ./libqueue ./liblog
COMPONENT_SRCDIRS = libposix libposix4rtos libmacro libdarray libqueue libthread libmedia-io librtmpc  liblog
COMPONENT_OBJEXCLUDE = libposix/test_libposix.o libmacro/test_libmacor.o libdarray/test_libdarray.o libqueue/test_libqueue.o libthread/test_libthread.o libmedia-io/test_libmedia-io.o librtmpc/test_librtmpc.o liblog/test_liblog.o 
COMPONENT_DEPENDS := newlib
CFLAGS += -DFREERTOS #for libposix
CFLAGS += -DESP32 #for libposix4rtos
CFLAGS += -DNO_CRYPTO #for librtmpc
CFLAGS += -Wno-format-truncation
