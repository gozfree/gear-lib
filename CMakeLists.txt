cmake_minimum_required(VERSION 3.9)

include_directories(include)

set(base64_src src/libbase64.c)
set(log_src src/liblog.c)
set(file_src src/filewatcher.c src/fio.c src/io.c src/libfile.c)
set(ipc_src src/libipc.c src/msgq_posix.c src/msgq_sysv.c src/netlink.c src/shm.c src/unix_socket.c)
set(gevent_src src/epoll.c src/libgevent.c src/poll.c src/select.c)
set(dict_src src/libdict.c)
set(macro_src src/libmacro.c)
set(queue_src src/libqueue.c)
set(vector_src src/libvector.c)

add_library(gevent SHARED ${gevent_src})
add_library(dict SHARED ${dict_src})
add_library(ipc SHARED ${ipc_src})
target_link_libraries(ipc rt gevent dict)

add_library(log SHARED ${log_src})
add_library(file SHARED ${file_src})
add_library(macro SHARED ${macro_src})
add_library(queue SHARED ${queue_src})
add_library(vector SHARED ${vector_src})

