COMPONENT_ADD_INCLUDEDIRS = . ./libposix ./libthread ./libdarray ./libmedia-io ./libqueue ./liblog ./libavcap ./libfile ./libhal ./librbtree
COMPONENT_SRCDIRS = libposix liblog libdarray libqueue libthread libmedia-io librtmpc libgevent libavcap libfile libhal librbtree
COMPONENT_OBJEXCLUDE = libposix/libposix4win.o libposix/libposix4nix.o libposix/libposix4rtthread.o libposix/test_libposix.o \
		       libavcap/dshow.o libavcap/xcbgrab.o libavcap/v4l2.o libavcap/test_libavcap.o libavcap/pulseaudio.o libavcap/uvc.o \
		       libfile/test_libfile.o libfile/filewatcher.o \
		       libdarray/test_libdarray.o libqueue/test_libqueue.o libthread/test_libthread.o libmedia-io/test_libmedia-io.o \
		       librtmpc/test_librtmpc.o liblog/test_liblog.o libgevent/iocp.o libgevent/epoll.o libgevent/wepoll.o \
		       libhal/hal_nix.o libhal/hal_win.o libhal/test_hal.o \
		       librbtree/test_librbtree.o

COMPONENT_DEPENDS := newlib
CFLAGS += -DFREERTOS -DESP32 #for libposix
CFLAGS += -DNO_CRYPTO #for librtmpc
CFLAGS += -Wno-format-truncation
