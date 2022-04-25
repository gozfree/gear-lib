COMPONENT_ADD_INCLUDEDIRS = . ./libposix ./libthread ./libdarray ./libmedia-io ./libqueue ./liblog
COMPONENT_SRCDIRS = libposix libdarray libqueue libthread libmedia-io librtmpc liblog libgevent
COMPONENT_OBJEXCLUDE = libposix/libposix4win.o libposix/libposix4nix.o libposix/libposix4rtthread.o libposix/test_libposix.o libdarray/test_libdarray.o libqueue/test_libqueue.o libthread/test_libthread.o libmedia-io/test_libmedia-io.o librtmpc/test_librtmpc.o liblog/test_liblog.o libgevent/iocp.o libgevent/epoll.o libgevent/wepoll.o
COMPONENT_DEPENDS := newlib
CFLAGS += -DFREERTOS -DESP32 #for libposix
CFLAGS += -DNO_CRYPTO #for librtmpc
CFLAGS += -Wno-format-truncation
